#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include <fcntl.h>

using namespace std;
#define MAX_LENGTH 80
#define MAX_LIST 5
#define INT_TYPE 1
#define OUT_TYPE 2

/*
Clear memory and
Printf "shh>" to the screen
*/
void type_prompt(char* line, char* parsed[], char* parsedRedir[])
{
    static int first_time = 1;
    if(first_time){ //clear screen for the first time
        const char* CLEAR_SCREEN_ANSI = " \e[1;1H\e[2J";
        write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
        first_time = 0;
    }
    
    memset(line, '\0', MAX_LENGTH);
    printf("ssh>");
    fflush(stdout);
}

/*Read input from screen
  return 1 if success, 0 if not
*/
int get_input(char* line, char* history)
{
    int count = 0;
    
    //Read one line
    while (true)
    {
        int c = fgetc(stdin);
        line[count++] = (char)c;
        if (c=='\n')
            break;
    }

    if(strlen(line)==1)     //user did not enter anything
        return 0;      
    
    line[count - 1] = '\0';        //add NULL
    if (strcmp(line, "!!") == 0)    // found "!!"
    {
        if (strcmp(history, "")==0) // there no command in history
        {
            printf("No command in history..\n");        //continue
            return 0;       //get input not success
        }
        else
        {      
            strcpy(line, history);
            printf("ssh>%s\n",line);
            return 1;       //get input success
        }
    }
    strcpy(history, line);
    return 1;
}

 /*
Check if parent process should wait or not
return 1 if wait, 0 if not
*/
int check_wait(char* parsed[])
{
   
    for(int i=0; i<MAX_LIST; ++i){
        if(parsed[i] == NULL)
            break;
        if (strcmp(parsed[i], "&") == 0){
            parsed[i] = NULL; //delete "&" from parsed
            return 0;
        }
    }
    return 1;
}

void parent(pid_t child_pid, int should_wait) {
   int status;
   printf("Parent <%d> spawned a child <%d>.\n", getpid(), child_pid);
    if(should_wait){
        // Parent waits for child process with PID to be terminated
        waitpid(child_pid, &status, WUNTRACED);
        if (WIFEXITED(status)) {   
            printf("Child <%d> exited with status = %d.\n", child_pid, status);
        }
    }
    else
      // Parent and child are running concurrently
        waitpid(child_pid, &status, 0);
}

