#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "global.h"
#include "node.h"
#include "vector.h"

#define LINE_MAX 1000

static struct node *try_read_expresion();

static struct node *try_read_pnum();

static struct node *try_read_var();

static struct node *try_read_negation();

static struct node *try_read_binary();

static bool try_read_number();

static bool try_read_string(const char *s);

static bool try_read_char(char c);

static char*line;

static int pos;

struct node *read_expresion() {
	struct node *result = NULL;
	pos = 0;
	line = malloc(LINE_MAX *sizeof(int));
	fgets(line, LINE_MAX, stdin);

	result = try_read_expresion();

	free(line);
	return result;
}

static struct node *try_read_expresion() {
	struct node *result;
	struct node *ret;

	result = NULL;
	if((ret = try_read_pnum()) != NULL) {
		result = ret;
	} else if((ret = try_read_var()) != NULL) {
		result = ret;
	} else if((ret = try_read_negation()) != NULL) {
		result = ret;
	} else if((ret = try_read_binary()) != NULL) {
		result = ret;
	}

	return result;
}

static struct node *try_read_pnum() {
	long number;
	struct node *result = NULL;
	if(try_read_number(&number)) {
		result = create_node(NUMBER);
		result->value = number;
	}
	return result;
}

static struct node *try_read_var() {
	long n;
	int start_pos = pos;
	bool found = true;
	struct node *result = NULL;

	if(!try_read_string("x[")) {
		found = false;
	} else if(!try_read_number(&n)) {
		found = false;
	} else if(!try_read_string("]")) {
		found = false;
	}

	if(found) {
		result = create_node(VARIABLE);
		result->value = n;
	} else {
		pos = start_pos;
	}
	return result;
}

static struct node *try_read_negation() {
	int start_pos = pos;
	bool found = true;
	struct node *expression;
	struct node *result = NULL;

	if(!try_read_string("(-")) {
		found = false;
	} else if((expression = try_read_expresion()) == NULL) {
		found = false;
	} else if(!try_read_string(")")) {
		found = false;
	}

	if(found) {
		result = create_node(OPERATOR);
		result->operation = '-';
		result->parents[0] = expression;
		push_back(expression->children, result);
	} else {
		pos = start_pos;
	}
	return result;
}

static struct node *try_read_binary() {
	int start_pos = pos;
	bool found = true;
	struct node *result = NULL;
	struct node *lhs, *rhs;
	char operation;

	if(!try_read_char('(')) {
		found = false;
	} else if((lhs = try_read_expresion()) == NULL) {
		found = false;
	}

	if(try_read_string("+")) {
		operation = '+';
	} else if(try_read_string("*")) {
		operation = '*';
	} else {
		found = false;
	}

	if(found) {
		if((rhs = try_read_expresion()) == NULL) {
			found = false;
		} else if(!try_read_char(')')) {
			found = false;
		}
	}

	if(found) {
		result = create_node(OPERATOR);
		result->operation = operation;
		result->parents[0] = lhs;
		result->parents[1] = rhs;
		push_back(lhs->children, result);
		push_back(rhs->children, result);
	} else {
		pos = start_pos;
	}
	return result;
}

static bool try_read_string(const char *s) {
	int start_pos = pos;

	while(*s != '\0') {
		if(!try_read_char(*s)) {
			pos = start_pos;
			return false;
		}
		s++;
	}

	return true;
}

static bool try_read_char(char c) {
	int start_pos = pos;
	while(isspace(line[pos])) {
		pos++;
	}
	if(line[pos] == c) {
		pos++;
		return true;
	}
	pos = start_pos;
	return false;
}


static bool try_read_number(long *res) {
	long number;
	int len;
	int ret = sscanf(line + pos, "%ld%n", &number, &len);
	if(ret == 0) {
		return false;
	}
	pos += len;
	*res = number;
	return true;
}
