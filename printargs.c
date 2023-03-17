#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char* argv[]){
    for (int i = 0; i < argc; i++){
        printf("arg %d = %s\n", i, argv[i]);
    }

    return 0;
}