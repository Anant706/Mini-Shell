/*
Author: Anant Kumar Yadav
PeopleSOft Id : 1xxxxx
Department : Computer Science
University : University Of Houston
Date of submission: 10/12/2018
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>

#define BUFFSIZE 1024
#define INPUTLINE_DELIM " \t\r\n\a"
#define BG_PROCESS_MAX 100

const char *cCleanrScrn = " \e[1;1H\e[2J";

int bgCmdCount;
char *shellPrompt = "MyShell";

struct BG_PROCESS_INFO
{
    char **argv;
    //pid_t data type represents process IDs. ... Your program should include the header files unistd.h
    pid_t pid;
    pid_t ppid;
};

struct BG_PROCESS_INFO new_bg_cmd[BG_PROCESS_MAX];

/*
method to search the user command in $PATH variable
*/
char *cmdFromPath(char *uCommand)
{

    char *directory = getenv("PATH");
    char *cmdBuffer = (char *)malloc(20);
    int pathLen = 0;
    char *pathArray = strtok(directory, ":");

    while (pathArray != NULL)
    {
        pathLen = strlen(pathArray) + 1 + strlen(uCommand);
        strcpy(cmdBuffer, pathArray);
        strcat(cmdBuffer, "/");
        strcat(cmdBuffer, uCommand);
        cmdBuffer[pathLen] = '\0';

        if (access(cmdBuffer, X_OK) == 0)
        {
            break;
        }
        /* Divide S into tokens separated by characters in DELIM. Iterates the directory variable */
        pathArray = strtok(NULL, ":");
    }
    return cmdBuffer;
}

