#ifndef INT_VECTOR_H
#define INT_VECTOR_H

struct int_vector {
	int *elements;
	int size;
	int capacity;
};

void push_back_int(struct int_vector *vector, int el);

struct int_vector *create_int_vector();

void free_int_vector(struct int_vector *vector);

#endif //INT_VECTOR_H
