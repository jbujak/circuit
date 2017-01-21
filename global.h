#ifndef GLOBAL_H
#define GLOBAL_H

// Node types
#define NUMBER 1
#define VARIABLE 2
#define OPERATOR 3

// Message types

// CALCULATE task_number(int)
#define CALCULATE 1

// RESULT task_number(int) value(int)
// negative task_number means undefined result
#define RESULT 2

// SET_VALUE task_number(int) value(int)
#define SET_VALUE 3

// ACKNOWLEDGE
#define ACKNOWLEGDE 4

#endif //GLOBAL_H
