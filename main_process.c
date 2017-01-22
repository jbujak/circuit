#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>

#include "main_process.h"
#include "global.h"
#include "vector.h"
#include "int_vector.h"
#include "err.h"

#define NODES_MAX 5000

static int last_node = 0;

static int root_id = -1;

static pid_t node_pid[NODES_MAX];

static int read_dsc[NODES_MAX];

static int write_dsc[NODES_MAX];

static int received_results = 0;


static int create_node_process(struct node *node);

static void close_uneeded_pipes(struct node *node);

static void send_variables(int task_number, struct int_vector *variables, struct int_vector *values);

void calculate_with_initialization(int task_number, struct int_vector *variables, struct int_vector *values) {
	send_variables(task_number, variables, values);

	int ret;
	int message_type = CALCULATE;
	ret = write(write_dsc[root_id], &message_type, sizeof(message_type));
	if(ret == -1) syserr("Error in write");
	ret = write(write_dsc[root_id], &task_number, sizeof(task_number));
	if(ret == -1) syserr("Error in write");
}

void create_processes(struct node *source) {
	if(source == NULL) return; // reached leaf
	if(source->internal_id >= 0) return; // process already created

	create_processes(source->parents[0]);
	create_processes(source->parents[1]);

	struct vector *children = source->children;
	int read_pipe_dsc[2]; // pipe for reading messages from children
	int write_pipe_dsc[2]; // pipe for sending messages to children
	for(int i = 0; i < children->size; i++) {
		if(pipe(read_pipe_dsc) == -1) syserr("Error in pipe");
		if(pipe(write_pipe_dsc) == -1) syserr("Error in pipe");

		struct node *child = ((struct node*)(children->elements[i]));
		if(child->parent_read_dsc[0] == -1) {
			child->parent_read_dsc[0] = write_pipe_dsc[0];
			child->parent_write_dsc[0] = read_pipe_dsc[1];

		} else {
			child->parent_read_dsc[1] = write_pipe_dsc[0];
			child->parent_write_dsc[1] = read_pipe_dsc[1];
		}

		push_back_int(source->children_read_dsc, read_pipe_dsc[0]);
		push_back_int(source->children_write_dsc, write_pipe_dsc[1]);
	}

	create_node_process(source);
	close_uneeded_pipes(source);
}

void read_remaining_results(int expected_results) {
	int ret;
	while(received_results < expected_results) {
		int message_type, return_task_number, return_value;

		ret = read(read_dsc[root_id], &message_type, sizeof(message_type));
		if(ret == -1) syserr("Error in read");
		ret = read(read_dsc[root_id], &return_task_number, sizeof(return_task_number));
		if(ret == -1) syserr("Error in read");
		ret = read(read_dsc[root_id], &return_value, sizeof(return_value));
		if(ret == -1) syserr("Error in read");

		received_results++;
		if(return_task_number > 0) {
			printf("%d P %d\n", return_task_number, return_value);
		} else {
			printf("%d F\n", -return_task_number);
		}
	}
}

void join() {
	int ret;
	for(int i = 0; i < NODES_MAX; i++) {
		if(write_dsc[i] != 0) {
			ret = close(write_dsc[i]);
			if(ret == -1) syserr("Error in close");
		}
	}
	for(int i = 0; i < NODES_MAX; i++) {
		if(node_pid[i] != 0) {
			int status;
			waitpid(node_pid[i], &status, 0);
			if(status != 0) {
				fprintf(stderr, "Process %d exited with status %d\n", i, status);
			}
		}

	}
}

