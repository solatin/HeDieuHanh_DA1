#include<stdio.h> 
#include<string.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
//#include<readline/readline.h> 
//#include<readline/history.h> 

using namespace std;
#define MAX_LENGTH 80
#define MAX_LIST 5

void type_prompt()
{
    static int first_time = 1;
    if(first_time){ //clear screen for the first time
        const char* CLEAR_SCREEN_ANSI = " \e[1;1H\e[2J";
        write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
        first_time = 0;
    }
    printf("ssh>");
    fflush(stdout);
}


int get_input(char* line)
{
    int count = 0;
    
    //Read one line
    while(true){
        int c=fgetc(stdin);
        line[count++] = (char)c;
        if(c=='\n') break;
    }
    if (count!=1) {     //strlen(line) != 0
        line[count-1] = 0;        //add NULL
        //add_history(line);
        return 0;
    }
    
    return 1;
}

/* Execute system command*/
void exec_args(char** parsed) 
{ 
    int should_wait = 1;
    //check if whether parent process should wait or not
    for(int i=0; i<MAX_LIST; ++i){
        if(parsed[i] == NULL)
            break;
        if (strcmp(parsed[i], "&") == 0){
            should_wait = 0;
            
            memset(parsed[i], 0, 4);   //delete "&" from parsed
        }
        
    }

    // Forking a child
    pid_t pid = fork(); 
    if (pid == -1) { 
        printf("\nFailed forking child.."); 
        return; 
    } 
    else if (pid == 0) {        //child process
        if(execvp(parsed[0], parsed) < 0){
            printf("\n Could not execute command..");
        }
        exit(0);
    }
    else if (pid>0) {                 //parent process
        if(should_wait){
            printf("Waiting...\n");
            wait(NULL); 
        }
        return; 
    } 
}

int parse_pipe(char* line, char** parsed)
{
    return 0;
}

void parse_space(char* str, char** parsed)
{
    int i;

    for(i=0; i<MAX_LIST; ++i){
        parsed[i] = strsep(&str, " ");
        if(parsed[i] == NULL)          //done parsing
            break;
        if(strlen(parsed[i]) == 0)   //space at begin of string
            i--;
    }
}
int main(void){
    char line[MAX_LENGTH];
    char* parsedArgs[MAX_LIST];
    while(true){       
        type_prompt();   //display prompt on screen
        if(get_input(line))
            continue;  //read input from terminal
        parse_space(line, parsedArgs);    //execute command
        exec_args(parsedArgs);
        
    }
    return 0;
}
