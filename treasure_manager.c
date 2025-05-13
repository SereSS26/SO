#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

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

volatile sig_atomic_t received_signal=0;

void sigusr1_handler(int sig) 
{
    received_signal=SIGUSR1;
}
void sigusr2_handler(int sig)
{
    received_signal=SIGUSR2;
}
void log_action(const char *hunt_id, const char *action) 
{   
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,LOG_FILE);
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
    char link_name[64];
    strcpy(link_name,"logged_hunt-");
    strcat(link_name,hunt_id);
    symlink(path,link_name); 
}
void add_treasure(const char *hunt_id) 
{   
    struct stat st={0};
    if(stat(hunt_id,&st)==-1)
    {
        if(mkdir(hunt_id, 0755)==-1)
        {
            perror("Eroare la creare director nou");
            exit(-1);
        }
    }
    char path[128];
    strcpy(path,hunt_id);
    strcat(path,"/");
    strcat(path,TREASURE_FILE);
    Treasure t;
    printf("Treasure ID: ");
    scanf("%s",t.id);
    printf("User name: ");
    scanf("%s",t.user);
    printf("Latitude: ");
    scanf("%lf",&t.latitude);
    printf("Longitude: ");
    scanf("%lf",&t.longitude);
    getchar();
    printf("Clue: ");
    fgets(t.clue,MAX_CLUE,stdin);
    t.clue[strcspn(t.clue,"\n")]='\0';
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
    char mesaj[256];
    sprintf(mesaj,"Am adaugat comoara cu id: %s",t.id);
    log_action(hunt_id,mesaj);
}
void list_treasures(const char *hunt_id) 
{
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
    printf("Ultima modificare: %s",ctime(&st.st_mtime));
    int fis=open(path,O_RDONLY);
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
    char mesaj[256];
    sprintf(mesaj,"Am listat comoarile");
    log_action(hunt_id,mesaj);
}
void view_treasure(const char *hunt_id, const char *id) 
{   
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
            printf("ID: %s\nUser: %s\nLat: %.2f\nLon: %.2f\nClue: %s\nValue: %d\n",t.id, t.user, t.latitude, t.longitude, t.clue, t.value);
            char mesaj[256];
            sprintf(mesaj,"Am vizualizat comoara cu id: %s",t.id);
            log_action(hunt_id,mesaj);
            close(fis);
            return;
        }
    }
    printf("Comoara nu exista\n");
    close(fis);
}
void remove_treasure(const char *hunt_id, const char *id) 
{   
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
        char mesaj[256];
        sprintf(mesaj,"Am eliminat comoara cu id: %s",t.id);
        log_action(hunt_id,mesaj);
    } 
    else
    {
        printf("Comoara inexistenta\n");
        remove("temp.dat");
    }
}
void remove_hunt(const char *hunt_id) 
{
    DIR *dir=opendir(hunt_id);
    if(!dir)
    {
        perror("Eroare la deschiderea directorului");
        return;
    }
    struct dirent *entry;
    char path[256];
    while((entry=readdir(dir))!=NULL) 
    {
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0)
            continue;
        strcpy(path,hunt_id);
        strcat(path,"/");
        strcat(path,entry->d_name);
        if(unlink(path)==-1) 
        {
            perror("Eroare la stergerea fisierului");
        }
    }
    closedir(dir);
    if(rmdir(hunt_id)==-1) 
    {
        perror("Eroare la stergerea directorului");
    } 
    else 
    {
        printf("Vanatoare eliminata\n");
    }
    char link_name[64];
    strcpy(link_name,"logged_hunt-");
    strcat(link_name,hunt_id);
    unlink(link_name);
}

int main(int argc,char *argv[])
{
    if(argc==2 && strcmp(argv[1],"--monitor")==0)
    {
        struct sigaction sa1,sa2;
        sa1.sa_handler=sigusr1_handler;
        sigemptyset(&sa1.sa_mask);
        sa1.sa_flags=0;
        sigaction(SIGUSR1,&sa1,NULL);

        sa2.sa_handler=sigusr2_handler;
        sigemptyset(&sa2.sa_mask);
        sa2.sa_flags=0;
        sigaction(SIGUSR2,&sa2,NULL);
        while(1) 
        {
            pause();
            if(received_signal==SIGUSR1)
            {
                received_signal=0;
                FILE *f=fopen("cmd.txt","r");
                if(!f)
                {
                    perror("Nu pot deschide cmd.txt");
                    continue;
                }
                char line[256];
                fgets(line,sizeof(line),f);
                fclose(f);
                char *cmd=strtok(line," \n");
                if(!cmd)
                    continue;
                if(strcmp(cmd,"list_hunts")==0)
                {
                    DIR *d=opendir(".");
                    if(!d)
                    {
                        perror("Nu pot deschide directorul curent");
                        continue;
                    }
                    struct dirent *entry;
                    while((entry=readdir(d))!=NULL) 
                    {
                        if(entry->d_type==DT_DIR && strcmp(entry->d_name, ".")!=0 && strcmp(entry->d_name,"..")!=0) 
                        {
                            char path[256];
                            snprintf(path,sizeof(path),"%s/%s",entry->d_name,TREASURE_FILE);
                            struct stat st;
                            if(stat(path,&st)==0)
                            {
                                printf("%s (%lld bytes)\n",entry->d_name,(long long)st.st_size);
                            }
                        }
                    }
                    closedir(d);
                }
                else if(strcmp(cmd,"list_treasures")==0)
                {
                    char *hunt_id=strtok(NULL,"\n");
                    if(hunt_id)
                        list_treasures(hunt_id);
                    else
                        printf("Missing hunt_id for list_treasures\n");
                }
                else if(strcmp(cmd,"view_treasure")==0)
                {
                    char *hunt_id=strtok(NULL," \n");
                    char *treasure_id=strtok(NULL," \n");
                    if(hunt_id && treasure_id)
                        view_treasure(hunt_id,treasure_id);
                    else
                        printf("Missing arguments for view_treasure\n");
                } 
                else 
                {
                    printf("Comanda necunoscuta: %s\n",cmd);
                }
                fflush(stdout); 
            }
            if(received_signal==SIGUSR2)
            {
                usleep(500000);
                exit(0);
            }
        }
    }
    if(argc<3) 
    {   
        printf("Eroare la introducere numar argumente:\n");
        printf("Folositi una din comenzile:\n");
        printf("%s --add <hunt_id>\n",argv[0]);
        printf("%s --list <hunt_id>\n",argv[0]);
        printf("%s --view <hunt_id> <id>\n",argv[0]);
        printf("%s --remove_treasure <hunt_id> <id>\n",argv[0]);
        printf("%s --remove_hunt <hunt_id>\n",argv[0]);
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