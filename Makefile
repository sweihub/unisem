unisem: unisem.c
	gcc -Wall -o unisem unisem.c -lpthread
love: unisem
	./unisem
