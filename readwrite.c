#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char* argv[]){
    /*printf("--------\n");
    printf("args:\n");
    for (int i = 0; i < argc; i++){
        printf("arg %d = %s\n", i, argv[i]);
        int j = 0;
        for (j = 0; argv[i][j] != 0; j++){}
        write(STDOUT_FILENO, argv[i], j);
        write(STDOUT_FILENO, "\n", 1);
    }
    printf("--------\n");

    printf("input:\n");*/

    int buffer_size = 1024;
    char buffer[buffer_size];
    int bytes = read(STDIN_FILENO, buffer, buffer_size);
    if (bytes < 0){
        perror("read");
        return 1;
    }
    else if (bytes < buffer_size){
        buffer[bytes] = 0;
        printf("%s", buffer);
    }
    else{
        printf("Input too big\n");
    }

    //printf("\n--------");

    return 0;
}