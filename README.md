# CS214_PA2

Creators	:

Armen Karakashian	netID avk56
Jason Guo	  	netID jg1715

Extensions	:

3.2 Home directory
3.4 Multiple pipes

Test plan	:

First, we studied how the normal shell (bash) treats different kinds of commands.

Our observations:

Assume "foo" is an executable file in the PATH and "a.txt", "b.txt", "c.txt" exist in the current working directory.

- "cd" changes the current working directory to the home directory, but "cd | foo" does not. Our takeaway was that the pipe creates two separate child processes to run "cd" and "foo", so the working directory of the parent process doesn't change.
- The last token, "baz", in "foo bar > a.txt baz" is ignored. "foo" and "bar" are the only arguments passed to foo. After redirection occurs, no more arguments are passed to the program.
- "thisfiledoesnotexist | foo" runs foo but prints an error for "thisfiledoesnotexist" (because it does not exist). So, in our shell, we need to make sure if there's an error for one process, it does not affect other processes.
- "foo | bar > > b.txt" prints a syntax error and does NOT run foo. We need to scan the command line for any syntax errors before we create and run child processes.
- "foo < a.txt < b.txt > c.txt" redirects input to b.txt and output to c.txt for foo. Similarly, "foo < a.txt > c.txt > b.txt" redirects output to b.txt. When there are more than one input/output redirects, we just redirect again.
- "foo                    <       a.txt" redirects input to a.txt. bash ignores extra whitespace.
- "foo | thisfiledoesnotexist" runs foo and prints an error for "thisfiledoesnotexist", but even though there's an error on the other end of the pipe, foo's output is still redirected. In other words, we need to make our shell so that foo's output does not default to STDOUT in the event of such an error.

Second, we implemented these above observations as functions of our shell to keep it faithful to bash.

Third, we created C two files to help us test the commands:

- "readwrite.c" reads STDIN to a buffer and then writes the buffer to STDOUT
- "printargs.c" prints everything in argv

Lastly, we created a .txt file of test command lines called "commands.txt" (see project folder). These commands include those that are similar to the commands we ran in bash to observe how bash responds. We did this to make sure our implementation is as close to bash as possible.

Extra note: We noticed that we can't add any new files to the default directories (/usr/bin, /bin, etc.), so we knew we had to try running something that is already there for testing. We did this by creating a simple python script ("myscript.py") and adding the command "python myscript.py" to "commands.txt", since python exists in "/usr/bin".