#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include "arraylist.h"

#define BUF_SIZE 4096


char* lineBuffer;
int lineLength;
int lastExit = 0;

int searchPath(char* path) {
    struct stat block;
    if(stat(path, &block) == -1) {
        // File DNE, or unaccessible
        return 0;
    }
    // File exists
    return 1;
}

char* obtainParent(char* path) {
    int i = 0, lastSlash;
    while(path[i] != '\0') {
        if(path[i] == '/') {lastSlash = i;}
    }
    char* parentPath = malloc(lastSlash + 1);
    for(i = 0; i < lastSlash; i++) {
        parentPath[i] = path[i];
    }
    return parentPath;
}

int searchFile(char* file) {
    struct stat block;
    char* path = malloc(128);
    lastExit = 0;
    // Check /usr/local/sbin
    strcpy(path, "/usr/local/sbin/");
    strcat(path, file);
    if(stat(file, &block) != -1) {
        return 0;
    }
    // Check /usr/local/bin
    strcpy(path, "/usr/local/bin/");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /usr/sbin
    strcpy(path, "/usr/sbin/");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /usr/bin
    strcpy(path, "/usr/bin/");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /sbin
    strcpy(path, "/sbin/");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    // Check /bin
    strcpy(path, "/bin/");
    strcat(path, file);
    if(stat(file, &block) != -1){
        return 0;
    }
    perror("Error: ");
    lastExit = 1;
    return 1;
}

