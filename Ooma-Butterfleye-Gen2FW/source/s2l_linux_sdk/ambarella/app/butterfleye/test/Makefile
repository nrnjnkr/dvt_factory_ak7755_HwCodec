DEBUG ?= 0

app: ../app/*.c ../utils/*.c
	gcc -o $@ $^ -I../include -lpthread -DDEBUG=$(DEBUG)
