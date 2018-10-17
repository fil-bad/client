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
char* UserName;

int clientDemo(int argc, char *argv[]) {

    char *buff;

    connection *con = initSocket((u_int16_t) strtol(argv[1], NULL, 10), argv[2]);

    if (initClient(con) == -1) {
        exit(-1);
    }
    printf("Connection with server done. Please choose 'login' or 'register'\n>>> ");
    mail *pack = malloc(sizeof(mail));

    retry:

    buff = obtainStr(buff);

    if (strcmp(buff, "login") == 0) {
        if (loginUserSide(con->ds_sock, pack) == -1) {
            perror("Login failed; cause:");
            return -1;
        }
    } else if (strcmp(buff, "creaUser") == 0) {
        int usid = createUser(con->ds_sock, pack);
        if ( usid == -1) {
            return -1;
        }
    } else {
        printf("Caso non supportato; riprovare\n");
        goto retry;
    }

    printf("<UserID>:<USER> = %s:%s\n", UserID,UserName);




    if (StartClientStorage("ChatList") == -1){
        return -1; // GESTIONE USCITA
    }
    FILE *temp = fopen(chatTable, "w+");
    if(fileWrite(temp,pack->md.dim,1,pack->mex) == -1){
        printf("Error writing file\n");
        return -1;
    }
    fclose(temp);
    //printf("file scritto.\n");
    table *tabChats = open_Tab(chatTable);
    //printf("opentab fatto.\n");
    if (tabChats == NULL){
        printf("Errore apertura Tabella Chat.\n");
        return -1;
    }

    crChat: // label che permette di re-listare tutte le chat

    printf("\nWelcome, you can talk over following chat:\n");
    tabPrint(tabChats);

    printf("\nPlease choose one: (otherwise write 'createChat', 'deleteChat', 'openChat' or 'joinChat')\n>>> ");

    // salvataggio tabella ricevuta ed apertura
    buff = obtainStr(buff);

    if(strcmp(buff,"createChat") == 0){ //todo: andare su terminalshell.c e copiare lo strtok
        if(createChat(con->ds_sock, pack, tabChats) == -1) {
            printf("Creation failed.\n");
            return -1;
            goto crChat; ///UNA VOLTA CREATA ===> jOINIAMO DIRETTAMENTE
        }
        else goto joChat;
    }
    else if(strcmp(buff,"openChat") == 0){
        int numEntry = searchFirstEntry(tabChats,buff);//gestire meglio il cerca per non compiere azioni ridondanti

        if( numEntry == -1){
            printf("Chat not exists, please choose one of the following, or create one.\n");
            goto crChat;
        }       /// UNA VOLTA APERTA ====> JOINIAMO DIRETTAMENTE
        else{
            if(openChat(con->ds_sock, pack, numEntry) == -1){
                printf("Unable to join the chat. Returning to chat selection...\n");
            }
        }
    }
    else if(strcmp(buff,"joinChat") == 0){

    }


    joChat:

    PID = getpid();

    pthread_t tidRX, tidTX;
    pthread_create(&tidRX, NULL, thUserRX, con);
    pthread_create(&tidTX, NULL, thUserTX, con);

    //todo: puo' essere utile attivare l'help da dentro la chat con ctrl+C

    raise(SIGSTOP); //discutere se puo' essere una soluzione
    //pause();

    goto crChat;

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

    free(packReceive->mex);
    free(packReceive);
    pthread_exit(NULL);
}


void* thUserTX(connection *con){
    mail *packSend = malloc(sizeof(mail));
    char *buff;

    do {
        printf("Inserire un messaggio:\n>>> ");
        buff = obtainStr(buff);

        fillPack(packSend, mess_p, strlen(buff)+1, buff, UserName, UserID); //Utente e UserID sono valori ottenuti dopo login

        if(writePack(con->ds_sock, packSend) == -1){
            pthread_exit(NULL);
        }
        printPack(packSend);

    } while (strcmp(packSend->mex, "quit") != 0);
    free(packSend->mex);
    free(packSend);
    close(con->ds_sock);
    kill(PID,SIGCONT);
    ///gestire il ritorno alla scelta della chat
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