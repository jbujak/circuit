circuit: circuit.c node.o int_vector.o vector.o parser.o main_process.o err.o node_process
	gcc -g -Wall -Wextra node.o int_vector.o vector.o parser.o main_process.o err.o circuit.c
	ctags -R .

node_process: node_process.c err.o int_vector.o
	gcc -g -Wall -Wextra int_vector.o err.o node_process.c -o node_process

%.o: %.c %.h
	gcc -g -c -Wall -Wextra $*.c

clean:
	-rm -f *.o
	-rm -f node_process circuit
