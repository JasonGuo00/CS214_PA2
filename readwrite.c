#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char* argv[]){
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
        printf("input too big\n");
    }

    return 0;
}