#include <iostream>
// #include <csignal>

#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>

#include "Command.hpp"

using namespace std;

int execCommand(Command com);
int execCommand(Command com, int p[2], int WoR);
int setUpPipedCommand(Command com);

int main(void)
{
    Command com;
    // prompt for and read in first command.
    cout << "😂👌💯 ▶ ";
    com.read();
    while (com.name() != "exit") // Go until user inputs exit command
    {
        if (com.name() == "cd") // Test for Change Dir command
            chdir(com.args()[1].c_str());
        else // Execute any other command
        {
            int pid = fork();
            if (pid != 0) // Parent Shell (Will wait on child to finish running command unless backgrounded)
            {
                while (com.pipeOut())
                {
                    com.read();
                }
                if (!com.backgrounded())
                    waitpid(pid, NULL, 0);
            }
            else // Child Process (Will exec the given command and will terminate when command is finished)
            {
                return com.pipeOut() ? setUpPipedCommand(com) : execCommand(com); // Should only return if something went super wrong
            }
        }
        cout << "😂👌💯 ▶️ ";
        com.read();
    }
    cout << "Thank you for using mini-shell. We now return you to your regularly scheduled shell!" << endl;
    return 0;
}

int execCommand(Command com)
{
    if (com.redirIn()) // Set stdin to file if redirect in
    {
        FILE *fp = fopen(com.inputRedirectFile().c_str(), "r");
        dup2(fileno(fp), STDIN_FILENO);
        fclose(fp);
    }
    if (com.redirOut()) // Set stdout to file if redirect out
    {
        FILE *fp = fopen(com.outputRedirectFile().c_str(), "w+");
        dup2(fileno(fp), STDOUT_FILENO);
        fclose(fp);
    }
    int numArgs = com.numArgs();       // Get number of entered arguments
    char *execArgs[10];                // Create array to store arguments for exec to run
    for (int i = 0; i <= numArgs; ++i) // Loop through arguments and add to array (Last iteration will append NULL)
    {
        i != numArgs ? execArgs[i] = (char *)com.args()[i].c_str() : execArgs[i] = NULL;
    }
    int errno; // Run exec and store any error number if exec fails
    errno = execvp(execArgs[0], execArgs);
    cerr << "Uh Oh! You shouldn't be here. Exec failed with error: " << errno << endl;
    return 1;
}

int execCommand(Command com, int pipe[2], bool piped)
{
    if (piped) // Forked proccess running command to be piped to
    {
        close(pipe[1]);              // Close pipe writer **IMPORTANT**
        com.read();                  // Read command to be piped to
        dup2(pipe[0], STDIN_FILENO); // Set stdin to pipe reader
        if (com.pipeOut())           // Check if piped command pipes to
        {
            return setUpPipedCommand(com);
        }
    }
    else // Forked proccess running command to be piped from
    {
        close(pipe[0]);               // Close the pipe reader
        dup2(pipe[1], STDOUT_FILENO); // Set stdout to pipe writer
    }
    return execCommand(com);
}

int setUpPipedCommand(Command com)
{
    int p[2];
    if (pipe(p) != 0) // Create the pipe before forking
    {
        cout << "Pipe creation failed. Quitting." << endl;
        return 1;
    }
    int pid = fork();
    if (pid != 0) // Run both halves of the piped command
    {
        return execCommand(com, p, true);
    }
    else
    {
        return execCommand(com, p, false);
    }
}