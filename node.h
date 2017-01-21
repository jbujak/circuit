#ifndef NODE_H
#define NODE_H

struct node {
	int type;
	int internal_id;
	struct vector *children;
	struct node *parents[2];

	int main_process_read_dsc;
	int parent_read_dsc[2];
	int parent_write_dsc[2];
	struct int_vector *children_read_dsc;
	struct int_vector *children_write_dsc;

	// For number and variable nodes
	int value;

	// For operator nodes
	char operation;
};

struct node *create_node(int type);

void free_node(struct node *node);

#endif //NODE_H
