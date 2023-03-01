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

int searchFile(char* file) {
    struct stat block;
    char* path = malloc(128);
    // Check /usr/local/sbin
    strcpy(path, "/usr/local/sbin");
    strcat(path, file);
    if(stat(file, &block) != -1) {
        return 0;
    }
    // Check /usr/local/bin
    strcpy(path, "/usr/local/bin");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /usr/sbin
    strcpy(path, "/usr/sbin");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /usr/bin
    strcpy(path, "/usr/bin");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /sbin
    strcpy(path, "/sbin");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /bin
    strcpy(path, "/bin");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    perror("Error: ");
    return 1;
}

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

char** tokenize(char* input, int* numTokens){
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
            int prevIsntWhitespace = 1;
            int j = i;
            while (input[i] != ' ' && input[i] != '\0' && input[i] != '\n'){
                if (input[i] == '<' || input[i] == '>' || input[i] == '|'){
                    prevIsntWhitespace = !(input[i-1] == ' ' && i > -1 && !(input[i-1] == '<' || input[i-1] == '>' || input[i-1] == '|'));
                    if (num_tokens == max_tokens-1-prevIsntWhitespace){
                        //More than "max_tokens"(-1) tokens in input, allow for more 
                        max_tokens *= 2;
                        token_arr = realloc(token_arr, max_tokens);
                    }
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
            if (input[i] == '\0' || input[i] == '\n'){
                //Avoid redundant terminator character or new line character
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
            //Add terminator character -> if line ends with a \n, then replace it with the null terminator
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
    *numTokens = num_tokens;
    return token_arr;
}

void interpreter(char** tokens, int numTokens) {
    if(strcmp(tokens[0],"cd") == 0 && numTokens == 2) {
        cd(tokens[1]);
    }
    if(strcmp(tokens[0],"pwd") == 0 && numTokens == 1) {
        pwd();
    }
    if(strcmp(tokens[0],"search") == 0 && numTokens == 2) {
        searchFile(tokens[1]);
    }
    free(tokens);
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
            // Exit Condition
            if(strcmp(lineBuffer, "exit\n") == 0) {
                close(file);
                free(buffer);
                free(lineBuffer);
                return 0;
            }
            // Interpret Line
            int numTokens;
            char** tokens = tokenize(lineBuffer, &numTokens);
            interpreter(tokens, numTokens);
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
                    // Exit condition
                    if(strcmp(lineBuffer, "exit\n") == 0) {
                        close(file);
                        free(buffer);
                        free(lineBuffer);
                        return 0;
                    }
                    // Do something with lineBuffer here
                    int numTokens;
                    char** tokens = tokenize(lineBuffer, &numTokens);
                    interpreter(tokens, numTokens);
                    // Set the starting position of the next line in the buffer
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