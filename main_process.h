#ifndef MAIN_PROCESS_H
#define MAIN_PROCESS_H

#include <sys/types.h>
#include "node.h"

int create_number_node(int value);

int create_variable_node();

int create_operator_node(char operation);

void create_processes(struct node *source);

void read_remaining_results(int expected_results);

void join();

void calculate_with_initialization(int task_number, struct int_vector *variables, struct int_vector *values);

#endif // MAIN_PROCESS_H
