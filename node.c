#include <stdlib.h>
#include <stdbool.h>
#include "node.h"
#include "vector.h"
#include "int_vector.h"
#include "global.h"

struct node *create_node(int type) {
	struct node *res = malloc(sizeof(struct node));

	res->type = type;
	res->children = create_vector();
	res->internal_id = -1;
	res->parents[0] = NULL;
	res->parents[1] = NULL;
	res->main_process_read_dsc = -1;
	res->parent_read_dsc[0] = -1;
	res->parent_read_dsc[1] = -1;
	res->parent_write_dsc[0] = -1;
	res->parent_write_dsc[1] = -1;
	res->children_read_dsc = create_int_vector();
	res->children_write_dsc = create_int_vector();

	return res;
}

void free_node(struct node *node) {
	free_vector(node->children);
	free_int_vector(node->children_read_dsc);
	free_int_vector(node->children_write_dsc);
	free(node);
}
