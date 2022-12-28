#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

//accept up to 16 command-line arguments
#define MAXARG 16

//allow up to 64 environment variables
#define MAXENV 64

//keep the last 500 commands in history
#define HISTSIZE 500

//accept up to 1024 bytes in one command
#define MAXLINE 1024

static char **parseCmd(char cmdLine[]) {
    char **cmdArg, *ptr;
    int i;

    //(MAXARG + 1) because the list must be terminated by a NULL ptr
    cmdArg = (char **) malloc(sizeof(char *) * (MAXARG + 1));
    if (cmdArg == NULL) {
        perror("parseCmd: cmdArg is NULL");
        exit(1);
    }
    for (i = 0; i <= MAXARG; i++) {
        cmdArg[i] = NULL;
    }
    i = 0;
    ptr = strsep(&cmdLine, " ");
    while (ptr != NULL) {
        cmdArg[i] = (char *) malloc(sizeof(char) * (strlen(ptr) + 1));
        if (cmdArg[i] == NULL) {
        perror("parseCmd: cmdArg[i] is NULL");
        exit(1);
        }
        strcpy(cmdArg[ i++ ], ptr);
        if (i == MAXARG)
        break;
        ptr = strsep(&cmdLine, " ");
    }
    return(cmdArg);
}

// I am including an extra parameter for the main method which will deal with the 
// environment commands like env, setenv, unsetenv. These commands are necessary 
// because it makes use of the environment variables. When a shell program (bash, 
// or bsh in this case) starts it takes three parameters.

