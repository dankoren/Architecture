#include "LineParser.c"
#include <unistd.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INPUT_MAX 2048
#define SYS_OPEN 2
#define STDIN 0
#define STDOUT 1
#define MAX_PROCESSES 5000

int execute(cmdLine* cmdline){
  if(cmdline->outputRedirect!=NULL){
    fclose(stdout);
    fopen(cmdline->outputRedirect,"a");
  }
  if(cmdline->inputRedirect!=NULL){
    fclose(stdin);
    fopen(cmdline->inputRedirect,"w");
  }
  return execvp(cmdline->arguments[0],cmdline->arguments);
}

void childProcess(cmdLine* cmdline){
  if(cmdline != NULL){
    if(execute(cmdline) == -1){
        perror("Error in execute");
        _exit(2);
    }
  }
}

void switch_fd(int replaced, int replacer){
    close(replaced);
    dup(replacer);
    close(replacer);
}


int main(){
  char path [PATH_MAX];
  int pid;
  int isTerminated = 0;
  char input [INPUT_MAX];
  int fd [2];
  cmdLine* cmdline;
  while (!isTerminated){
    getcwd(path,PATH_MAX);
    printf("%s> ",path);
    fgets(input,INPUT_MAX,stdin);
    cmdline = parseCmdLines(input);
    if(cmdline->next!=NULL) {
        pipe(fd);
    }
    if(strcmp(cmdline->arguments[0],"quit") == 0){
        isTerminated = 1;
    }
    pid = fork();
    if(!isTerminated && pid == 0){ //Child Process
        if(cmdline->next != NULL){
            switch_fd(STDOUT,fd[1]);
        }
        childProcess(cmdline);
    }
    else{ //Parent Process
      if(cmdline->next!= NULL){ //Case there is pipe
        close(fd[1]);
        int pid2 = fork();
        if(pid2==0){ // Child 2 process
          switch_fd(STDIN,fd[0]);
          childProcess(cmdline->next);
        }
        else{ // Parent Process
          close(fd[0]);
          int status = 0;
          waitpid(pid, &status, 0);
          waitpid(pid2, &status, 0);
          if(WEXITSTATUS(status) == 2){
            freeCmdLines(cmdline);
            _exit(2);
          }
        }
      }
      else{
        if(cmdline->blocking!=0){
          int status = 0;
          waitpid(pid,&status,0);
          if(WEXITSTATUS(status) == 2){
            freeCmdLines(cmdline);
            _exit(2);
          }
        }
      }
    }
    freeCmdLines(cmdline);
  }
  
  return 1;
}

