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
    // for(int i=0; i<MAX_LIST; i++){
    //     parsed[i] = NULL;
    //     parsedRedir = NULL;
    // }
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
check if parent process should wait or not
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
        if(should_wait){
            printf("%s\n", parsed[0]);
            printf("waiting...\n");
            wait(NULL); 
        }
        return; 
    } 
}

int parse_pipe(char* line, char** parsed)
{
    return 0;
}

/*Parse command words*/
void parse_space(char* str, char* parsed[])
{
    for(int i=0; i<MAX_LIST; ++i){
        parsed[i] = strsep(&str, " ");
        if(parsed[i] == NULL)          //done parsing
            break;
        if(strlen(parsed[i]) == 0)   //ignore space
            i--;
        
    }
}

/*
parse argument for redirect command
*/
void parse_redir(char* parsed[], char* parsedRedir[])
{
    parsedRedir[0] = NULL;
    parsedRedir[1] = NULL;
    if(parsed[1]== NULL) return;

    if(strcmp(parsed[1], ">")==0 || strcmp(parsed[1], "<")==0){ //found ">" "<" in command
        if(parsed[2]!=NULL){   //filename exit
            parsedRedir[0] = parsed[1];
            parsedRedir[1] = parsed[2];
            parsed[1] = NULL;
            parsed[2] = NULL;
            
            if(strcmp(parsed[3], "&")==0)
                parsed[1] = parsed[3];
        }
        //else if filename doesn't exit then will raise error when exec command
    }
}

int main(void){
    char line[MAX_LENGTH];
    char history[MAX_LENGTH] = "";
    char* parsedArgs[MAX_LIST];
    char* parsedRedirArgs[MAX_LIST];

    while(true){       
        type_prompt(line, parsedArgs, parsedRedirArgs);      //display prompt on screen
        if(!get_input(line, history))     //get input from screen
            continue;  
        parse_space(line, parsedArgs);
        parse_redir(parsedArgs, parsedRedirArgs);  //check and parse arg for redirection cmd
        exec_args(parsedArgs, parsedRedirArgs);
    }
    return 0;
}
