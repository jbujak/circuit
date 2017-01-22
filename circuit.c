#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include "parser.h"
#include "main_process.h"
#include "node.h"
#include "vector.h"
#include "int_vector.h"
#include "global.h"

#define VARIABLES_MAX 1000
#define NODES_MAX 5000

static int read_definitions();

static bool is_dependent(struct node *expresion, int variable);

static void add_nodes(struct node *expresion);

static void merge_expresions();

static void process_queries();

static void free_nodes(struct node *source);


struct node *definition[VARIABLES_MAX];

struct vector *nodes[VARIABLES_MAX];

static int N, K, V;

static bool root_created = false;

int main() {
	int ret;

	scanf("%d%d%d\n", &N, &K, &V);

	for(int i = 0; i < V; i++) {
		nodes[i] = create_vector();
	}

	ret = read_definitions();
	if(ret == -1) {
		return 0;
	}

	struct node *root = create_node(VARIABLE);
	root->value = 0;
	push_back(nodes[0], root);

	merge_expresions();
	create_processes(root);
	process_queries();
	join();

	for(int i = 0; i < V; i++) {
		free_vector(nodes[i]);
	}
	free_nodes(root);
	
	return 0;
}

static int read_definitions() {
	int ret;
	for(int i = 0; i < K; i++) {
		int line_num, variable;
		ret = scanf("%d x[%d] = ", &line_num, &variable);
		struct node *expr = read_expresion();
		if(ret < 2 || definition[variable] != NULL || expr == NULL) {
			printf("%d F\n", line_num);
			return -1;
		}
		if(variable == 0) {
			root_created = true;
		}

		definition[variable] = expr;
		if(is_dependent(definition[variable], variable)) {
			printf("%d F\n", line_num);
			return -1;
		}

		printf("%d P\n", line_num);
		add_nodes(expr);
	}
	return 0;
}

static void merge_expresions() {
	for(int i = 0; i < V; i++) {
		if(definition[i] != NULL) {
			for(int j = 0; j < nodes[i]->size; j++) {
				struct node *current = ((struct node*)(nodes[i]->elements[j]));
				current->parents[0] = definition[i];
				push_back(definition[i]->children, current);
			}
		}
	}
}

static void process_queries() {
	int expected_results = N - K;
	for(int i =  K; i < N; i++) {
		int task_number;
		int variable, value;
		struct int_vector *variables = create_int_vector();
		struct int_vector *values = create_int_vector();
		scanf("%d", &task_number);
		while(scanf(" x[%d] %d", &variable, &value) == 2) {
			for(int i = 0; i < nodes[variable]->size; i++) {
				struct node* variable_node = (struct node*)(nodes[variable]->elements[i]);
				push_back_int(variables, variable_node->internal_id);
				push_back_int(values, value);
			}
		}
		if(root_created) {
			calculate_with_initialization(task_number, variables, values);
		} else {
			printf("%d F\n", task_number);
			expected_results--;
		}
		free_int_vector(variables);
		free_int_vector(values);
	}

	read_remaining_results(expected_results);
}


static void add_nodes(struct node *expresion) {
	if(expresion->type == VARIABLE) {
		push_back(nodes[expresion->value], expresion);
	} else {
		if(expresion->parents[0] != NULL) {
			add_nodes(expresion->parents[0]);
		}
		if(expresion->parents[1] != NULL) {
			add_nodes(expresion->parents[1]);
		}
	}
}

static void free_nodes(struct node *source) {
	if(source->parents[0] != NULL) {
		free_nodes(source->parents[0]);
	}
	if(source->parents[1] != NULL) {
		free_nodes(source->parents[1]);
	}
	for(int i = 0; i < source->children->size; i++) {
		struct node *child = (struct node*)(source->children->elements[i]);
		if(child->parents[0] == source) {
			child->parents[0] = NULL;
		}
		if(child->parents[1] == source) {
			child->parents[1] = NULL;
		}
	}
	free_node(source);
}

static bool is_dependent(struct node *expresion, int variable) {
	if(expresion == NULL) {
		return false;
	}
	if(expresion->type == VARIABLE && expresion->value == variable) {
		return true;
	}
	if(expresion->parents[0] != NULL && is_dependent(expresion->parents[0], variable)) {
		return true;
	}
	if(expresion->parents[1] != NULL && is_dependent(expresion->parents[1], variable)) {
		return true;
	}
	if(expresion->type == VARIABLE && is_dependent(definition[expresion->value], variable)) {
		return true;
	}
	return false;
}
