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

// ---------------- Monitor Integration ----------------

void listeaza_vanatori() 
{
    DIR *dir=opendir(".");
    struct dirent *entry;
    while((entry=readdir(dir))!=NULL)
    {
        if(entry->d_type==DT_DIR && strcmp(entry->d_name,".") && strcmp(entry->d_name,".."))
        {
            char cale[256];
            sprintf(cale,"%s/%s",entry->d_name,TREASURE_FILE);
            FILE *fis=fopen(cale,"rb");
            if(!fis)
                continue;
            int count=0;
            Treasure c;
            while(fread(&c,sizeof(Treasure),1,fis)==1)
                count++;
            fclose(fis);
            printf("Vanatoare: %s | Comori: %d\n",entry->d_name,count);
        }
    }
    closedir(dir);
}
void gestioneaza_comanda(int semnal) 
{
    FILE *fis=fopen("monitor_cmd.txt","r");
    if(!fis) 
    {
        perror("Eroare la deschiderea fisierului de comenzi");
        return;
    }
    char comanda[64],vanatoare[64],id[64];
    fscanf(fis,"%s",comanda);
    if(strcmp(comanda,"list_hunts")==0)
    {
        listeaza_vanatori();
    } 
    else if(strcmp(comanda,"list_treasures")==0)
    {       
        fscanf(fis,"%s",vanatoare);
        vanatoare[strcspn(vanatoare,"\n")]='\0';
        list_treasures(vanatoare);
        strcpy(vanatoare,"");
    } 
    else if(strcmp(comanda,"view_treasure")==0)
    {   
        fscanf(fis,"%s",vanatoare);
        fscanf(fis,"%s",id);
        view_treasure(vanatoare,id);
        strcpy(vanatoare,"");
        strcpy(id,"");
    }
    fclose(fis);
}
void opreste_monitor(int semnal) 
{
    printf("Monitorul se opreste...\n");
    usleep(3000000);
    exit(0);
}

int main(int argc,char *argv[])
{
    if(argc==2 && strcmp(argv[1],"--monitor")==0)
    {
        struct sigaction sa;
        sa.sa_handler = gestioneaza_comanda;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGUSR1, &sa, NULL);

        struct sigaction sa_term;
        sa_term.sa_handler = opreste_monitor;
        sigemptyset(&sa_term.sa_mask);
        sa_term.sa_flags = 0;
        sigaction(SIGTERM, &sa_term, NULL);

        while(1) 
        {
            pause();
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