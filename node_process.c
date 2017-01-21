#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>

#include "global.h"
#include "err.h"
#include "int_vector.h"
#include "vector.h"

#define TASKS_MAX 1000

static void process_parameters(int argc, char **argv);

static void main_loop(int entries_size, struct pollfd *entries);

static struct int_vector *str_to_vector(char *str);

static void process_message_from_main_thread(int message_type, int read_dsc);

static void process_message_from_child(int message_type, int read_dsc, int write_dsc);

static void process_message_from_parent(int message_type, int read_dsc);

static void send_calculate(int dsc, int task_number);

static void send_result(int dsc, int task_number, int value); 

static void send_acknowledge(int dsc);

static void free_memory();


static int type;

static char type_c;

static int node_value = -1;

static char operation = '\0';

static int main_read_dsc;

static int main_write_dsc;

static bool is_value_set[TASKS_MAX + 1];

static int values[TASKS_MAX + 1];

static struct int_vector *parents_read_dsc;

static struct int_vector *parents_write_dsc;

static struct int_vector *children_read_dsc;

static struct int_vector *children_write_dsc;

static int received_results[TASKS_MAX + 1];

static int calculated_results[TASKS_MAX + 1];

static struct int_vector *waiting_for_result[TASKS_MAX];

int main(int argc, char **argv) {
	process_parameters(argc, argv);

	int entries_size = children_read_dsc->size + parents_read_dsc->size + 1;
	struct pollfd *entries = malloc(entries_size * sizeof(*entries));

	int i = 0;
	entries[i].events = POLLIN;
	entries[i].fd = main_read_dsc;
	i++;
	for(int j = 0; j < children_read_dsc->size; j++) {
		entries[i + j].events = POLLIN;
		entries[i + j].fd = children_read_dsc->elements[j];
	}
	i += children_read_dsc->size;
	for(int j = 0; j < parents_read_dsc->size; j++) {
		entries[i + j].events = POLLIN;
		entries[i + j].fd = parents_read_dsc->elements[j];
	}

	main_loop(entries_size, entries);

	free_memory();

	return 0;
}

static void process_parameters(int argc, char **argv) {
	if(argc != 9) {
		exit(1);
	}

	// Reading parameters
	type = atoi(argv[1]);
	char *param = argv[2];
	char *children_read_dsc_str = argv[3];
	char *children_write_dsc_str = argv[4];
	char *parents_read_dsc_str = argv[5];
	char *parents_write_dsc_str = argv[6];
	char *main_read_dsc_str = argv[7];
	char *main_write_dsc_str = argv[8];

	if(type == NUMBER) {
		type_c = 'N';
		node_value = atoi(param);
		for(int i = 0; i <= TASKS_MAX; i++) {
			is_value_set[i] = true;
			values[i] = node_value;
		}
	} else if(type == VARIABLE) {
		type_c = 'V';
		node_value = atoi(param);
		for(int i = 0; i <= TASKS_MAX; i++) {
			is_value_set[i] = false;
		}
	} else if(type == OPERATOR) {
		type_c = 'O';
		operation = *param;
		for(int i = 0; i <= TASKS_MAX; i++) {
			is_value_set[i] = false;
			received_results[i] = 0;
		}
	}

	main_read_dsc = atoi(main_read_dsc_str);
	main_write_dsc = atoi(main_write_dsc_str);

	children_read_dsc = str_to_vector(children_read_dsc_str);
	children_write_dsc = str_to_vector(children_write_dsc_str);
	parents_read_dsc = str_to_vector(parents_read_dsc_str);
	parents_write_dsc = str_to_vector(parents_write_dsc_str);

	for(int i = 0; i < TASKS_MAX; i++) {
		waiting_for_result[i] = create_int_vector();
	}

	if(type == VARIABLE && node_value == 0) {
		// To avoid corner case, main process is children of root node
		push_back_int(children_write_dsc, main_write_dsc);
	}
}

static void main_loop(int entries_size, struct pollfd *entries) {
	int ret;
	bool finished = false;
	while(!finished) {
		for(int i = 0; i < entries_size; i++) {
			entries[i].revents = 0;
		}
		ret = poll(entries, entries_size, -1);
		if(ret == -1) syserr("Error in poll");

		for(int i = 0; i < entries_size; i++) {
			if(entries[i].revents & (POLLIN | POLLERR | POLLHUP)) {
				int message_type;
				int buf_len = read(entries[i].fd, &message_type, sizeof(message_type));
				if(buf_len == -1) {
					syserr("Error in read");
				} else if(entries[i].fd == main_read_dsc && buf_len == 0) { // Main thread closed pipe
					finished = true;
				} else {
					if(i == 0) {
						process_message_from_main_thread(message_type, entries[i].fd);
					} else if(i <= children_read_dsc->size) {
						int response_dsc = children_write_dsc->elements[i - 1];
						process_message_from_child(message_type, entries[i].fd, response_dsc);
					} else {
						process_message_from_parent(message_type, entries[i].fd);
					}
				}
			}
		}
	}
}

