all: lets-talk

lets-talk: lets-talk.o 
	gcc -pthread -o lets-talk lets-talk.c -g

# lets-talk.o: lets-talk.c
# 	gcc -g -o lets-talk.o lets-talk.c


# list.o: list.c
# 	gcc -g -c list.c

valgrind: lets-talk
	valgrind --leak-check=full ./lets-talk

clean:
	rm lets-talk
	rm *.o