#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_NAME 32
#define MAX_CLUE 128
#define TREASURE_FILE "treasures.dat"
#define TREASURE_SIZE sizeof(Treasure)

typedef struct Treasure{
    char id[MAX_NAME];
    char user[MAX_NAME];
    double latitude;
    double longitude;
    char clue[MAX_CLUE];
    int value;
}Treasure;

typedef struct UserScore 
{
    char user[MAX_NAME];
    int score;
    struct UserScore *next;
}UserScore;

UserScore* find_or_create(UserScore **head, const char *user) 
{
    UserScore *cur=*head;
    while(cur)
    {
        if(strcmp(cur->user,user)==0)
            return cur;
        cur=cur->next;
    }

    UserScore *new_node = malloc(sizeof(UserScore));
    strcpy(new_node->user,user);
    new_node->score=0;
    new_node->next=*head;
    *head=new_node;
    return new_node;
}
void free_scores(UserScore *head) 
{
    while(head)
    {
        UserScore *tmp=head;
        head=head->next;
        free(tmp);
    }
}
int main(int argc, char *argv[]) 
{
    if(argc < 2) 
    {
        fprintf(stderr,"Usage:%s <hunt_id>\n",argv[0]);
        return 1;
    }
    char path[256];
    snprintf(path,sizeof(path),"%s/%s",argv[1],TREASURE_FILE);
    int fd=open(path,O_RDONLY);
    if(fd==-1)
    {
        perror("Failed to open treasure file");
        return 1;
    }
    Treasure t;
    UserScore *scores=NULL;
    while(read(fd,&t,TREASURE_SIZE)==TREASURE_SIZE) 
    {
        UserScore *user_score=find_or_create(&scores,t.user);
        user_score->score+=t.value;
    }
    if(scores == NULL)
    {
        printf("No treasures found.\n");
    }
    close(fd);
    UserScore *cur=scores;
    while(cur) 
    {
        printf("%s: %d\n",cur->user,cur->score);
        cur=cur->next;
    }
    free_scores(scores);
    fflush(stdout);
    close(fd); 

    return 0;
}