all: chat.c error.h chat1.c
	gcc -g -Wall chat.c -o chat
	gcc -g -Wall chat1.c -o chat1