int main(int argc, char *argv[], char *envp[MAXENV]) {
    char cmdLine[MAXLINE], **cmdArg;
    int status, i, debug;
    pid_t pid;
    // Variables needed for the separation of a string with the strsep(). I
    // needed twice the amount of these variables because of nested ifs and loops.
    char tmpStr[MAXLINE], *myPath, *justPATH, *justPATH1, *myPath1, tmpStr1[MAXLINE];
    // Useful for the history command. I have included an array that contains 500 
    // elements. I have also included a variable that will keep count of all the 
    // commands given by the user. This count will be helpful to copy the commands
    // in a certain place in the array. 
    char *historyArray[500];
    int history_count = -1;
    // k is a variable needed for managing the environment commands
    int k;
    // This is an array in which I will copy the environment variables. The size
    // of the array will be 64 because this is the maximum number of variables in the
    // environment that we will assume we have.
    char *myEnv[MAXENV];
    // This array is needed in order to include the result of calling fgetc().
    char cmdstuff[MAXLINE];
    // I am initializing the history array to NULL value for each index.
    for (k = 0; k < 500; k++) {
        historyArray[k] = NULL;
    }
    // It is important to create a copy of the environment variables. If we don't
    // make a copy of those environmental variables, we may come accross a lot of 
    // segmentation faults. At first, I am creating an array that will initially
    // contain NULL value in every single index.
    for (k = 0; k < MAXENV; k++) {
        myEnv[k] = NULL;
    }
    // Then, I make copies of the environment variables in the envp array to
    // the new array I initialized above. This is happening in order to prevent
    // future segmentation faults, as mentioned above. 
    for (k = 0; envp[k] != NULL; k++) {
        // Use of malloc to allocate space in memory for the strings I am copying 
        // from one array to another.
        myEnv[k] = malloc(strlen(&envp[k] + 1));
        strcpy(&myEnv[k], &envp[k]);
    }
  
    debug = 0;
    i = 1;
    while (i < argc) {
        if (! strcmp(argv[i], "-d") )
        debug = 1;
        i++;
    }
    while (( 1 )) {
        // The shell program bsh, starts
        printf("bsh> "); 
        //get a line from keyboard
        strcpy(cmdstuff, fgets(cmdLine, MAXLINE, stdin)); 
        //strip '\n'
        cmdLine[strlen(cmdLine) - 1] = '\0';  
        cmdArg = parseCmd(cmdLine);
        // every time a command is inserted, increase the history count
        history_count++;
        // malloc the history array at the index given by the history_count
        historyArray[history_count] = malloc(MAXLINE + 1);
        // copy the command given by the user to the array at the index
        // history_count
        strcpy(historyArray[history_count], cmdstuff);
        if (debug) {
            i = 0;
            while (cmdArg[i] != NULL) {
	            printf("\t%d (%s)\n", i, cmdArg[i]);
	            i++;
            }
        }
    
        // For the command exit
        if (strcmp(cmdArg[0], "exit") == 0) {
            if (debug) {
                printf("exiting\n");
            }
            break;
        }
        // For the command env
        else if (strcmp(cmdArg[0], "env") == 0) {
            if (debug == 0) {
                // Print all the enviroment variables that are in the system
                for (k = 0; myEnv[k] != NULL; k++) {
                    printf("%s\n", myEnv[k]);
                }
            }
        }
        // For the command setenv
        else if (strcmp(cmdArg[0], "setenv") == 0) {
            // In the case where setenv is followed by nothing, throw an error to let
            // the user know that setenv needs two arguments, with the first one
            // representing the name of the variable and the second one the value of
            // the variable.
            if (cmdArg[1] == NULL) {
                printf("%s", "setenv is always followed by two arguments.");
                printf("%s\n", " Please include a variable and its name.");
            } else {
                // In case the user has given the name of the variable, check if they
                // have also given its value. If the value is not included let the user
                // know that a value for the variable is necessary.
                if (cmdArg[2] == NULL) {
                    printf("%s", "The variable is missing its value. ");
                    printf("%s\n", "Please insert a value.");
                // Otherwise, include the variable in the list with the rest of the
                // environment variables.
                } else {
                    for (k = 0; myEnv[k] != NULL; k++) {
                        // make a copy of the item k in the array myEnv
                        strcpy(tmpStr, myEnv[k]);
                        // put that copy in the variable myPath
                        myPath = tmpStr;
                        // use strsep() to split the myPath when it finds "=". This is an
                        // important separation in order to get the variable name and the
                        // value separately.
                        justPATH = strsep(&myPath, "=");
                        // If variable name given by the user is the same with a variable in
                        // the myEnv array then replace the old value of the variable with
                        // the new value which is contain in the cmdArg[2].
                        if (strcmp(cmdArg[1], justPATH) == 0) {
                            myEnv[k] = malloc(strlen(cmdArg[2]) + 1);
                            // concatenation is important because the myEnv array contains
                            // strings in which is in the form ABC=a/b/c and we need the new
                            // values and variables to also be in the same form. Thus, I am
                            // concatinating the "=" in the variable name, and then I am
                            // contatenating the value to the variablename=.
                            strcat(justPATH, "=");
                            strcat(justPATH, cmdArg[2]);
                            strcpy(myEnv[k], justPATH);
                        }
                    }
                    // This for loop does what the previous one is doing. However, in this
                    // case, I am inserting a new variable in the array myEnv rather
                    // than just updating the value of an already existing variable.
                    for (k = 0; myEnv[k] != NULL; k++) {
                        // split again variable name from equality and the value
                        strcpy(tmpStr, myEnv[k]);

                        myPath = tmpStr;
                        justPATH = strsep(&myPath, "=");
                        // In the case were the variable name has not been found in the
                        // array, and you look at the next index which should be null,
                        // insert the new variable name concatenated with = and its value
                        // after the last not empty index of the array myEnv.
                        if (! (strcmp(cmdArg[1], justPATH) == 0) && (myEnv[k+1] == NULL)) {
                            myEnv[k+1] = malloc(MAXLINE + 1);
                            strcpy(justPATH, cmdArg[1]);
                            strcat(justPATH, "=");
                            strcat(justPATH, cmdArg[2]);
                            strcpy(myEnv[k + 1], justPATH);
                        }
                    }
                }
            }
        }
        // For the command unsetenv
        else if (strcmp(cmdArg[0], "unsetenv") == 0) {
            // In case the unsetenv doesn't have an argument following it, throw an
            // error to let the user know
            if (cmdArg[1] == NULL) {
                printf("%s", "A variable has not been included.");
                printf("%s\n", " Please include an existing one.");
            // Otherwise, just delete the variable from the myEnv array.
            } else {
                // As I did before, I go through the array, and do a split because I need
                // to check if the argument given by the user is the same as one of the
                // variable name in the myEnv array. When that variable name is found,
                // delete it from the array.
                for (k = 0; myEnv[k] != NULL; k++) {
                    strcpy(tmpStr, myEnv[k]);
                    myPath = tmpStr;
                    justPATH = strsep(&myPath, "=");
                    if (strcmp(cmdArg[1], justPATH) == 0) {
                        myEnv[k] = malloc(MAXLINE + 1);
                        // when the variable is found put in its index NULL
                        myEnv[k] = NULL;
                        // shift the variables that are following the variable that was
                        // just deleted.
                        while (myEnv[k+1] != NULL) {
                            myEnv[k] = malloc(MAXLINE + 1);
                            strcpy(myEnv[k], myEnv[k+1]);
                            k++;
                        }
                    }
                }
                // Included this for loop because with my implementation above, I end up
                // with a duplicate of the last variable before a NULL value. I don't
                // need two of those variables and since I had already shifted it
                // before, I can just replace it with NULL.
                for (k = 0; myEnv[k] != NULL; k++) {
                    if (myEnv[k+1] == NULL) {
                        myEnv[k] = malloc(MAXLINE + 1);
                        myEnv[k] == NULL;
                    }
                }
            }
        }
        // For the command cd
        else if (strcmp(cmdArg[0], "cd") == 0) {
            // First option is when the use is given the "cd" argument without any
            // following arguments
            if (cmdArg[1] == NULL) {
                // Go through the myEnv array, do a split between the variable names and
                // the values. When you find the variable HOME, call the method chdir()
                // that will change the directory you are in to be the home
                for (k = 0; myEnv[k] != NULL; k++) {
                    strcpy(tmpStr, myEnv[k]);
                    myPath = tmpStr;
                    justPATH = strsep(&myPath, "=");
                    if (strcmp(justPATH, "HOME") == 0){
                        chdir(myPath);
                        // Once you change the directory to the home directory, go through
                        // the myEnv array once again. This time we need to find the
                        // variable PWD and replace its values with the current path you are
                        // in, which is the path that leads to the home directory. For this
                        // part of the code, I needed some extra variables which are going to
                        // be doing the exact same thing with tmpStr, myPath, justPATH above.
                        for (int j = 0; myEnv[j] != NULL; j++) {
                            // split each one of the variables in the array
                            strcpy(tmpStr1, myEnv[j]);
                            myPath1 = tmpStr1;
                            justPATH1 = strsep(&myPath1, "=");
                            // Did you find PWD?
                            if (strcmp(justPATH1, "PWD") == 0) {
                                // Yes? Replace with the new path.
                                myEnv[j] = malloc(MAXLINE + 1);
                                strcpy(myEnv[j], justPATH1);
                                strcat(myEnv[j], "=");
                                // concatenate myPath because this is the variable that contains
                                // the value of HOME, not myPath1.
                                strcat(myEnv[j], myPath);
                            }
                        }
                    }
                }
                // Here is the case were after "cd" the user has input "~", which will
                // lead the user to the home directory, like in the previous if option. This
                // is dealth with the exact same way which a plain cd like above.
            } else if (strcmp(cmdArg[1], "~") == 0) {
                for (k = 0; myEnv[k] != NULL; k++) {
                    strcpy(tmpStr, myEnv[k]);
                    myPath = tmpStr;
                    justPATH = strsep(&myPath, "=");
                    if (strcmp(justPATH, "HOME") == 0){
                        chdir(myPath);
                        for (int j = 0; myEnv[j] != NULL; j++) {
                            strcpy(tmpStr1, myEnv[j]);
                            myPath1 = tmpStr1;
                            justPATH1 = strsep(&myPath1, "=");
                            if (strcmp(justPATH1, "PWD") == 0) {
                                myEnv[j] = malloc(MAXLINE + 1);
                                strcpy(myEnv[j], justPATH1);
                                strcat(myEnv[j], "=");
                                strcat(myEnv[j], myPath);
                            }
                        } 
                    }
                }
            // Here we have the option where the user has actually inserted a path
            // with directories.
            } else if (cmdArg[1] != NULL) {
                // if the directory is not found, I throw an error to let the user know
                // that the directory doesn't exist.
                if (chdir(cmdArg[1]) == -1) {
                    printf("%s", "The ");
                    printf("%s", cmdArg[1]);
                    printf("%s\n", " directory doesn't exist!");
                // for the case that the user inserted ".." after cd. Go to the parent
                // directory
                } else if (strcmp(cmdArg[1], "..") == 0) {
                    // get the path of the parent directory 
                    myPath = getcwd(tmpStr, sizeof(tmpStr));
                    // go to the parent directory
                    chdir(myPath);
                    // Some of the following lines are the same with what we have 
                    // encountered in the case of cd and cd ~.
                    for (int j = 0; myEnv[j] != NULL; j++) {
                        strcpy(tmpStr1, myEnv[j]);
                        myPath1 = tmpStr1;
                        justPATH1 = strsep(&myPath1, "=");
                        if (strcmp(justPATH1, "PWD") == 0) {
                            myEnv[j] = malloc(MAXLINE + 1);
                            strcpy(myEnv[j], justPATH1);
                            strcat(myEnv[j], "=");
                            strcat(myEnv[j], myPath);
                        }
                    }
                // here is the option of having a "." after cd which doesn't change
                // anything for the directory. It just give you the same directory.
                } else if (strcmp(cmdArg[1], ".") == 0) {
                    myPath = getcwd(tmpStr, sizeof(tmpStr));
                    chdir(myPath);
                // Here starts the case where you either encounter a name of a
                // directory, a path, or something like ./someDir or ../someDir.
                } else {
                    // Separate the given path according the first / you encounter.
                    strcpy(tmpStr, cmdArg[1]);
                    myPath = tmpStr;
                    justPATH = strsep(&myPath, "/");
                    // if the string before the "/" is a dot, then you are staying the in
                    // same directory because of the . and you move to the directory
                    // following "/".
                    if (strcmp(justPATH, ".") == 0) {
                        chdir(myPath);
                        // get the Path for the directory that is after "/"
                        myPath = getcwd(tmpStr, sizeof(tmpStr));
                        // Insert the new Path in the PWD variable in the myEnv array like
                        // you have done with all the other cases before.
                        for (int j = 0; myEnv[j] != NULL; j++) {
                            strcpy(tmpStr1, myEnv[j]);
                            myPath1 = tmpStr1;
                            justPATH1 = strsep(&myPath1, "=");
                            if (strcmp(justPATH1, "PWD") == 0) {
                                myEnv[j] = malloc(MAXLINE + 1);
                                strcpy(myEnv[j], justPATH1);
                                strcat(myEnv[j], "=");
                                strcat(myEnv[j], myPath);
                            }
                        }
                        // if the string before the "/" is a "..", then you are going to the
                        // parent directory because of the .. and you move to the directory
                        // following "/".
                    } else if(strcmp(justPATH, "..") == 0) {
                        // get the path for the parent directory
                        justPATH = getcwd(tmpStr, sizeof(tmpStr));
                        // go to the directory that is the parent and the one following the
                        // "/"
                        chdir(myPath);
                        // Insert the new Path in the PWD variable in the myEnv array like
                        // you have done with all the other cases before.
                        for (int j = 0; myEnv[j] != NULL; j++) {
                            strcpy(tmpStr1, myEnv[j]);
                            myPath1 = tmpStr1;
                            justPATH1 = strsep(&myPath1, "=");
                            if (strcmp(justPATH1, "PWD") == 0) {
                                myEnv[j] = malloc(MAXLINE + 1);
                                strcpy(myEnv[j], justPATH1);
                                strcat(myEnv[j], "=");
                                strcat(myEnv[j], justPATH);
                            }
                        }
            
                    } else {
                        // This is the case that the user is giving the name of a directory
                        // or a path with no ./ or ../
                        myPath = getcwd(tmpStr, sizeof(tmpStr));
                        chdir(myPath1);
                        // Insert the new Path in the PWD variable in the myEnv array like
                        // you have done with all the other cases before.
                        for (int j = 0; myEnv[j] != NULL; j++) {
                            strcpy(tmpStr1, myEnv[j]);
                            myPath1 = tmpStr1;
                            justPATH1 = strsep(&myPath1, "=");
                            if (strcmp(justPATH1, "PWD") == 0) {
                                myEnv[j] = malloc(MAXLINE + 1);
                                strcpy(myEnv[j], justPATH1);
                                strcat(myEnv[j], "=");
                                strcat(myEnv[j], myPath);
                            }
                        }
                    }
                }
            }
        }
        // For the command history
        else if (strcmp(cmdArg[0], "history") == 0) {
            // The following for loop is printing the history array that contains all
            // the commands the user has called.
            for (k = 0; k < history_count; k++) {
                printf("%d", k);
                printf("%s", " ");
                printf("%s", historyArray[k]);
            }
            // add history command in the array and print the result.
            historyArray[history_count] = malloc(MAXLINE + 1);
            strcpy(historyArray[history_count], cmdArg[0]);
            printf("%d", history_count);
            printf("%s", " ");
            printf("%s\n", historyArray[history_count]);
        }
        // Implemented how to execute Linux commands here
        else if (strcmp(cmdArg[0], "ls") == 0) {
            // At this part of my code, I am just implementing the linux command ls
            // (hardcoded), given the path where the actual executable is located.
            if (access("/usr/bin/ls", F_OK) == 0) {
                pid = fork();
                if (pid != 0) {
                    waitpid(pid, &status, 0);
                } else {
                    execv("/usr/bin/ls", cmdArg);
                    return 1;
                }
            }
        }
        //the following is a template for using fork() and execv()
        else {
            if (debug) {
                printf("calling fork()\n");
            }
            pid = fork();
            if (pid != 0) {
                if (debug) {
                    printf("parent %d waiting for child %d\n", getpid(), pid);
                }
                waitpid(pid, &status, 0);
            }
            else {
      	        status = execv(cmdArg[0], cmdArg);
      	        if (status) {
                    printf("\tno such command (%s)\n", cmdArg[0]);
      	            return 1;
                }
            }
        }
        i = 0;
        while (cmdArg[i] != NULL) {
            free(cmdArg[i++]);
        }
        free(cmdArg);
    }
    return 0;
}
