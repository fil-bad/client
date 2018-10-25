#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "include/client.h"
#include "include/tableFile.h"
#include "include/fileSystemUtylity.h"
#include "include/mexData.h"
#include "globalSet.h"



int PID; // processID per lo stop del main thread

char* UserID; // UserID restituito al termine della creazione utente
char* UserName; //Username retituito allo stesso punto

int ChatID; // ID della chat nella quale scriveremo quando saremo nella fase di messaggistica

mail packReceive;
mail packSend;
pthread_t tidRX, tidTX;




int clientDemo(int argc, char *argv[]) {

    char *buff;

    connection *con = initSocket((u_int16_t) strtol(argv[1], NULL, 10), argv[2]);

    if (initClient(con) == -1) {
        exit(-1);
    }
    printf("Connection with server done. Please choose 'login' or 'register'\n>>> ");
    mail *pack = malloc(sizeof(mail));

    //PARTE LOGIN O CREATE

    retry:

    buff = obtainStr(buff);

    if (strcmp(buff, "login") == 0) {
        if (loginUserSide(con->ds_sock, pack) == -1) {
            perror("Login failed; cause:");
            return -1;
        }
    } else if (strcmp(buff, "register") == 0) {
        int usid = createUser(con->ds_sock, pack);
        if ( usid == -1) {
            return -1;
        }
    } else {
        printf("Caso non supportato; riprovare\n");
        goto retry;
    }
    //printf("<UserID>:<USER> = %s:%s\n", UserID,UserName);

    table *tabChats = initClientTable(tabChats,pack) ;
    if (tabChats == NULL){
        printf("Errore apertura Tabella Chat.\n");
        return -1;
    }

    printf("\nWelcome. ");

    showChat: // label che permette di re-listare tutte le chat

    printf("You can talk over following chat:\n");
    //tabPrint(tabChats);
    //printf("\n\n\n");

    printChats(tabChats);

    printf("\nPlease choose one command or the relative number: (otherwise help() will be shown)\n\n");
    printf("\t'createChat'/'1'\t'deleteChat'/'2'\t'leaveChat'/'3'\n\t'openChat'/'4'\t'joinChat'/'5'\n\n>>> ");

    buff = obtainStr(buff);

    ChatID = chooseAction(buff, con, pack, tabChats);
    if(ChatID == -1){
        goto showChat;
    }

    PID = getpid();

    pthread_create(&tidRX, NULL, thUserRX, con);
    pthread_create(&tidTX, NULL, thUserTX, con);

    //todo: puo' essere utile attivare l'help da dentro la chat con ctrl+C

    //raise(SIGSTOP); //discutere se possa essere una soluzione (SEMBREREBBE NO)

    pause(); // USARE I SEMAFORI, che sono signal free

    goto showChat;

    return 0;
}

void *thUserRX(connection *con) {

    do {
        if(readPack(con->ds_sock, &packReceive) == -1){
            pthread_exit(NULL);
        }
        printPack(&packReceive);
    } while (packReceive.md.type != exitRm_p);

    free(packReceive.mex);
    pthread_exit(NULL);
}


void* thUserTX(connection *con){
    char *buff;

    do {
        printf("Inserire un messaggio:\n>>> ");
        buff = obtainStr(buff);

        fillPack(&packSend, mess_p, strlen(buff)+1, buff, UserName, UserID); //Utente e UserID sono valori ottenuti dopo login

        if(writePack(con->ds_sock, &packSend) == -1){
            pthread_exit(NULL);
        }
        printPack(&packSend);

    } while (packSend.md.type != exitRm_p);
    free(packSend.mex);
    close(con->ds_sock);

    pthread_kill(tidRX,SIGKILL);

    free(packReceive.mex);



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