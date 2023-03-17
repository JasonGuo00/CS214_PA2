all: go

clean:
	rm -rf mysh

array: arraylist.c 
	gcc -Wall -Werror -g -fsanitize=address -std=c11 -o arraylist.o -c arraylist.c

go: mysh.c 
	make clean
	gcc readwrite.c -o rw | gcc printargs.c -o printargs
	gcc -Wall -Werror -g -fsanitize=address -std=c11 mysh.c arraylist.o -o mysh