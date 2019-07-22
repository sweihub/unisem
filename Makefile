unisem: unisem.c
	gcc -Wall -std=gnu99 -o unisem unisem.c -lpthread
love: unisem
	./unisem
clean:
	rm -f ./unisem