int cd(char* path) {
    int ret = chdir(path);
    if(ret == -1) {
        lastExit = 1;
    }
    perror("Error: ");
    return ret;
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

int isWild(char* token, int* asterisk, int* totalChars) {
    int i = 0, isWild = 0;
    while(token[i] != '\0') {
        if(token[i] == '*') {
            isWild = 1;
            *asterisk = i;
        }
    }
    *totalChars = i;
    return isWild;
}

int identifyWild(char* fileName, char* pattern, int asterisk, int totalChars) {
    int num_front = asterisk;
    int num_end =  totalChars - asterisk - 1;
    printf("%d %d\n", num_front, num_end);
    int fileNameSize = 0;
    while(fileName[fileNameSize] != '\0') {fileNameSize++;}
    printf("%d\n", fileNameSize);
    if(fileNameSize < num_front+num_end) {
        return 0;
    }
    else {
        if(num_front > 0) {
            for(int i = 0; i < num_front; i++) {
            if(fileName[i] != pattern[i]) {
                return 0;
            }
            }
        }
        if(num_end > 0) {
            int temp = asterisk+1;
            for(int i = fileNameSize - num_end; i < fileNameSize; i++) {
                if(fileName[i] != pattern[temp]) {
                    return 0;
                }
                temp++;
            }
        }
        return 1;
    }
}

char **tokenize(char *input, int *numTokens)
{
    size_t max_tokens = 8;
    char **token_arr = malloc(sizeof(char *) * max_tokens);
    int num_tokens = 0;
    char scanningWhitespace = 0;
    int i = 0;
    while (input[i] != '\0')
    {
        // printf("scanning whitespace: %d, i = %d\n", scanningWhitespace, i);

        if (scanningWhitespace)
        {
            // No whitespace detected here: start scanning this token
            scanningWhitespace = 0;
            // Count token length
            int j = i;
            char isSpecial = input[i] == '<' || input[i] == '>' || input[i] == '|' || input[i] == ' ';
            if (!isSpecial)
            {
                while (input[i] != '\0')
                {
                    i++;
                    isSpecial = input[i] == '<' || input[i] == '>' || input[i] == '|' || input[i] == ' ';
                    if (isSpecial)
                    {
                        if (input[i - 1] == '\\')
                        {
                            if (i > 1)
                            {
                                if (input[i - 2] == '\\')
                                {
                                    break;
                                }
                            }
                            i++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            if (input[i] == '\0')
            {
                // Avoid redundant terminator character or new line character
                i--;
            }
            if (i == j)
            {
                i++;
                break;
            }
            if (num_tokens == max_tokens - 1)
            {
                // More than "max_tokens" tokens in input, allow for more
                max_tokens *= 2;
                token_arr = (char **)realloc(token_arr, sizeof(char *) * max_tokens);
            }
            int str_size = i - j + 1;
            if (input[i] == '\n')
            {
                i++;
            }
            token_arr[num_tokens] = malloc(sizeof(char) * (str_size));
            // Copy the characters
            memcpy(token_arr[num_tokens], &(input[j]), str_size);
            // Add terminator character
            token_arr[num_tokens][str_size - 1] = '\0';

            // printf("%s\n", token_arr[num_tokens]);
            for (int x = 0; x < str_size; x++)
            {
                if (token_arr[num_tokens][x] == '\\')
                {
                    char isNewline = token_arr[num_tokens][x + 1] == 'n';
                    for (int y = x; y < str_size - 1 - isNewline; y++)
                    {
                        token_arr[num_tokens][y] = token_arr[num_tokens][y + 1 + isNewline];
                    }
                }
            }

            num_tokens++;
        }
        else
        {
            // Move past whitespace
            scanningWhitespace = 1;
            char isSpecial = input[i] == '<' || input[i] == '>' || input[i] == '|';
            while (input[i] == ' ' || isSpecial)
            {
                if (isSpecial)
                {
                    if (num_tokens == max_tokens - 1)
                    {
                        // More than "max_tokens" tokens in input, allow for more
                        max_tokens *= 2;
                        token_arr = (char **)realloc(token_arr, sizeof(char *) * max_tokens);
                    }
                    token_arr[num_tokens] = malloc(sizeof(char) * 2);
                    token_arr[num_tokens][0] = input[i];
                    token_arr[num_tokens][1] = '\0';
                    printf("%s\n", token_arr[num_tokens]);
                    num_tokens++;
                }
                i++;
                isSpecial = input[i] == '<' || input[i] == '>' || input[i] == '|';
            }
        }
    }
    *numTokens = num_tokens;
    return token_arr;
}

void executeProgram(char** tokens, int numTokens) {
    int pipe(int[2]);

    // Obtain the path of the directory encapsulating the program
    char* parentPath = obtainParent(tokens[0]);
    // Open the directory 
    DIR* directory = opendir(parentPath);
    struct dirent *dir;
    // Main loop
    for(int i = 1; i < numTokens; i++) {
        // Case: Wildcards
        int asterisk = -1, totalChars = -1;
        if(isWild(tokens[i], &asterisk, &totalChars)) {
            // Read through directory to find files that fit the wildcard pattern
            while((dir = readdir(directory)) != NULL) {
                if(identifyWild(dir->d_name, tokens[i], asterisk, totalChars)) {
                    // found a file that will be passed as an argument
                    // pass the file name in as an argument
                }
            }
        }
        // Case: Redirection < (input)
        else if(tokens[i] == '<') {
            // Take tokens[i+1] as the input of the pipe

        }
        // Case: Redirection > (output)
        else if(tokens[i] == '>') {
            // Take tokens[i+1] as the output of the pipe
        }
        // Case: Sub Command
        else if(tokens[i] == '|') {
            // Everything to the right of the bar is a subcommand

        }
        // Case: Default, token is read as a argument
        else {
            // Pass token as a argument
        }
    }
    free(parentPath);
}

void interpreter(char** tokens, int numTokens) {
    // Path mode
    if(tokens[0][0] == '/') {
        // Path mode shit here ...
    }
    // Built in Commands Mode
    if(strcmp(tokens[0],"cd") == 0 && numTokens == 2) {
        cd(tokens[1]);
    }
    if(strcmp(tokens[0],"pwd") == 0 && numTokens == 1) {
        pwd();
    }
    if(strcmp(tokens[0],"search") == 0 && numTokens == 2) {
        searchFile(tokens[1]);
    }
    // Neither Path nor Built-in Command
    searchFile(tokens[0]);
    if(lastExit == 0) {     // Means that a file with the given name has been found
        // Execute the file with the given name
    }

    // Free the tokenized line
    for(int i = 0; i < numTokens; i++) {
        free(tokens[i]);
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
                write(1, "mysh> exiting...\n", 17);
                close(file);
                free(buffer);
                free(lineBuffer);
                return 0;
            }
            // Interpret Line
            int numTokens;
            char** tokens = tokenize(lineBuffer, &numTokens);
            interpreter(tokens, numTokens);
            // The terminal prompt: Preceded by ! if the last exit code was non zero / failed execution
            if(lastExit != 0) {
                write(1, "!mysh> ", 8);
            }
            else {
                write(1, "mysh> ", 7);
            }
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
                        write(1, "mysh> exiting...\n", 17);
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