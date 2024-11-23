main.out: main.c child.out
	gcc main.c -o main.out -lrt
child.out: child.c
	gcc child.c -o child.out -lrt
clean:
	rm -f *.out