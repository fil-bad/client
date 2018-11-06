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

table *tabChats; //tabella locale delle chat

mail packReceive;
mail packSend;
pthread_t tidRX, tidTX;

sem_t sem;

int TypeMex = mess_p; //e' il tipo del messaggio, che sara' modificato dall'handler con exitRM

void changerType(int sig){
    printf("TypeMex precedente = %d\n",TypeMex);
    TypeMex = exitRm_p;
    printf("TypeMex changed to %d; the next message will be the last.\n", TypeMex);
}


int clientDemo(int argc, char *argv[]) {

    char *buff;

    sem_init(&sem,0,0); // inizializzamo il semaforo dei thread

    connection *con = initSocket((u_int16_t) strtol(argv[1], NULL, 10), argv[2]);

    if (initClient(con) == -1) {
        exit(-1);
    }
    printf("Connection with server done. ");
    mail *pack = malloc(sizeof(mail));

    //PARTE LOGIN O CREATE

    retry:

    printf("Please choose 'login'/'1' or 'register'/'2'\n>>> ");

    buff = obtainStr(buff);

    if (strcmp(buff, "login") == 0 || strtol(buff,NULL,10) == 1) {
        if (loginUser(con->ds_sock, pack) == -1) {
            perror("Login failed; cause:");
            return -1;
        }
    } else if (strcmp(buff, "register") == 0 || strtol(buff,NULL,10) == 2) {
        int usid = registerUser(con->ds_sock, pack);
        if ( usid == -1) {
            return -1;
        }
    } else {
        printf("Caso non supportato; riprovare\n");
        goto retry;
    }
    printf("<UserID>:<USER> = %s:%s\n", UserID,UserName);

    tabChats = initClientTable(tabChats,pack) ;
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
    printf("\t'createChat'/'1'\t'deleteChat'/'2'\t'leaveChat'/'3'\n\t'openChat'/'4'\t'joinChat'/'5'\t'printTab'/'$p'\n\n>>> ");

    buff = obtainStr(buff);

    ChatID = chooseAction(buff, con, pack, tabChats);

    // Modalita' temporanea di test
    if (!(strcmp(buff,"openChat") == 0 || strtol(buff,NULL,10) == 4)){
        goto showChat;
    }

    /* QUESTO LO METTEREMO SE VORREMO FARE IL JOIN/OPEN AUTOMATICO
    if(ChatID == -1){
        goto showChat;
    }
    */

    PID = getpid();

    printf("Benvenuto nella chat n.%d. Entro nella room...\n", ChatID);


    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_create(&tidRX, NULL, thUserRX, con);

    signal(SIGINT, changerType); //inizio a gestire i l'handler per

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_create(&tidTX, NULL, thUserTX, con);

    sem_wait(&sem);

    signal(SIGINT, SIG_DFL);

    goto showChat;

    close(con->ds_sock);
    return 0;
}

void *thUserRX(connection *con) {

    do {
        if(readPack(con->ds_sock, &packReceive) == -1){
            break;
        }
        printPack(&packReceive);
    } while (packReceive.md.type != delRm_p);

    pthread_cancel(tidTX);

    if(packReceive.md.type == delRm_p) delEntry(tabChats, ChatID);

    free(packReceive.mex);
    free(packSend.mex);

    sem_post(&sem);

    pthread_exit(NULL);
}


void* thUserTX(connection *con){
    char *buff;
    char WorW[24];
    sprintf(WorW, "%d",ChatID); //Immettiamo il ChatID per comunicare al server a chi scriviamo

    char userBuff[28];
    sprintf(userBuff,"%s:%s",UserID,UserName); // UserID:UserName

    TypeMex = mess_p;

    printf("Inserire un messaggio ('$q' o CTRL+C per terminare):");

    do {
        printf("\n>>> ");
        buff = obtainStr(buff);

        if (strcmp(buff, "$q") == 0) TypeMex = exitRm_p;

        fillPack(&packSend, TypeMex, strlen(buff)+1, buff, userBuff, WorW); //Utente e UserID sono valori ottenuti dopo login

        if(writePack(con->ds_sock, &packSend) == -1){
            break;
        }
        printPack(&packSend);

    } while (packSend.md.type != exitRm_p);

    pthread_cancel(tidRX);

    free(packSend.mex);
    free(packReceive.mex);

    sem_post(&sem);

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