#include <iostream>
#include <csignal>

#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "Command.hpp"

void execPipedCom(int p[2], int WoR, Command com);

int main(void)
{
    Command com;
    int num = 1; // keep track of number of commands.

    // prompt for and read in first command.
    cout << ">>>> ";
    com.read();

    while (com.name() != "exit")
    {
        // print out current command
        cout << num++ << ")" << com << endl;
        // Test for Change Dir command
        if (com.name() == "cd")
            chdir(com.args()[1].c_str());
        else // Execute any other command
        {
            int pid = fork();
            if (pid != 0) // Parent Shell (Will wait on child to finish running command unless backgrounded)
            {
                if (!com.backgrounded())
                    waitpid(pid, NULL, 0);
            }
            else // Child Process (Will exec the given command and will terminate when command is finished)
            {
                if (com.pipeOut()) // Command was piped
                {
                    int p[2];
                    if (pipe(p) != 0)
                    {
                        cout << "Pipe creation failed. Quitting." << endl;
                        return 1;
                    }
                    int pid = fork();
                    if (pid != 0)
                    {
                        execPipedCom(p, 1, com);
                    }
                    else
                    {
                        execPipedCom(p, 0, com);
                    }
                }
                else // Command was not piped
                {

                    int numArgs = com.numArgs();
                    char *execArgs[numArgs + 1];
                    for (int i = 0; i <= numArgs; ++i)
                        i != numArgs ? strcpy(execArgs[i], com.args()[i].c_str()) : execArgs[i] = NULL;
                    return 0;
                }
            }
        }
        // prompt for and read next command
        cout << ">>>> ";
        com.read();
    }

    cout << "Thank you for using mini-shell. We now return you to your regularly scheduled shell!" << endl;
    return 0;
}

void execPipedCom(int p[2], int RoW, Command com)
{
    if (RoW == 1) // Forked proccess running command to be piped to
    {
        close(p[1]);               // Close pipe writer **IMPORTANT**
        com.read();                // Read command to be piped to
        dup2(p[0], fileno(stdin)); // Set stdin to pipe reader
    }
    else // Forked proccess running command to be piped from
    {
        close(p[0]);                // Close the pipe reader
        dup2(p[1], fileno(stdout)); // Set stdout to pipe writer
    }
    if (com.redirIn()) // See if command input was redirected
    {
        FILE *fp = fopen(com.inputRedirectFile().c_str(), "r");
        dup2(fileno(fp), fileno(stdin));
    }
    else if (com.redirOut()) // See if command output was redirected
    {
    }
    int numArgs = com.numArgs();
    char *execArgs[numArgs + 1];
    for (int i = 0; i <= numArgs; ++i)
        i != numArgs ? strcpy(execArgs[i], com.args()[i].c_str()) : execArgs[i] = NULL;
    execvp(execArgs[0], execArgs);
}
