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

typedef struct program{
    list_t* arguments;
    char* file;
    char* input;
    char* output;
} Program;

char* append(char* dest, char* toAppend) {
    int dest_length = 0, append_length = 0;
    while(dest[dest_length] != '\0') {dest_length++;}
    while(toAppend[append_length] != '\0') {append_length++;}
    dest = realloc(dest, dest_length + append_length + 1);
    strcat(dest, toAppend);
    return dest;
}

int searchPath(char* path) {
    struct stat block;
    if(stat(path, &block) == -1) {
        // File DNE, or unaccessible
        return 0;
    }
    // File exists
    return 1;
}

char* obtainParent(char* path, char** filename) {
    int i = 0, lastSlash;
    while(path[i] != '\0') {
        if(path[i] == '/') {lastSlash = i;}
        i++;
    }
    int totalLength = i;
    char* parentPath;
    if(lastSlash == 0) {
        parentPath = malloc(2);
    }
    else {
        parentPath = malloc(lastSlash+1);
    }
    *filename = malloc(i - lastSlash); 
    for(i = 0; i < lastSlash; i++) {
        parentPath[i] = path[i];
    }
    parentPath[i] = '\0';
    // Case: File is in the root's directory
    if(lastSlash == 0) {
        parentPath[0] = '/';
        parentPath[1] = '\0';
    }
    i = 0;
    for(int x = lastSlash+1; x < totalLength; x++) {
        // get the file name
        (*filename)[i] = path[x];
        i++;
    }
    (*filename)[i] = '\0';
    return parentPath;
}

char* searchFile(char* file) {
    struct stat block;
    char* path = malloc(32);
    lastExit = 0;
    // Check /usr/local/sbin
    strcpy(path, "/usr/local/sbin/");
    append(path, file);
    if(stat(path, &block) != -1) {
        return path;
    }
    // Check /usr/local/bin
    strcpy(path, "/usr/local/bin/");
    append(path, file);
    if(stat(path, &block) != -1){
        return path;
    }
    // Check /usr/sbin
    strcpy(path, "/usr/sbin/");
    append(path, file);
    if(stat(path, &block) != -1){
        return path;
    }
    // Check /usr/bin
    strcpy(path, "/usr/bin/");
    append(path, file);
    if(stat(path, &block) != -1){
        return path;
    }
    // Check /sbin
    strcpy(path, "/sbin/");
    append(path, file);
    if(stat(path, &block) != -1){
        return path;
    }
    // Check /bin
    strcpy(path, "/bin/");
    append(path, file);
    if(stat(path, &block) != -1){
        return path;
    }
    perror("Error: ");
    lastExit = 1;
    return NULL;
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
    printf("Inside isWild\n");
    while(token[i] != '\0') {
        printf("Iteration %d\n", i);
        if(token[i] == '*') {
            isWild = 1;
            *asterisk = i;
        }
        i++;
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

list_t *tokenize(char *input)
{
    if(strcmp(input, "\n") == 0) {
        return NULL;
    }    
    list_t* token_arr = malloc(sizeof(list_t));
    al_init(token_arr, 8);
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
                break;
            }
            int str_size = i - j + 1;
            if (input[i] == '\n')
            {
                i++;
            }
            char referencesHomeDir = 0;
            if (str_size > 1){
                //long enough to be a reference to the home directory
                if (input[j] == '~' && input[j+1] == '/'){
                    referencesHomeDir = 1;
                }
            }
            char* str;
            if (!referencesHomeDir){
                str = malloc(sizeof(char) * (str_size));
            }
            else{
                char* homeDir = getenv("HOME");
                int homeDirLength = 0;
                while (homeDir[homeDirLength] != '\0'){
                    homeDirLength++;
                }
                str = malloc(sizeof(char) * (str_size+homeDirLength));
                memcpy(str, homeDir, homeDirLength+1);
                referencesHomeDir = homeDirLength;
            }
            
            // Copy the characters
            memcpy(str+referencesHomeDir, &(input[j+(referencesHomeDir != 0)]), str_size);
            // Add terminator character
            str[str_size+referencesHomeDir - 1 - (referencesHomeDir != 0)] = '\0';
            // Push token into the array
            al_push(token_arr, str);

            for (int x = 0; x < str_size; x++)
            {
                if (token_arr->data[token_arr->size-1][x] == '\\')
                {
                    char isNewline = token_arr->data[token_arr->size-1][x + 1] == '\\' && token_arr->data[token_arr->size-1][x + 1] == 'n';
                    for (int y = x; y < str_size - 1 - isNewline; y++)
                    {
                        token_arr->data[token_arr->size-1][y] = token_arr->data[token_arr->size-1][y + 1 + isNewline];
                    }
                }
            }

           //printf("%s\n", str);
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
                    char* str2 = malloc(sizeof(char) * 2);
                    str2[0] = input[i];
                    str2[1] = '\0';
                    al_push(token_arr, str2);
                }
                i++;
                isSpecial = input[i] == '<' || input[i] == '>' || input[i] == '|';
            }
        }
    }
    /*for(int x = 0; x < token_arr->size; x++) {
        printf("%s\n", token_arr->data[x]);
    }*/
    return token_arr;
}