static int create_node_process(struct node *node) {
	char type_str[10];
	sprintf(type_str, "%d", node->type);

	char param[10];
	switch(node->type) {
		case NUMBER:
		case VARIABLE:
			sprintf(param, "%d", node->value);
			break;
		case OPERATOR:
			sprintf(param, "%c", node->operation);
	}

	char parents_read_dsc_str[20];
	char parents_write_dsc_str[20];
	if(node->parent_read_dsc[1] != -1) {
		sprintf(parents_read_dsc_str, "%d %d", node->parent_read_dsc[0], node->parent_read_dsc[1]);
		sprintf(parents_write_dsc_str, "%d %d", node->parent_write_dsc[0], node->parent_write_dsc[1]);
	} else if(node->parent_read_dsc[0] != -1) {
		sprintf(parents_read_dsc_str, "%d", node->parent_read_dsc[0]);
		sprintf(parents_write_dsc_str, "%d", node->parent_write_dsc[0]);
	} else {
		parents_read_dsc_str[0] = '\0';
		parents_write_dsc_str[0] = '\0';
	}

	char children_read_dsc_str[10000];
	char children_write_dsc_str[10000];
	char *children_read_dsc_str_ptr = children_read_dsc_str;
	char *children_write_dsc_str_ptr = children_write_dsc_str;
	for(int i = 0; i < node->children_write_dsc->size; i++) {
		children_read_dsc_str_ptr += sprintf(children_read_dsc_str_ptr, "%d ",
				node->children_read_dsc->elements[i]);
		children_write_dsc_str_ptr += sprintf(children_write_dsc_str_ptr, "%d ",
				node->children_write_dsc->elements[i]);
	}

	char main_read_dsc_str[10];
	char main_write_dsc_str[10];
	int main_read_dsc[2]; // node will read from this pipe
	int main_write_dsc[2]; // node will write to this pipe

	int ret = pipe(main_read_dsc);
	if(ret == -1) syserr("Error in pipe");
	ret = pipe(main_write_dsc);
	if(ret == -1) syserr("Error in pipe");

	sprintf(main_read_dsc_str, "%d", main_read_dsc[0]);
	sprintf(main_write_dsc_str, "%d", main_write_dsc[1]);
	read_dsc[last_node] = main_write_dsc[0];
	write_dsc[last_node] = main_read_dsc[1];

	node->internal_id = last_node;
	if(node->type == VARIABLE && node->value == 0) { // root node
		root_id = node->internal_id;
	}

	int pid;
	switch(pid = fork()) {
		case -1:
			syserr("Error in fork\n");
		case 0:
			for(int i = 0; i <= last_node; i++) {
				ret = close(read_dsc[i]);
				ret = close(write_dsc[i]);
				if(ret == -1) syserr("Error in close");
			}
			execl("./node_process", "node_process",
					type_str, param,
					children_read_dsc_str, children_write_dsc_str,
					parents_read_dsc_str, parents_write_dsc_str,
					main_read_dsc_str, main_write_dsc_str, NULL);
			syserr("Error in execl\n");
		default:
			ret = close(main_read_dsc[0]);
			ret = close(main_write_dsc[1]);
			if(ret == -1) syserr("Error in close");
			node_pid[last_node] = pid;

			return last_node++;
	}
}

static void close_uneeded_pipes(struct node *source) {
	int ret;
	for(int i = 0; i < 2; i++) {
		if(source->parent_read_dsc[i] != -1) {
			ret = close(source->parent_read_dsc[i]);
			ret = close(source->parent_write_dsc[i]);
			if(ret == -1) syserr("Error in close");
		}
	}

}

// Send values to variables and wait for ACKNOWLEDGE response
static void send_variables(int task_number, struct int_vector *variables, struct int_vector *values) {
	int ret;
	for(int i = 0; i < variables->size; i++) {
		int message_type = SET_VALUE;
		int value = values->elements[i];
		int dsc = write_dsc[variables->elements[i]];
		ret = write(dsc, &message_type, sizeof(message_type));
		if(ret == -1) syserr("Error in write");
		ret = write(dsc, &task_number, sizeof(task_number));
		if(ret == -1) syserr("Error in write");
		ret = write(dsc, &value, sizeof(value));
		if(ret == -1) syserr("Error in write");

		int return_message_type = -1;
		while(return_message_type != ACKNOWLEGDE) {
			// We received some result while waiting for ACKNOWLEDGE and have to process it
			ret = read(read_dsc[variables->elements[i]], &return_message_type, sizeof(return_message_type));
			if(ret == -1) syserr("Error in read");

			if(return_message_type == RESULT) {
				int return_task_number, return_value;

				ret = read(read_dsc[root_id], &return_task_number, sizeof(return_task_number));
				if(ret == -1) syserr("Error in read");
				ret = read(read_dsc[root_id], &return_value, sizeof(return_value));
				if(ret == -1) syserr("Error in read");

				received_results++;
				if(return_task_number > 0) {
					printf("%d P %d\n", return_task_number, return_value);
				} else {
					printf("%d F\n", -return_task_number);
				}

			}
		}
	}
}

