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

/*Printf "shh>" to the screen*/
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

/*Read input from screen*/
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
    if (count != 1) //strlen(line) != 0
    {
        line[count - 1] = '\0';        //add NULL
        if (strcmp(line, "!!") == 0)    // neu nhap !! de truy cap lich su
        {
            if (strcmp(history, "")==0) // neu khong co lenh gan day trong lich su
            {
                printf("No command in history..\n");
                return 1;
            }
            else
            {      
                strcpy(line, history);
                printf("ssh>%s\n",line);
                return 0;
            }
        }
        strcpy(history, line);
        return 0;
    }
    
    //strlen(line) = 0, user did not enter anything

    return 1;
}
/* Execute system command*/
void exec_args(char** parsed) 
{ 
    int should_wait = 1;
    //check if parent process should wait or not
    for(int i=0; i<MAX_LIST; ++i){
        if(parsed[i] == NULL)
            break;
        if (strcmp(parsed[i], "&") == 0){
            should_wait = 0;
            parsed[i] = NULL; //delete "&" from parsed
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
            printf("\n Could not execute command..\n");
        }
        exit(0);
    }
    else if (pid>0) {                 //parent process
        if(should_wait){
            printf("waiting...\n");
            wait(NULL); 
        }
        return; 
    } 
}

/*Parse command words*/
void parse_space(char* str, char** parsed)
{
    int i;

    for(i=0; i<MAX_LIST; ++i){
        parsed[i] = strsep(&str, " ");
        if(parsed[i] == NULL)          //done parsing
            break;
        if(strlen(parsed[i]) == 0)   //ignore space
            i--;
    }
}

void exec_pipe(char** parsed, char** parsedpipe) 
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
        // It only needs to write at the write end 
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO); 
        close(pipefd[1]); 
  
        if (execvp(parsed[0], parsed) < 0) { 
            printf("\nCould not execute command 1.."); 
            exit(0); 
        } 
    } else { 
        // Parent executing 
        p2 = fork(); 
  
        if (p2 < 0) { 
            printf("\nCould not fork.."); 
            return; 
        } 
  
        // Child 2 executing.. 
        // It only needs to read at the read end 
        if (p2 == 0) { 
            close(pipefd[1]); 
            dup2(pipefd[0], STDIN_FILENO); 
            close(pipefd[0]); 
            if (execvp(parsedpipe[0], parsedpipe) < 0) { 
                printf("\nCould not execute command 2.."); 
                exit(0); 
            } 
        } else { 
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
    while(true){       
        type_prompt();      //display prompt on screen

        if(get_input(line, history)) //read input from terminal
            continue;  
        parse_space(line, parsedArgs);    //execute command
        exec_args(parsedArgs);
        
    }
    return 0;
}