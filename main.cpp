#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

#include "Command.hpp"

int execCommand(Command com);
int setUpPipedCommand(Command com);

int main(void)
{
    Command com;
    printf("😂👌💯 ▶ "); // Prompt for and read in first command
    com.read();
    while (com.name() != "exit") // Go until user inputs exit command
    {
        if (com.name() == "cd") // Check for "cd" command
            chdir(com.args()[1].c_str());
        else // Execute any other command
        {
            int pid = fork();
            if (pid != 0) // Parent Shell (Will wait on child to finish running command unless backgrounded)
            {
                while (com.pipeOut()) // Read through command until not piping
                    com.read();
                if (!com.backgrounded()) // If not told to background, wait until child finishes executing
                    waitpid(pid, NULL, 0);
            }
            else // Child Process (Will execute the given command and will terminate when command is finished)
                return com.pipeOut() ? setUpPipedCommand(com) : execCommand(com);
        }
        printf("😂👌💯 ▶ "); // Prompt for and read in next command
        com.read();
    }
    printf("Thank you for keeping it 💯 with mini-shell. We now return you to your regularly scheduled shell!\n");
    return 0;
}

/**
 * Uses the execvp function to run input cli command
 * @param com Command class object containing information on input command
 * @return 1 only if exec fails
 */
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
    char *execArgs[MAX_ARGS + 1];      // Create array to store arguments for exec to run
    for (int i = 0; i <= numArgs; ++i) // Loop through arguments and add to array (Last iteration will append NULL)
        i != numArgs ? execArgs[i] = (char *)com.args()[i].c_str() : execArgs[i] = NULL;
    int errorNumber = execvp(execArgs[0], execArgs); // Run exec and store any error number if exec fails
    fprintf(stderr, "Uh Oh! You shouldn't be here. Exec failed with error: %d\n", errorNumber);
    return 1;
}

/**
 * Sets up a pipe to be used between forked processes and overwrites stdio to the pipe's ends.
 * After setup, execCommand is called to have the forked processes execute their respective commands.
 * @param com Command class object containing information on input command to be split and sent to execCommand
 * @return 1 only if pipe or execCommand fails
 */
int setUpPipedCommand(Command com)
{
    int p[2];
    if (pipe(p) != 0) // Create the pipe before forking
    {
        fprintf(stderr, "Pipe creation failed. Quitting.\n");
        return 1;
    }
    int pid = fork();
    if (pid != 0) // Forked proccess running command to be piped to
    {
        close(p[1]);              // Close pipe writer **IMPORTANT**
        com.read();               // Read command to be piped to
        dup2(p[0], STDIN_FILENO); // Set stdin to pipe reader
        if (com.pipeOut())        // Check if piped command pipes to another command
            return setUpPipedCommand(com);
    }
    else // Forked proccess running command to be piped from
    {
        close(p[0]);               // Close the pipe reader
        dup2(p[1], STDOUT_FILENO); // Set stdout to pipe writer
    }
    return execCommand(com);
}