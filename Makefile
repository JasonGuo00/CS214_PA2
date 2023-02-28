all: go

clean:
	rm -rf mysh

go: mysh.c 
	make clean
	gcc -Wall -Werror -fsanitize=address -std=c11 mysh.c -o mysh