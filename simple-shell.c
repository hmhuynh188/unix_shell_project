#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h> // is used with wait() 
#include <fcntl.h> // changes the properties of a file that is already open 

#define MAXLINE 80

void temp_max(char *args[],int argv) {
    int i = 0;
    while (args[i] != NULL && (i < argv)) {
        free(args[i]); 
        i++;
        if (i == 80) { 
          break;
        } 
    }
}

void read_user_command(char *args[], int *has_ampersand, int *argv) {

    char user_command[MAXLINE];
    int length = 0;
    char delimiter[] = " ";

    // read from STDIN 
    length = read(STDIN_FILENO, user_command, 80);

    if (user_command[length - 1] == '\n') {
        user_command[length - 1] = '\0';
    }

    if (strcmp(user_command, "!!") == 0) { // strcmp() is used to compare the string arguments
        if (*argv == 0) {
            printf("No commands in history.\n");
        }
        return;
    }
  
    if (strcmp(user_command, "exit()") == 0) { 
      exit(0); 
    }
  
    temp_max(args, *argv);
    *argv = 0;
    *has_ampersand = 0;
    char *ptr = strtok(user_command, delimiter);
    while (ptr != NULL) {
        if (ptr[0] == '&') {
            *has_ampersand = 1;
            ptr = strtok(NULL, delimiter);
            continue;
        }
        *argv += 1;
        args[*argv - 1] = strdup(ptr);
        ptr = strtok(NULL, delimiter);
    }
  
    args[*argv] = NULL;
}

int main(void) {
    char *args[MAXLINE / 2 + 1]; 
    int run = 1;
    pid_t pid;
    int has_ampersand = 0;
    int argv = 0;
    int use_pipe = 0;
    while (run) {
        use_pipe = 0;
        printf("osh> ");
        fflush(stdout);
        read_user_command(args, &has_ampersand, &argv);
        pid = fork();
        if (pid == 0) {
            if (argv == 0) {
                continue;
            } else {
                int redirectCase = 0;
                int file;
                
                for (int i = 1; i <= argv - 1; i++) {
                    if (strcmp(args[i], "<") == 0) {
                        file = open(args[i + 1], O_RDONLY);
                        if (file == -1 || args[i+1]  == NULL) {
                            printf("Invalid.\n");
                            exit(1);
                        }
                        dup2(file, STDIN_FILENO);
                        args[i] = NULL;
                        args[i + 1] = NULL;
                        redirectCase = 1;
                        break;
                    } else if (strcmp(args[i], ">") == 0) {
                        file = open(args[i + 1], O_WRONLY | O_CREAT, 0644);
                        if (file == -1 || args[i+1]  == NULL) {
                            printf("Invalid.\n");
                            exit(1);
                        }
                        dup2(file, STDOUT_FILENO);
                        args[i] = NULL;
                        args[i + 1] = NULL;
                        redirectCase = 2;
                        break;
                
                    } else if (strcmp(args[i], "|") == 0) {
                        use_pipe = 1;
                        int fd1[2];
                        if (pipe(fd1) == -1) {
                            fprintf(stderr, "Pipe Failed\n");
                            return 1;
                        }
                        
                        char *first[i + 1];
                        char *second[argv - i - 1 + 1];
                        for (int j = 0; j < i; j++) {
                            first[j] = args[j];
                        }
                        first[i] = NULL;
                        for (int j = 0; j < argv - i - 1; j++) {
                            second[j] = args[j + i + 1];
                        }
                        second[argv - i - 1] = NULL;

                        int pid_pipe = fork();
                        if (pid_pipe > 0) {
                            wait(NULL);
                            close(fd1[1]);
                            dup2(fd1[0], STDIN_FILENO);
                            close(fd1[0]);
                            if (execvp(second[0], second) == -1) {
                                printf("Invalid.\n");
                                return 1;
                            }

                        } else if (pid_pipe == 0) {
                            close(fd1[0]);
                            dup2(fd1[1], STDOUT_FILENO);
                            close(fd1[1]);
                            if (execvp(first[0], first) == -1) {
                                printf("Invalid.\n");
                                return 1;
                            }
                            exit(1);
                        }
                        close(fd1[0]);
                        close(fd1[1]);
                        break;
                    }
                }

                if (use_pipe == 0) {
                    if (execvp(args[0], args) == -1) {
                        printf("Invalid.\n");
                        return 1;
                    }
                }
                if (redirectCase == 1) {
                    close(STDIN_FILENO);

                } else if (redirectCase == 2) {
                    close(STDOUT_FILENO);
                }
                close(file);
            }
            exit(1);
        } else if (pid > 0) {
            if (has_ampersand == 0) wait(NULL);
        } else {
            printf("Error.");
        }
    }
}
