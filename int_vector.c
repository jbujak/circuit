#include <stdlib.h>
#include <string.h>
#include "int_vector.h"

#define INIT_CAPACITY 8

void push_back_int(struct int_vector *vector, int el) {
	if(vector->size == vector->capacity) {
		int *new_elements = malloc(2 * vector->capacity * sizeof(vector->elements));
		memcpy(new_elements, vector->elements, vector->size * sizeof(vector->elements));
		free(vector->elements);
		vector->elements = new_elements;
		vector->capacity = 2 * vector->capacity;
	}

	vector->elements[vector->size++] = el;
}

struct int_vector *create_int_vector() {
	struct int_vector *res = malloc(sizeof(struct int_vector));

	res->elements = malloc(INIT_CAPACITY * sizeof(*(res->elements)));
	res->capacity = INIT_CAPACITY;
	res->size = 0;

	return res;
}

void free_int_vector(struct int_vector *vector) {
	free(vector->elements);
	free(vector);
}
