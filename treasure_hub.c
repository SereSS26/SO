#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

pid_t monitor_pid=-1;
int pipefd[2];
int monitor_exiting=0;

void sigchld_handler(int sig)
{
    int saved_errno=errno;  
    pid_t pid;
    while((pid=waitpid(-1,NULL,WNOHANG))>0)
    {
        if(pid==monitor_pid)
        {
            printf("[hub] Monitor terminated.\n");
            monitor_pid=-1;
            monitor_exiting=0;
        }
    }
    errno=saved_errno;
}
void send_command_to_monitor(const char *cmd) 
{
    FILE *f=fopen("cmd.txt","w");
    fprintf(f,"%s\n",cmd);
    fclose(f);
    kill(monitor_pid,SIGUSR1);

    char buffer[1024];
    int n=read(pipefd[0],buffer,sizeof(buffer)-1);
    if(n>0)
    {
        buffer[n]='\0';
        printf("[hub <- monitor]:\n%s",buffer);
    }
}
void calculate_score()
{
    DIR *d=opendir(".");
    if(!d)
    {
        perror("opendir");
        return;
    }
    struct dirent *entry;
    while((entry = readdir(d))!=NULL)
    {
        if(entry->d_type==DT_DIR && strcmp(entry->d_name,".")!=0 && strcmp(entry->d_name,"..")!=0)
        {
            char path[256];
            snprintf(path,sizeof(path),"%s/%s", entry->d_name,"treasures.dat");
            struct stat st;
            if(stat(path, &st)==0)
            {
                int pfds[2];
                pipe(pfds);
                pid_t pid = fork();
                if(pid==0)
                {
                    dup2(pfds[1],STDOUT_FILENO);
                    close(pfds[0]);
                    close(pfds[1]);
                    execl("./score_calculator", "score_calculator",entry->d_name,NULL);
                    perror("execl");
                    exit(1);
                } 
                else 
                {
                    close(pfds[1]);
                    char buf[1024];
                    printf("[score for %s]:\n", entry->d_name);
                    int n;
                    while ((n = read(pfds[0], buf, sizeof(buf) - 1)) > 0) 
                    {
                        buf[n] = '\0';
                        printf("%s", buf);
                    }
                    waitpid(pid, NULL, 0);  
                    close(pfds[0]);         
                }
            }
        }
    }
    closedir(d);
}
int main() 
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sigaction(SIGCHLD,&sa,NULL);
    char command[256];
    while(1) 
    {
        printf(">> ");
        fflush(stdout);
        if(!fgets(command,sizeof(command),stdin)) 
            break;
        command[strcspn(command,"\n")]=0;
        if(strcmp(command,"start_monitor")==0) 
        {
            if(monitor_pid!=-1)
            {
                printf("[hub] Monitor already running.\n");
                continue;
            }
            pipe(pipefd);
            monitor_pid=fork();
            if(monitor_pid==0)
            {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                execl("./treasure_manager","treasure_manager","--monitor",NULL);
                perror("execl monitor");
                exit(1);
            }
            close(pipefd[1]);
            printf("[hub] Monitor started (PID %d)\n", monitor_pid);
        } 
        else if(strcmp(command,"stop_monitor")==0)
        {
            if(monitor_pid==-1)
            {
                printf("[hub] Monitor not running.\n");
                continue;
            }
            kill(monitor_pid,SIGUSR2);
            monitor_exiting=1;
        } 
        else if(strcmp(command,"exit")==0)
        {
            if(monitor_pid!=-1)
            {
                printf("[hub] Cannot exit, monitor still running.\n");
                continue;
            }
            break;
        } 
        else if(strcmp(command,"calculate_score")==0)
        {
            calculate_score();
        } 
        else 
        {
            if(monitor_pid==-1||monitor_exiting)
            {
                printf("[hub] Error: monitor is not active or is terminating.\n");
                continue;
            }
            send_command_to_monitor(command);
        }
    }

    return 0;
}