// int execute(char* executablePath, list_t* arguments) {
//     pid_t pid1, pid2;
//     int pipe(int fd[2]);

//     // Command 1
//     pid1 = fork();
//     if(pid1 < 0) {
//         perror("Error: failure in the fork for child 1");
//         return -1;
//     }
//     else if(pid1 == 0) {
//         // Child process 1: run the stuff
//         if(execv(executablePath, arguments[0]->data) < 0) {
//             perror("Error: ");
//             return -1;
//         }
//     }

//     // Command 2
//     if(executablePath[2] != NULL) {
//         pid2 = fork();
//         if(pid2 < 0) {
//             perror("Error: failure in the fork for child 2");
//             return -1;
//         }
//         else if(pid2 == 0) {
//             // Chid process 2: run the stuff
//             if(execv(executablePath, arguments[1]->data) < 0) {
//                 perror("Error: ");
//                 return -1;
//             }
//         }
//     }
//     return 1;
// }

Program** parseArguments(list_t* tokens) {
    Program** programList = malloc(sizeof(Program*));
    list_t* arguments = malloc(sizeof(list_t));
    al_init(arguments, tokens->size);
    Program* program = malloc(sizeof(struct program));
    printf("Checkpoint 1: Entered parseArguments\n");

    // First token is not a pathway
    if(tokens->data[0][0] != '/') {
        // Find the path and rename the token as a pathway, or return NULL if path to file not found
        char* name = searchFile(tokens->data[0]);
        if(name != NULL) {
            strcpy(tokens->data[0], name);
        }
        else {
            return NULL;
        }
    }
    // File name is taken
    program->file = tokens->data[0];
    // Obtain the path of the directory encapsulating the program
    char* programName;
    char* parentPath = obtainParent(tokens->data[0], &programName);
    printf("%s\n", program->file);
    printf("Checkpoint 2: Obtain Parent Complete: Parent Path: %s File Name: %s\n", parentPath, programName);
    // Program takes its own name as an argument
    al_push(arguments, programName);
    printf("Checkpoint 3: Pushed command name in as an argument\n");
    // Open the directory 
    DIR* directory = opendir(parentPath);
    if(directory == NULL) {
        perror("Error");
        return NULL;
    }
    struct dirent *dir;
    // Main loop
    for(int i = 1; i < tokens->size; i++) {
        printf("Checkpoint 4: Entered main loop\n");

        // Case: Wildcards
        int asterisk = -1, totalChars = -1;
        if(isWild(tokens->data[i], &asterisk, &totalChars) == 1) {
            // Read through directory to find files that fit the wildcard pattern
            write(1,"Special Checkpoing: Entered Wildcard Search\n",45);
            int foundWildCard = 0;
            while((dir = readdir(directory)) != NULL) {
                if(identifyWild(dir->d_name, tokens->data[i], asterisk, totalChars)) {
                    al_push(arguments, dir->d_name);
                    foundWildCard = 1;
                    // found a file that will be passed as an argument
                    // pass the file name in as an argument
                }
            }
            // If no file matching the wildcard is found, then pass it as an argument to the program
            if(!foundWildCard) {
                al_push(arguments, tokens->data[i]);
            }
        }

        // Case: Redirection < (input)
        else if(tokens->data[i][0] == '<') {
            // Bad case: there is no argument after this token
            if(i == tokens->size-1) {
                printf("Error: no file to set input as.");
                return NULL;
            }
            // Bad case: the argument after this token is invalid
            if(strcmp(tokens->data[i+1], "<") == 0 || strcmp(tokens->data[i+1], ">") == 0 || strcmp(tokens->data[i+1], "|") == 0) {
                printf("Error: argument after is invalid");
                return NULL;
            }
            program->input = tokens->data[i+1];
            i++;
        }
        // Case: Redirection > (output)
        else if(tokens->data[i][0] == '>') {
            // Bad case: there is no argument after this token
            if(i == tokens->size-1) {
                printf("Error: no file to set input as.");
                return NULL;
            }
            // Bad case: the argument after this token is invalid
            if(strcmp(tokens->data[i+1], "<") == 0 || strcmp(tokens->data[i+1], ">") == 0 || strcmp(tokens->data[i+1], "|") == 0) {
                printf("Error: argument after is invalid");
                return NULL;
            }
            program->output = tokens->data[i+1];
            i++;
        }
        // Case: Sub Command Piping (|)
        else if(tokens->data[i][0] == '|') {
            // Bad commands:
            if(i == tokens->size-1 || strcmp(tokens->data[i+1], "<") == 0 || strcmp(tokens->data[i+1], ">") == 0 || strcmp(tokens->data[i+1], "|") == 0) {
                printf("Error: argument after | is invalid");
                return NULL;
            }
            // Everything to the right of the bar is a subcommand -> recursion
            // Split the tokens at the point where the | occurs
            list_t* splitTokens = malloc(sizeof(list_t));
            al_init(splitTokens, tokens->size - i + 1);
            for(int z = i+1; z < tokens->size; z++) {
                splitTokens->data[z-i+-1] = tokens->data[z];
            }
            // Recursive call, get the returning list of commands (should only be one thing)
            Program** subcommand_list = parseArguments(splitTokens);
            if(subcommand_list == NULL) {
                // There is some issue with the subcommand, terminate
                printf("Error: Subcommand cannot be read.");
                return NULL;
            }
            else {
                // Subcommand's Program will be stored in the first element, read it in as the second element of the parent list
                programList[1] = subcommand_list[0];
                free(subcommand_list);
                // Subcommand marks the end of the line, recursion will read the remainder
                break;
            }
        }
        // Case: Default, token is read as a argument
        else {
            printf("Special Checkpoint: Adding argument to list\n");
            // Pass token as a argument
            al_push(arguments, tokens->data[i]);
        }
    }
    program->arguments = arguments;
    // al_destroy(arguments);
    // free(arguments);
    // free(parentPath);
    // free(programName);
    printf("END CHECKPOINT\n");
    programList[0] = program;
    return programList;
}

