#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int cd(char* path) {
    return chdir(path);
}

void pwd() {
    int size = 100, complete = 0;
    char* buffer = malloc(sizeof(char)*100);
    while (!complete) {
        if(getcwd(buffer, size-1) != NULL) {
            complete = 1;
        }
        else {
            size = size * 2;
            buffer = (char*)realloc(buffer, size);
        }
    }
    printf("%s\n", buffer);
}

int main(int argc, char* argv[]) {
    if(argc == 1) {
        //interactive mode
        printf("Welcome to my shell!\n");
        int exit = 0;
        while(!exit) {
            printf("mysh> ");
        }
    }
    else if(argc == 2) {
        //batch mode
    }
    else {
        printf("Error: Too many arguments.");
    }
    printf("%d\n", argc);
}