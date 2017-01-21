#include <stdlib.h>
#include <string.h>
#include "vector.h"

#define INIT_CAPACITY 8

void push_back(struct vector *vector, void *el) {
	if(vector->size == vector->capacity) {
		void **new_elements = malloc(2 * vector->capacity * sizeof(vector->elements));
		memcpy(new_elements, vector->elements, vector->size * sizeof(vector->elements));
		free(vector->elements);
		vector->elements = new_elements;
		vector->capacity = 2 * vector->capacity;
	}

	vector->elements[vector->size++] = el;
}

struct vector *create_vector() {
	struct vector *res = malloc(sizeof(struct vector));

	res->elements = malloc(INIT_CAPACITY * sizeof(*(res->elements)));
	res->capacity = INIT_CAPACITY;
	res->size = 0;

	return res;
}

void free_vector(struct vector *vector) {
	free(vector->elements);
	free(vector);
}
