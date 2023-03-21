#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include "arraylist.h"

#define BUF_SIZE 4096

char buffer[BUF_SIZE];
char lineBuffer[BUF_SIZE];
int lineLength;
int lastExit = 0;

void (*perror_ptr)(const char*) = &perror;
void myperror(const char* str){
    (*perror_ptr)(str);
    lastExit = 1;
}

#define perror(str) myperror(str)

int isPath(char* token){
    int i;
    for (i = 0; token[i] != 0; i++){
        if (token[i] == '/'){
            return 1;
        }
    }
    return 0;
}

char* cloneString(char* strToCopy){
    size_t strSize = 0;
    int i;
    for (i = 0; strToCopy[i] != 0; i++){
        strSize++;
    }
    char* strClone = malloc(strSize+1);
    strcpy(strClone, strToCopy);
    return strClone;
}

char isSpecialToken(char firstChar){
    return (firstChar == '|' || firstChar == '>' || firstChar == '<');
}

char* append(char* dest, char* toAppend) {
    int dest_length = 0, append_length = 0;
    while(dest[dest_length] != '\0') {dest_length++;}
    while(toAppend[append_length] != '\0') {append_length++;}
    dest = realloc(dest, dest_length + append_length + 1);
    strcat(dest, toAppend);
    return dest;
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

char* path_variables[] = {
    "/usr/local/sbin/",
    "/usr/local/bin/",
    "/usr/sbin/",
    "/usr/bin/",
    "/sbin/",
    "/bin",
    NULL
};
char* searchPath(char* file) {
    struct stat block;

    int i;
    for (i = 0; path_variables[i] != NULL; i++){
        char* path = malloc(18);
        path = strcpy(path, path_variables[i]);
        char* parentPath = cloneString(path);
        char* fullPathtoFile = append(path, file);
        if(stat(fullPathtoFile, &block) != -1) {
            free(fullPathtoFile);
            return parentPath;
        }
        free(fullPathtoFile);
        free(parentPath);
        parentPath = NULL;
    }

    perror(file);
    return NULL;
}

int cd(char* path) {
    int ret = chdir(path);
    if(ret == -1) {
        perror("chdir");
    }
    return ret;
}

void pwd() {
    // Change the size of the buffer until it can encapsulate the path
    int size = 100, complete = 0;
    char* buffer = malloc(sizeof(char)*(100+2)); // Add two for \n and \0 chars
    while (!complete) {
        if(getcwd(buffer, size-1) != NULL) {
            complete = 1;
        }
        else {
            size = size * 2;
            buffer = (char*)realloc(buffer, sizeof(char)*(size+2));
        }
    }
    // Find the size of the path
    int pathSize = 0;
    for(int i = (size > 100 ? size/2 : 0); i < size; i++) {
        pathSize = i+1;
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
        i++;
    }
    *totalChars = i;
    return isWild;
}

int identifyWild(char* fileName, char* pattern, int asterisk, int totalChars) {
    int num_front = asterisk;
    int num_end =  totalChars - asterisk - 1;
    //printf("%d %d\n", num_front, num_end);
    int fileNameSize = 0;
    while(fileName[fileNameSize] != '\0') {fileNameSize++;}
    //printf("%d\n", fileNameSize);
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

//  Responsible for converting input lines to tokens
//  And checking syntax
list_t *tokenize(char *input, unsigned* numProcesses)
{
    if(strcmp(input, "\n") == 0) {
        return NULL;
    }    
    list_t* token_arr = malloc(sizeof(list_t));
    al_init(token_arr, 8);
    char lastWasSpecial = 1;
    char scanningWhitespace = 0;
    int i = 0;
    while (input[i] != '\0')
    {
        if (scanningWhitespace)
        {
            //  No whitespace detected here: start scanning this token
            scanningWhitespace = 0;
            //  Count token length
            int j = i;
            char isSpecial = isSpecialToken(input[i]) || input[i] == ' ';
            if (!isSpecial)
            {
                while (input[i] != '\0')
                {
                    i++;
                    isSpecial = isSpecialToken(input[i]) || input[i] == ' ';
                    if (isSpecial)
                    {
                        break;
                    }
                }
            }
            if (input[i] == '\0')
            {
                //  Avoid redundant terminator character or new line character
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
                //  Long enough to be a reference to the home directory
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
            
            //  Copy the characters
            memcpy(str + referencesHomeDir, &(input[j + (referencesHomeDir != 0)]), str_size);
            //  Add terminator character
            str[str_size + referencesHomeDir - 1 - (referencesHomeDir != 0)] = '\0';
            //  Push token into the array
            al_push(token_arr, str);
            lastWasSpecial = 0;
        }
        else
        {
            //  Move past whitespace
            scanningWhitespace = 1;
            char isSpecial = isSpecialToken(input[i]);
            while (input[i] == ' ' || isSpecial)
            {
                if (isSpecial)
                {
                    if (lastWasSpecial){break;}
                    if (input[i] == '|'){
                        *numProcesses = *numProcesses+1;
                    }
                    char* str2 = malloc(sizeof(char) * 2);
                    str2[0] = input[i];
                    str2[1] = '\0';
                    al_push(token_arr, str2);
                    lastWasSpecial = 1;
                }
                i++;
                isSpecial = isSpecialToken(input[i]);
            }
        }
    }
    
    if (lastWasSpecial){
        printf("tokenize: incorrect syntax\n");
        lastExit = 1;
    }
    
    return token_arr;
}

int verifyExecutability(char* filePath){
    struct stat block;
    if (stat(filePath, &block) == -1)
    {
        perror(filePath);
        return 1;
    }
    else if (!(block.st_mode & S_IXUSR) || ((block.st_mode & __S_IFMT) != __S_IFREG))
    {
        printf("%s: Not an executable or do not have permission\n", filePath);
        lastExit = 1;
        return 1;
    }
    return 0;
}

int interpreter(list_t* tokens, unsigned numChildren) {
    //  Terminal ignores empty lines
    if(tokens == NULL) {
        return 0;
    }
    unsigned numTokens = tokens->size;
    int pipes[numChildren][2];
    int errpipes[numChildren][2];

    int i;
    int processNum = 0;
    for (i = 0; i < numTokens; i++)
    {
        //  Create pipe

        if (pipe(pipes[processNum]) == -1)   { perror("pipe"); return -1; }
        if (pipe(errpipes[processNum]) == -1){ perror("pipe"); return -1; }

        pid_t processID = fork();
        if (processID == 0)
        {
            //  Child process: ith program

            //  Initialize arguments list
            list_t* args = malloc(sizeof(list_t));
            al_init(args, 8);

            //  Handle first token
            int startIndex = i;
            char* executableName = NULL;
            char* parentPath = NULL;

            //  Is not a built in command
            if (!strcmp(tokens->data[i], "cd") == 0 && !strcmp(tokens->data[i], "pwd") == 0)
            {
                //  Is a path to an executable (contains a '/')
                if (isPath(tokens->data[i]))
                {
                    parentPath = obtainParent(tokens->data[i], &executableName);
                    free(executableName); 
                }
                //  Is not a path to executable, search path variables
                else
                {
                    executableName = tokens->data[i];
                    parentPath = searchPath(executableName);
                    //  Exists in one of six paths
                    if (parentPath != NULL)
                    {
                        tokens->data[i] = append(parentPath, executableName);
                        free(executableName);
                    }
                    //  Does not exist in the paths
                    else
                    {
                        free(parentPath);
                        parentPath = NULL;
                        al_destroy(args);
                        free(args);
                        write(errpipes[processNum][1], &lastExit, 1);
                        return 1;
                    }
                }

                //  Prints error and sets lastExit if file is not executable
                verifyExecutability(tokens->data[i]);
            }

            al_push(args, cloneString(tokens->data[i]));

            //  Collect program arguments, redirections
            char redirected = 0;
            int inputRedirection = STDIN_FILENO;
            int outputRedirection = STDOUT_FILENO;
            i++;
            while (!lastExit && i < numTokens && tokens->data[i][0] != '|')
            {
                char* token = tokens->data[i];
                //  Input redirect
                if (token[0] == '<')
                {
                    if (i < numTokens-1)
                    {
                        //  Close any previous redirections (we can allow multiple redirections)
                        (inputRedirection != STDIN_FILENO) ? (close(inputRedirection) == -1 ? perror("close") : 0) : 0;
                        //  Open provided input
                        if ((inputRedirection = open(tokens->data[i+1], O_RDONLY)) == -1){ 
                            perror(tokens->data[i+1]); 
                            break; 
                        }
                        //  Already read the next token, increment twice
                        i++;
                        redirected = 1;
                    }
                }
                //  Output redirect
                else if (token[0] == '>')
                {
                    if (i < numTokens-1)
                    {
                        //  Close any previous redirections
                        (outputRedirection != STDOUT_FILENO) ? (close(outputRedirection) == -1 ? perror("close") : 0) : 0;
                        //  Open provided output
                        if ((outputRedirection = open(tokens->data[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0640)) == -1){ 
                            perror(tokens->data[i+1]); 
                            break; 
                        }
                        //  Already read the next token, increment twice
                        i++;
                        redirected = 1;
                    }
                }
                //  Add arguments only before a redirection occurs
                else if (!redirected)
                {
                    int asterisk = -1, totalChars = -1, foundWildCard = 0;
                    if (isWild(tokens->data[i], &asterisk, &totalChars) == 1)
                    {
                        DIR* directory = opendir(parentPath);
                        if(directory == NULL) {
                            perror("opendir");
                            break;
                        }
                        struct dirent *dir;
                        //  Read through directory to find files that fit the wildcard pattern
                        while ((dir = readdir(directory)) != NULL) {
                            if (identifyWild(dir->d_name, tokens->data[i], asterisk, totalChars)){
                                al_push(args, cloneString(dir->d_name));
                                foundWildCard = 1;
                                //  found a file that will be passed as an argument
                                //  pass the file name in as an argument
                            }
                        }
                        // If no file matching the wildcard is found, then pass it as an argument to the program
                    }
                    
                    if (!foundWildCard)
                    {
                        al_push(args, cloneString(token));
                    }
                }
                //  Else, ignore extra arguments just like linux shell normally would
                i++;
            }

            write(errpipes[processNum][1], &lastExit, 1);
            if (!lastExit)
            {
                //  Pipe to another program is next
                if (i < numTokens && tokens->data[i][0] == '|')
                {
                    //  Prepare output to write to pipe (> takes priority)
                    if (outputRedirection == STDOUT_FILENO)
                    {
                        outputRedirection = pipes[processNum][1];
                    }
                }
                //  Prepare input to read from previous pipe (< takes priority)
                if (processNum > 0 && inputRedirection == STDIN_FILENO)
                {
                    inputRedirection = pipes[processNum-1][0];
                }

                //  Redirect input, output
                (inputRedirection  != STDIN_FILENO)  ? dup2(inputRedirection, STDIN_FILENO)   : 0;
                (outputRedirection != STDOUT_FILENO) ? dup2(outputRedirection, STDOUT_FILENO) : 0;
                
                //  Close extra file descriptors
                close(pipes[processNum][0]);
                close(pipes[processNum][1]);
                if (processNum > 0){
                    close(pipes[processNum-1][0]);
                    close(pipes[processNum-1][1]);
                }
            
                //  Handle built in commands
                if(strcmp(tokens->data[startIndex], "cd") == 0 && numChildren > 1) //   If this is the only child process.. do nothing 
                {
                    //  If this ends up being run in a child process, it will effectively do nothing. But it should run anyway
                    //  to throw errors
                    if (tokens->size > 1 && startIndex < tokens->size-1 && !isSpecialToken(tokens->data[startIndex+1][0])){
                        cd(tokens->data[startIndex+1]);
                    }
                    else{
                        char* homeDir = getenv("HOME");
                        cd(homeDir);
                    }
                }
                else if(strcmp(tokens->data[startIndex], "pwd") == 0)
                {
                    pwd();
                }
                //  Not built in, call execv
                else
                {
                    //  NULL terminator so program can count the number of arguments
                    al_push(args, NULL);
                    execv(tokens->data[startIndex], args->data);
                }
            }

            //  Free arguments list
            al_destroy(args);
            free(args);

            //  Free extra pointers
            if (parentPath != NULL){
                free(parentPath);
                parentPath = NULL;
            }
            
            //  Write success

            //  Return 1 to signal that this is a child process 
            return 1;
        }
        else
        {
            //  Parent process --> skip to anything after the pipe, if it exists
            while (i < numTokens && tokens->data[i][0] != '|'){
                i++;
            }

            //  Don't want to put cd into a child process(unless there's a pipe): we want
            //  to switch the directory of the parent process
            if (numChildren == 1 && strcmp(tokens->data[0], "cd") == 0){
                if (tokens->size > 1){
                    cd(tokens->data[1]);
                }
                else{
                    char* homeDir = getenv("HOME");
                    cd(homeDir);
                }
                int wstatus;
                wait(&wstatus);
                return 0;
            }

            //  Count the next process
            processNum++;
        }
    }

    for (int i = 0; i < numChildren; i++){
        char errorDetected;
        int bytes = read(errpipes[i][0], &errorDetected, 1);
        if (bytes == -1){
            perror("read");
        }
        if (errorDetected){ lastExit = 1; }
    }

    int wstatus;
    for (int i = 0; i < numChildren; i++)
    {
        wait(&wstatus);
        if (wstatus != 0){
            lastExit = 1;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int file, bytes;
    // Check whether we're in batch mode, interactive mode, or too many arguments were passed
    if(argc == 1) {
        //interactive mode
        printf("Welcome to my shell!\n");
        // input loop
        file = 0;
        write(1, "mysh> ", 7);
        while((bytes = read(file, buffer, BUF_SIZE)) > 0) {
            lastExit = 0;

            memcpy(lineBuffer, buffer, bytes);
            lineBuffer[bytes] = '\0';
            //  Exit Condition
            if(strcmp(lineBuffer, "exit\n") == 0) {
                write(1, "mysh> exiting...\n", 17);
                close(file);
                return 0;
            }
            //  Interpret Line
            unsigned numProcesses = 1;
            list_t* tokens = tokenize(lineBuffer, &numProcesses);
            //  Tokens have no syntax errors, so we can proceed
            if (lastExit == 0){
                int isChildProcess = interpreter(tokens, numProcesses);
                al_destroy(tokens);
                free(tokens);
                
                if (isChildProcess){
                    return 0;
                }
            }
            //  The terminal prompt: Preceded by ! if the last exit code was non zero / failed execution
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
                        return 0;
                    }
                    // Do something with lineBuffer here
                    unsigned numProcesses = 1;
                    list_t* tokens = tokenize(lineBuffer, &numProcesses);
                    int isChildProcess = 0;
                    if (lastExit == 0){
                        isChildProcess = interpreter(tokens, numProcesses);
                    }
                    lastExit = 0;
                    al_destroy(tokens);
                    free(tokens);

                    if (isChildProcess){
                        return 0;
                    }
                    
                    // Set the starting position of the next line in the buffer
                    lineStart = i+1;
                }
            }
        }
        // If the last character in the file wasn't a '/n', then we have a partial command: just ignore it.
    }
    else {
        printf("mysh: Too many arguments\n");
    }
}