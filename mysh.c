#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUF_SIZE 4096


char* lineBuffer;
int lineLength;

int cd(char* path) {
    // Check if path is accessible
    struct stat block;
    if(stat(path, &block) == -1) {
        return -1;
    }
    // Change directory
    else {
      return chdir(path);  
    }
}

void pwd() {
    // Change the size of the buffer until it can encapsulate the path
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
    // Find the size of the path
    int pathSize = 0;
    for(int i = 0; i < size; i++) {
        pathSize++;
        if(buffer[i] == '\0') {
            break;
        }
    }
    // Some polishing
    buffer[pathSize] = '\n';
    buffer[pathSize+1] = '\0';
    // Return path, free buffer
    write(1, buffer, pathSize+1);
    free(buffer);
}

char** tokenize(char* input){
    size_t max_tokens = 8;
    char** token_arr = malloc(sizeof(char*) * max_tokens);
    int num_tokens = 0;
    char scanningWhitespace = 0;
    int i = 0;
    while (input[i] != '\0'){
        if (scanningWhitespace){
            //No whitespace detected here: start scanning this token
            scanningWhitespace = 0;
            //Count token length
            int prevIsntWhitespace = 0;
            int j = i;
            while (input[i] != ' ' && input[i] != "\0"){
                if (input[i] == '<' || input[i] == ">" || input[i] == "|"){
                    if (num_tokens == max_tokens-2){
                        //More than "max_tokens" tokens in input, allow for more 
                        max_tokens *= 2;
                        token_arr = realloc(token_arr, max_tokens);
                    }
                    int prevIsntWhitespace = !(input[i-1] == ' ' && i > -1 && !(input[i-1] == '<' || input[i-1] == ">" || input[i-1] == "|"));
                    token_arr[num_tokens+prevIsntWhitespace] = malloc(2);
                    memcpy(token_arr[num_tokens+1], &(input[i]), 1);
                    token_arr[num_tokens+prevIsntWhitespace][2] = '\0';
                    i--;
                    break;
                }
                i++;
            }
            if (!prevIsntWhitespace){
                i+=2;
                continue;
            }
            if (input[i] == '\0'){
                //Avoid redundant terminator character
                i--;
            }
            if (num_tokens == max_tokens-1){
                //More than "max_tokens" tokens in input, allow for more 
                max_tokens *= 2;
                token_arr = realloc(token_arr, max_tokens);
            }
            
            token_arr[num_tokens] = malloc(sizeof(char) * (i - j + 1));
            //Copy the characters
            memcpy(token_arr[num_tokens], &(input[j]), i - j + 1);
            //Add terminator character
            token_arr[num_tokens][i - j + 1] = '\0';

            num_tokens++;
        }
        else{
            //Move past whitespace
            scanningWhitespace = 1;
            while (input[i] == ' '){
                i++;
            }
        }
    }
    return token_arr;
}

int main(int argc, char* argv[]) {
    int file, bytes;
    char* buffer = malloc(sizeof(char)*BUF_SIZE);
    lineBuffer = malloc(sizeof(char)*BUF_SIZE);

    // Check whether we're in batch mode, interactive mode, or too many arguments were passed
    if(argc == 1) {
        //interactive mode
        printf("Welcome to my shell!\n");
        // input loop
        file = 0;
        write(1, "mysh> ", 7);
        while((bytes = read(file, buffer, BUF_SIZE)) > 0) {
            memcpy(lineBuffer, buffer, bytes);
            lineBuffer[bytes] = '\0';
            // Do something with lineBuffer here

            // Exit command for testing
            if(strcmp(lineBuffer, "exit\n") == 0) {
                close(file);
                free(buffer);
                free(lineBuffer);
                return EXIT_SUCCESS;
            }
            else if(strcmp(lineBuffer, "pwd\n") == 0) {
                pwd();
            }
            write(1, "mysh> ", 7);
        }
    }
    else if(argc == 2) {
        //batch mode
        file = open(argv[1], O_RDONLY);
        int i;
        int lineStart = 0;
        while((bytes = read(file, buffer, BUF_SIZE)) > 0) {
            for(i = 0; i < bytes; i++) {
                if(buffer[i] == '\n') {
                    lineLength = i - lineStart + 1;
                    memcpy(lineBuffer, buffer + lineStart, bytes);
                    lineBuffer[lineLength] = '\0';
                    // Do something with lineBuffer here
                    // Exit command for testing
                    if(strcmp(lineBuffer, "exit\n") == 0) {
                        close(file);
                        free(buffer);
                        free(lineBuffer);
                        return EXIT_SUCCESS;
                    }
                    else if(strcmp(lineBuffer, "pwd\n") == 0) {
                        pwd();
                    }
                    lineStart = i+1;
                }
            }
        }
        // If the last character in the file wasn't a '/n', then we have a partial command: just ignore it.
    }
    else {
        printf("Error: Too many arguments.");
    }
}