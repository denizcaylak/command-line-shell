/*
Remzi Deniz Çaylak
150210081
Assignment 1
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LINE_LENGTH 250 //Specified in pdf.
#define MAX_COMMANDS 100
#define MAX_HISTORY 100

char history[MAX_HISTORY][MAX_LINE_LENGTH];//These are for "history" command. To prinf the history of commands.
int history_count = 0;

void execute_command(char *command) {//Execute single line of commands function. Such as "ls" or "cat f1.txt".

    char *args[MAX_COMMANDS];
    int arg_index = 0;

    char *token = strtok(command, " ");//Parse the " " for separating command and argument.
    while (token != NULL) {
        args[arg_index++] = token;
        token = strtok(NULL, " ");
    }
    args[arg_index] = NULL;

    execvp(args[0], args);//Directly execute the command because it is only 1 command.
    perror("Exec failed");
    exit(EXIT_FAILURE);
}

void execute_sequential_commands(char *commands[], int num_commands) {//This function is for sequential commands. If command contains ";" then this function will executed.
    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();//Because there are more than 1 command, fork() will be used here.
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        
        else if (pid == 0) { //Child process runs. It sends the argument to execute_command() function.
            execute_command(commands[i]);
        }
        
        else { //Parent process runs.
            //First wait for child process to finish.
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void execute_piped_commands(char *commands[], int num_commands) {//This function is for commands that contain "|" element. That means pipes will be used.
    pid_t pid;
    int status;
    int pipes[MAX_COMMANDS - 1][2];//Create pipes here.

    //Initialize the pipes.
    for (int i = 0; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("Pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_commands; i++) {//It loops through each command. First forks it and after forking runs it.
        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        }
        
        else if (pid == 0) { //Child process runs.
            //If it is not first command, redirect input from the previous pipe.
            if (i != 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
            //If it is not last command, redirect output to the next pipe.
            if (i != num_commands - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            execute_command(commands[i]);//Send the arguments to execute_command() function.
        }
    }

    //Closing all pipes.
    for (int i = 0; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    //Wait for all child processes to finish.
    for (int i = 0; i < num_commands; i++) {
        wait(&status);
    }
}

void print_history() {//This function is for history command.
    printf("Command History:\n");
    for (int i = 0; i < history_count; i++) {//Prints the commands that executed before history command.
        printf("%d. %s\n", i + 1, history[i]);
    }
}

int main(int argc, char *argv[]) {
    printf("Shell is ready. Enter commands ('quit' to exit).\n");

    if (argc == 2) {//This if statement means if program is used with file. For example "./shell script.sh".
        
        FILE *script_file = fopen(argv[1], "r");//Opens the script file.

        if (script_file == NULL) {
            perror("Error opening script file");
            return EXIT_FAILURE;
        }

        
        char line[MAX_LINE_LENGTH];
        while (fgets(line, sizeof(line), script_file) != NULL) {//Reads and executes commands from the script file.

            if (line[strlen(line) - 1] == '\n') {//Handle the new line character.
                line[strlen(line) - 1] = '\0';
            }
            printf(">> %s\n", line); //Print the command inside script file to terminal.
            
            char *commands[MAX_COMMANDS];
            int num_commands = 0;

            if (strchr(line, '|') != NULL) { // Piped commands
                char *token = strtok(line, "|");
                while (token != NULL) {
                    commands[num_commands++] = token;
                    token = strtok(NULL, "|");
                }

                execute_piped_commands(commands, num_commands);
            }
            
            else { // Sequential commands
                char *token = strtok(line, ";");
                while (token != NULL) {
                    commands[num_commands++] = token;
                    token = strtok(NULL, ";");
                }

                execute_sequential_commands(commands, num_commands);
            }
        }

        
        fclose(script_file);//Close the script file.
    }
    else{//This piece of code is for executing the program like "./script" without any arguments. It will take command until quit command.
        char line[MAX_LINE_LENGTH];
        while (1) {
            printf(">> ");
            fflush(stdout);//Enter command

            if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
            }

            if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
            }

            if (strcmp(line, "quit") == 0) {//Handle the quit command.
            break;
            }

            if (strcmp(line, "history") == 0) {//Handle the history command.
            print_history();
            continue;
            }

            strcpy(history[history_count++], line);

            char *commands[MAX_COMMANDS];
            int num_commands = 0;

            if (strchr(line, '|') != NULL) { //First check for string. If it contains "|", then use execute_piped_commands() function.
                char *token = strtok(line, "|");
                while (token != NULL) {
                    commands[num_commands++] = token;
                    token = strtok(NULL, "|");
                }

                execute_piped_commands(commands, num_commands);
            }
            
            else { //If it does not contain "|" but contains ";", then use execute_sequential_commands() function.
                char *token = strtok(line, ";");
                while (token != NULL) {
                    commands[num_commands++] = token;
                    token = strtok(NULL, ";");
                }

                execute_sequential_commands(commands, num_commands);
            }
        }
    }
    

    printf("Exiting shell.\n");//Exit the shell.
    return EXIT_SUCCESS;

    
    }
