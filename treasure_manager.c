#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define MAX_NAME 32
#define MAX_CLUE 128
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define TREASURE_SIZE sizeof(struct Treasure)

typedef struct Treasure{
    char id[MAX_NAME];
    char user[MAX_NAME];
    double latitude;
    double longitude;
    char clue[MAX_CLUE];
    int value;
}Treasure;

void log_action(const char *hunt_id, const char *action) 
{   
    //construim calea/path
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,TREASURE_FILE);
    int fis=open(path, O_WRONLY | O_APPEND | O_CREAT,0644);
    if(fis==-1)
    {
        perror("Eroare deschidere fisier");
        exit(-1);
    }
    char buffer[256];
    strcpy(buffer,action);
    strcat(buffer,"\n");
    write(fis,buffer,strlen(buffer));
    close(fis);
    // Creeaza legatura simbolica
    char link_name[64];
    strcpy(link_name,"logged_hunt-");
    strcat(link_name,hunt_id);
    symlink(path,link_name); 
}
void add_treasure(const char *hunt_id) 
{   
    //creez director nou
    mkdir(hunt_id,0755);
    //Construim calea/path
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,TREASURE_FILE);
    //Citim datele de la tastatura
    Treasure t;
    printf("Treasure ID: ");
    scanf("%s",t.id);
    printf("User name: ");
    scanf("%s",t.user);
    printf("Latitude: ");
    scanf("%lf",&t.latitude);
    printf("Longitude: ");
    scanf("%lf",&t.longitude);
    getchar(); //consuma\n
    printf("Clue: ");
    fgets(t.clue,MAX_CLUE,stdin);
    t.clue[strcspn(t.clue,"\n")]='\0'; //elimina\n
    printf("Value: ");
    scanf("%d",&t.value);
    int fis=open(path, O_WRONLY | O_APPEND | O_CREAT,0644);
    if(fis==-1) 
    {
        perror("Eroare la deschidere treasure file");
        exit(-1);
    }
    write(fis,&t,TREASURE_SIZE);
    close(fis);
    log_action(hunt_id,"Am adaugat comoara");
}
void list_treasures(const char *hunt_id) 
{
    //Construim calea
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,TREASURE_FILE);
    struct stat st;
    if(stat(path,&st)==-1) 
    {
        perror("Treasure file stat");
        exit(-1);
    }
    printf("Hunt: %s\n",hunt_id);
    printf("Size: %lld bytes\n",st.st_size);
    printf("Last modified: %s",ctime(&st.st_mtime));
    int fis=open(path, O_RDONLY);
    if(fis==-1)
    {
        perror("Eroare la deschidere fisier treasure file");
        exit(-1);
    }
    Treasure t;
    while(read(fis,&t,TREASURE_SIZE)==TREASURE_SIZE) 
    {
        printf("ID: %s | User: %s | Lat: %.2f | Lon: %.2f | Value: %d\n",t.id,t.user,t.latitude,t.longitude,t.value);
        printf("Clue: %s\n\n",t.clue);
    }
    close(fis);
    log_action(hunt_id,"Listez comori");
}
void view_treasure(const char *hunt_id, const char *id) 
{   
    //Construim calea
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,TREASURE_FILE);
    int fis=open(path,O_RDONLY);
    if(fis==-1) 
    {
        perror("Eroare deschidere fisier treasure file");
        exit(-1);
    }
    Treasure t;
    while(read(fis,&t,TREASURE_SIZE)==TREASURE_SIZE) 
    {
        if(strcmp(t.id,id)==0)
        {
            printf("Treasure Found:\nID: %s\nUser: %s\nLat: %.2f\nLon: %.2f\nClue: %s\nValue: %d\n",t.id, t.user, t.latitude, t.longitude, t.clue, t.value);
            log_action(hunt_id,"Viewed treasure");
            close(fis);
            exit(-1);
        }
    }
    printf("Comoara nu exista\n");
    close(fis);
}
void remove_treasure(const char *hunt_id, const char *id) 
{   
    //Construim calea
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,TREASURE_FILE);
    int fis=open(path,O_RDWR);
    if(fis==-1) 
    {
        perror("Eroare la deschidere fisier treasure file");
        exit(-1);
    }
    int temp_fis=open("temp.dat", O_WRONLY | O_CREAT | O_TRUNC,0644);
    if(temp_fis==-1) 
    {
        perror("Eroare deschidere fisier auxiliar temp file");
        close(fis);
        exit(-1);
    }
    Treasure t;
    int found=0;
    while(read(fis,&t,TREASURE_SIZE)==TREASURE_SIZE)
    {
        if(strcmp(t.id,id)!=0) 
        {
            write(temp_fis,&t,TREASURE_SIZE);
        } 
        else 
        {
            found=1;
        }
    }
    close(fis);
    close(temp_fis);
    if(found) 
    {
        rename("temp.dat",path);
        printf("Comaoara stearsa\n");
        log_action(hunt_id,"Eliminam comoara");
    } 
    else
    {
        printf("Comoara inexistenta\n");
        remove("temp.dat");
    }
}
void remove_hunt(const char *hunt_id) 
{
    //Construim calea
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,TREASURE_FILE);
    unlink(path);
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,LOG_FILE);
    unlink(path);
    rmdir(hunt_id);
    printf("Vanatoare eliminata\n");
    char link_name[64];
    strcpy(link_name,"logged_hunt-");
    strcat(link_name,hunt_id);
    unlink(link_name);
}
int main(int argc,char *argv[])
{
    if(argc<3) 
    {   
        printf("Eroare la introducere numar argumente:\n");
        printf("Folositi una din comenzile:\n");
        printf("%s --add <hunt_id>\n",argv[0]);
        printf("%s --list <hunt_id>\n",argv[0]);
        printf("%s --view <hunt_id> <id>\n",argv[0]);
        printf("%s --remove_treasure <hunt_id> <id>\n",argv[0]);
        printf("%s --remove_hunt <hunt_id>\n",argv[0]);
        exit(-1);
    }
    if(strcmp(argv[1],"--add")==0)
        add_treasure(argv[2]);
    else if(strcmp(argv[1],"--list")==0)
        list_treasures(argv[2]);
    else if(strcmp(argv[1],"--view")==0 && argc==4)
        view_treasure(argv[2], argv[3]);
    else if(strcmp(argv[1],"--remove_treasure")==0 && argc==4)
        remove_treasure(argv[2],argv[3]);
    else if(strcmp(argv[1],"--remove_hunt")==0)
        remove_hunt(argv[2]);
    else{
        printf("Parametrii invalizi\n");
    }
    
    return 0;
}