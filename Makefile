all: go

clean:
	rm -rf mysh

array: arraylist.c 
	rm -rf arraylist.o
	gcc -Wall -Werror -fsanitize=address -std=c11 -o arraylist.o -c arraylist.c

go: mysh.c 
	make clean
	gcc -Wall -Werror -fsanitize=address -std=c11 mysh.c arraylist.o -o mysh