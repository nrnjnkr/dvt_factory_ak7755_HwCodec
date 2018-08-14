#Makefile for building factory tests tool in Linux.
#For windows need to modify the source code as header files for socket are
# different 
FLAGS = -I./ -lpthread -lm

SRC = main.c tests.c factory_tests.c

all:
	cc $(SRC) $(FLAGS) -o factory_butterfleye
