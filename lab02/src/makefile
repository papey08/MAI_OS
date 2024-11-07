n=

all: m f
m:
	gcc -g main.c -o sol.out
f:
	gcc -g main_fork.c -o solf.out
o:
	./generate.sh
	./sol.out $n
of:
	./generate.sh
	./solf.out $n	
c:
	rm -f *.out