#ifndef VECTOR_H
#define VECTOR_H

struct vector {
	void **elements;
	int size;
	int capacity;
};

void push_back(struct vector *vector, void* el);

struct vector *create_vector();

void free_vector(struct vector *vector);

#endif //VECTOR_H
