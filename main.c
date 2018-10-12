#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "include/client.h"
#include "include/tableFile.h"
#include "include/fileSystemUtylity.h"
#include "include/mexData.h"
#include "globalSet.h"


int UserID; // ID restituito al termine della creazione utente

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

    if (pack->md.type == dataUs_p){

        //buff = obtainStr(buff);
        if (StartClientStorage("ChatList") == -1){ //poi usare il buff per renderlo adattabile
            return -1; // GESTIONE USCITA
        }

        printf("Welcome, you can talk over following chat; please choose one:\n");

        // salvataggio tabella ricevuta ed apertura
    }

    pthread_t tidRX, tidTX;
    pthread_create(&tidRX, NULL, thUserRX, NULL);
    pthread_create(&tidTX, NULL, thUserTX, NULL);

    //todo: puo' essere utile attivare l'help da dentro la chat con ctrl+C

    pause();
    return 0;
}

void *thUserRX(connection *con) {

    mail *packReceive = malloc(sizeof(mail));
    do {
        if(readPack(con->ds_sock, packReceive) == -1){
            pthread_exit(NULL);
        }
        printPack(packReceive);
    } while (packReceive->md.type != exitRm_p);
}


void* thUserTX(connection *con){
    mail *packSend = malloc(sizeof(mail));
    char *buff;

    do {
        buff = obtainStr(buff);

        fillPack(packSend, mess_p, strlen(buff)+1, buff, "UTENTE", "ID"); //Utente e ID sono valori ottenuti dopo login

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
    pthread_exit(NULL);
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