static void process_message_from_main_thread(int message_type, int read_dsc) {
	int ret;
	if(message_type == SET_VALUE) {
		int task_number, new_value;
		ret = read(read_dsc, &task_number, sizeof(task_number));
		if(ret == -1) syserr("Error in read");
		ret = read(read_dsc, &new_value, sizeof(new_value));
		if(ret == -1) syserr("Error in read");

		is_value_set[task_number] = true;
		values[task_number] = new_value;
		send_acknowledge(main_write_dsc);
	} else if(message_type == CALCULATE) {
		int task_number;
		ret = read(read_dsc, &task_number, sizeof(task_number));
		if(ret == -1) syserr("Error in read");
		if(is_value_set[task_number]) {
			// Result is known
			send_result(main_write_dsc, task_number, values[task_number]);
		} else if(parents_write_dsc->size == 0) {
			// We don't have parents -- result is undefined
			send_result(main_write_dsc, -task_number, 0);
		} else {
			// We have to ask parent for his result
			send_calculate(parents_write_dsc->elements[0], task_number);
			push_back_int(waiting_for_result[task_number], main_write_dsc);
		}
	} else {
		printf("Incorrect message_type from main thread\n");
	}
}

static void process_message_from_child(int message_type, int read_dsc, int write_dsc) {
	int ret;
	if(message_type == CALCULATE) {
		int task_number;
		ret = read(read_dsc, &task_number, sizeof(task_number));
		if(ret == -1) syserr("Error in read");
		if(is_value_set[task_number]) {
			// Result is known
			send_result(write_dsc, task_number, values[task_number]);
		} else if(parents_read_dsc->size == 0) {
			// We don't have parents -- result is undefined
			send_result(write_dsc, -task_number, 0);
		} else {
			// We have to ask parents for their results
			push_back_int(waiting_for_result[task_number], write_dsc);
			for(int i = 0; i < parents_write_dsc->size; i++) {
				send_calculate(parents_write_dsc->elements[i], task_number);
			}
		}
	} else {
		printf("Incorrect message_type from child\n");
	}
}

static void process_message_from_parent(int message_type, int read_dsc) {
	int ret;
	if(message_type == RESULT) {
		int task_number, value;
		ret = read(read_dsc, &task_number, sizeof(task_number));
		if(ret == -1) syserr("Error in read");
		ret = read(read_dsc, &value, sizeof(value));
		if(ret == -1) syserr("Error in read");

		bool can_send = false;
		int return_task_number, return_value;
		if(type == OPERATOR) {
			if(task_number < 0) {
				// Parent returned undefined -- we can pass it without waiting for other results
				received_results[-task_number] = 3;
				return_task_number = task_number;
				return_value = value;
				can_send = true;
			} else {
				received_results[task_number]++;
				if(received_results[task_number] == 1) {
					// Received first result from parent -- can send value if it is unary operator
					if(operation == '-') {
						return_task_number = task_number;
						return_value = -value;
						can_send = true;
					} else {
						calculated_results[task_number] = value;
					}
				} else if(received_results[task_number] == 2) {
					// Received second result from parent -- calculate result and then send
					if(operation == '+') {
						calculated_results[task_number] += value;
					} else {
						calculated_results[task_number] *= value;
					}
					return_task_number = task_number;
					return_value = calculated_results[task_number];
					can_send = true;
				}
			}
		} else {
			// It is not operator -- we can pass received value further
			return_task_number = task_number;
			return_value = value;
			can_send = true;
		}
		if(can_send) {
			// Result is fully known -- send it to those who requested it
			int abs_task_number = task_number < 0 ? -task_number : task_number;
			for(int i = 0; i < waiting_for_result[abs_task_number]->size; i++) {
				int dsc = waiting_for_result[abs_task_number]->elements[i];
				send_result(dsc, return_task_number, return_value);
			}
			free_int_vector(waiting_for_result[abs_task_number]);
			waiting_for_result[abs_task_number] = create_int_vector();
		}
	} else {
		printf("Incorrect message_type from child\n");
	}
}

static void send_calculate(int dsc, int task_number) {
	int ret;
	int message_type = CALCULATE;
	ret = write(dsc, &message_type, sizeof(message_type));
	if(ret == -1) syserr("Error in write");
	ret = write(dsc, &task_number, sizeof(task_number));
	if(ret == -1) syserr("Error in write");
}

static void send_result(int dsc, int task_number, int value) {
	int ret;
	int message_type = RESULT;
	ret = write(dsc, &message_type, sizeof(message_type));
	if(ret == -1) syserr("Error in write");
	ret = write(dsc, &task_number, sizeof(task_number));
	if(ret == -1) syserr("Error in write");
	ret = write(dsc, &value, sizeof(value));
	if(ret == -1) syserr("Error in write");
}

static void send_acknowledge(int dsc) {
	int ret;
	int message_type = ACKNOWLEGDE;
	ret = write(dsc, &message_type, sizeof(message_type));
	if(ret == -1) syserr("Error in write");
}


static void free_memory() {
	free_int_vector(children_read_dsc);
	free_int_vector(children_write_dsc);
	free_int_vector(parents_read_dsc);
	free_int_vector(parents_write_dsc);
	for(int i = 0; i < TASKS_MAX; i++) {
		free_int_vector(waiting_for_result[i]);
	}
}

static struct int_vector *str_to_vector(char *str) {
	int num;
	int len;
	struct int_vector *vector = create_int_vector();
	while(sscanf(str, "%d%n", &num, &len) != EOF) {
		push_back_int(vector, num);
		str += len;
	}
	return vector;
}

