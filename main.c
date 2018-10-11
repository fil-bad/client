#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "include/client.h"

int UserID;
/*

int serverDemo(int argc, char *argv[])
{
    printf("Server in avvio\n");

    con = initSocket(atoi(argv[2]),"INADDR_ANY");
    if (con == NULL){
        exit(-1);
    }

    printf("Socket inizializzato\n");
    if(initServer(con,atoi(argv[3])) == -1){
        exit(-1);
    }

    printf("Server avviato\n");
    pthread_t tid;
    thUserServ *arg;
    int i = 0;


    /// parte con semafori di test
    sem_init(&sem[0],0,1);
    sem_init(&sem[1],0,0);



    while(1){
        arg = malloc(sizeof(thUserServ));
        printf("arg = %p creato.\n", arg);
        arg->id = i;
        if(acceptCreate(con, thUserServer, arg) == -1)
        {
            exit(-1);
        }
        i++;
    }

    return 0;
}



void *thUserServer(thConnArg *argTh){
    thUserServ *info=argTh->arg;
    printf("TH creato\nId = %d\n", info->id);

    mail *packRecive= malloc(sizeof(mail));

    if(loginServerSide(argTh->con.ds_sock, packRecive) == -1){
        printf("Login fallito; disconnessione...\n");
        pthread_exit(NULL);
    }  /// RIVEDERE LA PARTE DELL'IF, RIMANGONO TROPPI ACCEPT NON LIBERATI COMPLETAMENTE IN QUESTO MODO

    argTh->arg = packRecive;

    printf("mi metto in ascolto\n");
    pthread_t tidRX, tidTX;

    pthread_create(&tidRX,NULL, thrServRX,argTh);
    pthread_create(&tidTX,NULL, thrServTX,argTh);

    pause();
    pthread_exit(NULL);
}

void *thrServRX(thConnArg *argTh){

    mail *packTest = argTh->arg;

    while(1) {
        sem_wait(&sem[0]);

        printf("Entro nel semaforo di andata\n");

        readPack(argTh->con.ds_sock, packTest);
        printf("Numero byte pacchetto: %d\n", packTest->md.dim);
        printf("Stringa da client: %s\n\n", packTest->mex);
        if (strcmp(packTest->mex, "quit") == 0) {
            break;
        }

        sem_post(&sem[1]);
        printf("Esco dal semaforo di ritorno\n");

        //writePack(); da aggiungere il selettore chat
    }
    close(argTh->con.ds_sock);
    free(argTh);
    pthread_exit(0);
}

void *thrServTX(thConnArg * argTh){

    mail *packTest = argTh->arg;

    while(1){
        //readPack() da aggiungere il selettore in ingresso di chat

        sem_wait(&sem[1]);
        printf("Entro nel semaforo di ritorno\n");

        writePack(argTh->con.ds_sock,packTest);
        if(errno){
            exit(-1);
        }


        sem_post(&sem[0]);
        printf("Esco dal semaforo di ritorno\n");
    }
}

 */

int clientDemo(int argc, char *argv[]) {

    char *buff;

    connection *con = initSocket((u_int16_t) strtol(argv[1], NULL, 10), argv[2]);

    if (initClient(con) == -1) {
        exit(-1);
    }
    printf("Collegamento al server effettuato. Scegliere 'login' o 'creaUser'\n");
    mail *pack = malloc(sizeof(mail));

    retry:

    buff = obtainStr(buff);

    if (strcmp(buff, "login") == 0) {
        if (loginUserSide(con->ds_sock, pack) == -1) {
            perror("Login failed; cause:");
            return -1;
        }
    } else if (strcmp(buff, "creaUser") == 0) {
        UserID = createUser(con->ds_sock, pack);
        if ( UserID == -1) {
            return -1;
        }
    } else {
        printf("Caso non supportato; riprovare\n");
        goto retry;
    }
    mail *packSend = malloc(sizeof(mail));

    printf("Benvenuto. Le chat disponibili sono:\n");

    pthread_t tidRX, tidTX;

    pthread_create(&tidRX, NULL, thUserRX, NULL);
    //pthread_create(&tidTX, NULL, thUserTX, NULL);

    //todo: puo' essere utile attivare l'help da dentro la chat con ctrl+C

    pause();
    return 0;
}

void *thUserRX(connection *con) {

    mail *packSend = malloc(sizeof(mail));
    char *buff;

    do {
        buff = obtainStr(buff);

        fillPack(packSend, dataUs_p, strlen(buff)+1, buff, "UTENTE", "ID"); //Utente e ID sono valori ottenuti dopo login

        if(writePack(con->ds_sock, packSend) == -1){
            pthread_exit(NULL);
        }
        printPack(packSend);

    // inizio parte per il testing mono-thread

        printf("\n\nRitorno\n");

        if(readPack(con->ds_sock, packSend) ==-1){
            pthread_exit(NULL);
        }
        printPack(packSend);

    } while (strcmp(packSend->mex, "quit") != 0);
    free(packSend->mex);
    free(packSend);
    close(con->ds_sock);
    return 0;
}


void* thUserTX(mail *pack){

}

void helpProject()
{
    printf("I parametri Client sono:\n");
    printf("[port] [IP]\tMi collego al server a porta e IP specificati (127.0.0.1 per test locale)\n");
}

int main(int argc, char *argv[])
{

    if (argc == 3)
    {
        if (clientDemo(argc, argv) == -1){
            return -1;
        }
    }
    helpProject();

    return 0;
}