void interpreter(list_t* tokens) {
    // terminal ignores empty lines
    if(tokens == NULL) {
        return;
    }
    // Path mode
    if(tokens->data[0][0] == '/') {
        // Path mode shit here ...
        Program** programs = parseArguments(tokens);
        if(programs == NULL) {
            printf("Error: line is not executable");
            return;
        }
        printf("%s\n", programs[0]->file);
        for(int i = 0; i < programs[0]->arguments->size; i++) {
            printf("%s\n", programs[0]->arguments->data[i]);
        }
        return;
    }
    // Built in Commands Mode
    if(strcmp(tokens->data[0],"cd") == 0) {
        if (tokens->size == 1){
            char* homeDir = getenv("HOME");
            cd(homeDir);
        }
        else{
            cd(tokens->data[1]);
        }
    }
    if(strcmp(tokens->data[0],"pwd") == 0 && tokens->size == 1) {
        pwd();
        return;
    }
    if(strcmp(tokens->data[0],"search") == 0 && tokens->size == 2) {
        searchFile(tokens->data[1]);
        return;
    }
    // Neither Path nor Built-in Command
    searchFile(tokens->data[0]);
    if(lastExit == 0) {     // Means that a file with the given name has been found
        // Execute the file with the given name
    }

    // Free the tokenized line
    al_destroy(tokens);
    free(tokens);
}

int main(int argc, char* argv[]) {
    int file, bytes;
    char* buffer = malloc(sizeof(char)*BUF_SIZE);
    lineBuffer = malloc(sizeof(char)*BUF_SIZE);

    char* str1 = malloc(sizeof(char)* 10);
    strcpy(str1, "dogman");
    char* str2 = malloc(sizeof(char)*10);
    strcpy(str2, "is cool");

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
            list_t* tokens = tokenize(lineBuffer);
            interpreter(tokens);
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
                    list_t* tokens = tokenize(lineBuffer);
                    interpreter(tokens);
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