partioner: main.o graph_partion.o
	cc -o partitioner graph_partion.o main.o -lmetis

main.o: main.c
	cc -c main.c graph_partion.o

graph_partion.o: graph_partion.c
	cc -c graph_partion.c

clean:
	rm -f part* *.o