/*
method to read user input from console
*/
char *read_line(void)
{
    int c;
    int index = 0;
    int maxString = BUFFSIZE;
    char *buffer = (char *)malloc(BUFFSIZE);

    if (!buffer)
    {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (maxString > 0)
    {
        //reading a character from any input stream
        c = getchar();

        if (c == EOF || c == '\n')
        {
            buffer[index] = '\0';
            return buffer;
        }
        else
        {
            buffer[index] = c;
            index++;
        }
        maxString--;
    }

    // If we have exceeded the buffer, reallocate.
    if (index >= BUFFSIZE)
    {
        index += BUFFSIZE;
        buffer = realloc(buffer, index);
        if (!buffer)
        {
            fprintf(stderr, "allocation error\n");
            exit(EXIT_FAILURE);
        }
    }

    return buffer;
}

/*
method to split the console line commands into tokens
*/
char **split_line(char *line)
{
    int bufsize = BUFFSIZE, position = 0;

    //array of pointers, to hold pointer i.e. tokens
    char **buffer = (char **)malloc(bufsize);
    char *token;

    if (!buffer)
    {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, INPUTLINE_DELIM);
    //printf("token is: > %s", token);
    while (token != NULL)
    {
        buffer[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += BUFFSIZE;
            buffer = realloc(buffer, bufsize * sizeof(char *));
            if (!buffer)
            {
                fprintf(stderr, "allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, INPUTLINE_DELIM);
    }
    buffer[position] = NULL;
    return buffer;
}

/*
method to show background processes
*/
int displayProcess(char **args)
{

    printf("\n-----------------Total background process = %d-----------------\n", bgCmdCount);
    for (int i = 0; i < bgCmdCount; i++)
    {
        printf("\nProcess Name = %s %s %s, Parent Id = %d, Child Id = %d\n\n",
               new_bg_cmd[i].argv[0], new_bg_cmd[i].argv[1], new_bg_cmd[i].argv[2],
               new_bg_cmd[i].ppid, new_bg_cmd[i].pid);
    }
    return EXIT_SUCCESS;
}

/*
method to change current working directory
*/
int changeDir(char **args)
{

    char buf[100];
    if (strcmp(args[1], "/") != 0)
    {
        char *gdir = getcwd(buf, sizeof(buf));
        char *dir = strcat(gdir, "/");
        char *to = strcat(dir, args[1]);
        chdir(to);
    }
    else
        chdir(args[1]);

    return EXIT_SUCCESS;
    //currrDir = getcwd(buf, sizeof(buf));
}

/*
method to kill background process using exit command
*/
int killProcess(char **args)
{
    int bgProcessCnt = 0;

    for (int i = 0; i < bgCmdCount; i++)
    {
        kill(new_bg_cmd[i].pid, SIGKILL);
        int programStatus = kill(new_bg_cmd[i].pid, 0);

        if (programStatus == 0)
        {
            bgProcessCnt++; // need to be assign 1
            printf("\nProcess Name = %s %s %s, Parent Id = %d, Child Id = %d, isRunning = False\n",
                   new_bg_cmd[i].argv[0], new_bg_cmd[i].argv[1], new_bg_cmd[i].argv[2],
                   new_bg_cmd[i].ppid, new_bg_cmd[i].pid);
        }
    }

    if (bgProcessCnt == 0)
    {
        printf("No background processs.\n");
    }
    else
    {
        bgCmdCount = 0;
        printf("\n%d running background processs were killed.\n\n", bgProcessCnt);
    }

    return EXIT_SUCCESS;
}

/*
method to execute user command
*/
bool executeCommand(char **args)
{
    // background process argument array
    char **dupArgs = (char **)malloc(BUFFSIZE);

    pid_t pid;
    int status;
    bool isBgCommand = false;

    // input/output redirectional operator variable
    int inOperator = 0, outOperator = 0;

    if (strcmp(args[0], "processes") == 0)
    {
        displayProcess(args);
        return 0;
    }

    if (strcmp(args[0], "exit") == 0)
    {
        killProcess(args);
        return 0;
    }

    if (strcmp(args[0], "bg") == 0)
        isBgCommand = true;

    pid = fork();

    if (pid == 0)
    {
        char *input = (char *)malloc(64);
        char *output = (char *)malloc(64);
        int indir = 0, outdir = 0;

        for (int i = 0; args[i] != NULL; i++)
        {
            /*
            make a different array of arguments as command for background processes i.e dupArgs[]
            */
            if (isBgCommand)
            {
                dupArgs[i] = args[i + 1];
            }

            if (strcmp(args[i], "<") == 0)
            {
                indir = i;
                args[i] = "";
                strcpy(input, args[i + 1]);
                inOperator = 2;
            }

            if (strcmp(args[i], ">") == 0)
            {
                outdir = i;
                args[i] = NULL;
                strcpy(output, args[i + 1]);
                outOperator = 2;
            }
        }

        if (indir > 0 && outdir > 0)
        {
            if (indir < outdir)
            {
                args[indir] = NULL;
                args[outdir] = "";
            }
            else
            {
                args[outdir] = NULL;
                args[indir] = "";
            }
        }
        // '<'  operator
        if (inOperator)
        {
            int fd0;
            if ((fd0 = open(input, O_RDONLY, 0)) < 0)
            {
                perror("Couldn't open input file\n");
                exit(0);
            }
            dup2(fd0, 0);
            close(fd0);
        }
        // '>' operator
        if (outOperator)
        {

            int fd1;
            if ((fd1 = creat(output, 0644)) < 0)
            {
                perror("Couldn't open the output file\n");
                exit(0);
            }

            dup2(fd1, STDOUT_FILENO);
            close(fd1);
        }

        char *cmd;
        int type;
        //execute foreground and background command
        if (isBgCommand)
        {
            cmd = cmdFromPath(dupArgs[0]);
            type = execv(cmd, dupArgs);
        }
        else
        {
            cmd = cmdFromPath(args[0]);   
            type = execv(cmd, args);
        }
        if (type)
        {
            perror("Error");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // make a list of background processes in array
        if (isBgCommand)
        {
            int i;

            new_bg_cmd[bgCmdCount].argv = (char **)malloc(BUFFSIZE);

            for (i = 0; args[i] != NULL; i++)
            {
                new_bg_cmd[bgCmdCount].argv[i] = args[i];
            }

            new_bg_cmd[bgCmdCount].pid = pid;
            new_bg_cmd[bgCmdCount].ppid = getpid();
            printf("\n----------------- New background process created -----------------\n");
            printf("Original Command = %s %s %s\n\n",
                   new_bg_cmd[bgCmdCount].argv[0], new_bg_cmd[bgCmdCount].argv[1], new_bg_cmd[bgCmdCount].argv[2]);
            bgCmdCount++;
        }
        else
        {
            do
            {
                wait(&status);
                waitpid(pid, &status, WUNTRACED);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }

    return EXIT_SUCCESS;
}

/*
    if (strcmp(args[0], "exit") == 0)
        exit(0);
*/

int main(int argc, char **argv)
{
    //reading the console input as line
    char *line;

    // holding the array of arguments spitted up
    char **args;

    //holding the status of executing commands
    int status = 1;

    //Clear the screen and display MyShell name
    write(STDOUT_FILENO, cCleanrScrn, 12);

    // Loop getting input and executing it.
    do
    {
        printf("%s>", shellPrompt);

        line = read_line();

        if (strcmp(line, "") != 0)
        {
            /* slitting the console commands into tokens */
            args = split_line(line);
            if (NULL != args)
                executeCommand(args);
        }
    } while (status);

    bgCmdCount = 0;

    //exit successfully
    return EXIT_SUCCESS;
}