/* Execute system command*/
void exec_args(char* parsed[], char* parsedRedir[]) 
{ 
    int should_wait = check_wait(parsed);

    // Forking a child
    pid_t pid = fork(); 
    if (pid == -1) { 
        printf("\nFailed forking child.."); 
        return; 
    } 
    else if (pid == 0) {        //child process
        //handle redirection command
        if(parsedRedir[0] != NULL){
            if(strcmp(parsedRedir[0], ">")==0){
                int fd_out = creat(parsedRedir[1], S_IRWXU);
                if(fd_out == -1){
                    printf("Redirect output failed");
                    exit(0);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            else if(strcmp(parsedRedir[0], "<")==0){
                int fd_in = open(parsedRedir[1], O_RDONLY);
                if(fd_in == -1){
                    printf("Redirect input failed");
                    exit(0);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
        }
        //Exec command
        if(execvp(parsed[0], parsed) < 0){
            printf("\n Could not execute command..\n");
        }
        exit(0);
    }
    else if (pid>0) {                 //parent process
        parent(pid, should_wait); 
    } 
}




/*
Seperate line into words by space
Return 1 if there no words found, otherwise 0
*/
int parse_space(char* str, char** parsed)
{
    int i;
    for(i=0; i<MAX_LIST; i++){
        parsed[i] = strsep(&str, " ");
        if(parsed[i] == NULL)          //done parsing
            break;
        if(strlen(parsed[i]) == 0)   //ignore space
            i--;
    }
    if(parsed[0] == NULL) return 1;
    return 0;

}

/*
parse argument for redirect command
*/
void parse_redir(char* parsed[], char* parsedRedir[])
{
    parsedRedir[0] = NULL;
    parsedRedir[1] = NULL;
    if(parsed[0] == NULL || parsed[1]== NULL) return;
    
    
    if(strcmp(parsed[1], ">")==0 || strcmp(parsed[1], "<")==0){ //found ">" "<" in command
        if(parsed[2]!=NULL){   //filename exit
            parsedRedir[0] = parsed[1];
            parsedRedir[1] = parsed[2];
            parsed[1] = NULL;
            parsed[2] = NULL;
            
            if(parsed[3]!=NULL && strcmp(parsed[3], "&")==0)        
                parsed[1] = parsed[3];
        }
        //else if filename doesn't exit then will raise error when exec command
    }
}

/*
Seperate line by "|"
Return 0 if no pipe is found, otherwise 1
*/
int parse_pipe(char* str, char* strpiped[]) 
{ 
    int i; 
    for (i = 0; i < 2; i++) { 
        strpiped[i] = strsep(&str, "|"); 
        if (strpiped[i] == NULL) 
            break; 
    } 
  
    if (strpiped[1] == NULL) 
        return 0;
    else { 
        return 1; 
    } 
} 

/*
Seperate line into arguments
Return 1 if normal command
       2 if pipe command
       0 if there error in parsing
*/
int process_string(char* line, char* parsedArgs[], char* parsedRedirArgs[], char* parsedPipeArgs[])
{
    char* strpiped[2];
    int flag_pipe = 0;
    int flag_no_words=0;          //if there any case parsing space found no word
    flag_pipe = parse_pipe(line, strpiped);

    if(flag_pipe){
        flag_no_words+=parse_space(strpiped[0], parsedArgs);
        flag_no_words+=parse_space(strpiped[1], parsedPipeArgs);
    }
    else{
        flag_no_words+=parse_space(line, parsedArgs);
        parse_redir(parsedArgs, parsedRedirArgs);
    }
    if(flag_no_words>0){
        printf("Error!\n");   
        return 0;
    }
    return 1+flag_pipe;
}

// Execute builtin commands 
int exec_owncmd(char* parsed[]) 
{ 
    if(strcmp(parsed[0], "exit")==0)
        exit(0); 
    if(strcmp(parsed[0], "cd")==0){
        chdir(parsed[1]); 
        return 1; 
    }
    return 0; 
} 

/*
Execute command with pipe
*/
void exec_pipe(char* parsed[], char* parsedpipe[]) 
{ 
    // 0 is read end, 1 is write end 
    int pipefd[2];  
    pid_t p1, p2; 
  
    if (pipe(pipefd) < 0) { 
        printf("\nPipe could not be initialized.."); 
        return; 
    } 
    p1 = fork(); 
    if (p1 < 0) { 
        printf("\nCould not fork.."); 
        return; 
    } 
  
    if (p1 == 0) { 
        // Child 1 executing.. 

        dup2(pipefd[1], STDOUT_FILENO); 
        close(pipefd[0]); 
        close(pipefd[1]); 
  
        if (execvp(parsed[0], parsed) < 0) { 
            printf("\nCould not execute command 1.."); 
            exit(0); 
        } 
    } else { 
        // Parent executing 
        //Create 2nd child
        p2 = fork(); 
  
        if (p2 < 0) { 
            printf("\nCould not fork.."); 
            return; 
        } 
  
        // Child 2 executing.. 
        if (p2 == 0) { 
            dup2(pipefd[0], STDIN_FILENO); 
            close(pipefd[1]); 
            close(pipefd[0]); 
            if (execvp(parsedpipe[0], parsedpipe) < 0) { 
                printf("\nCould not execute command 2.."); 
                exit(0); 
            } 
        } else { 
            close(pipefd[0]);
            close(pipefd[1]);
            // parent executing, waiting for two children 
            wait(NULL); 
            wait(NULL); 
        } 
    } 
}



int main(void){
    char line[MAX_LENGTH];
    char history[MAX_LENGTH] = "";
    char* parsedArgs[MAX_LIST];
    char* parsedRedirArgs[MAX_LIST];
    char* parsedPipeArgs[MAX_LIST];
    int execFlag=1;

    while(true){       
        type_prompt(line, parsedArgs, parsedRedirArgs);      //display prompt on screen
        if(!get_input(line, history))     //get input from screen
            continue;  
        execFlag = process_string(line, parsedArgs, parsedRedirArgs, parsedPipeArgs);

        if(execFlag==0)
            continue;
        if(execFlag == 1){
            if(exec_owncmd(parsedArgs))
                continue;
            exec_args(parsedArgs, parsedRedirArgs);
        }
        if(execFlag == 2)
            exec_pipe(parsedArgs, parsedPipeArgs);
    }
    return 0;
}
