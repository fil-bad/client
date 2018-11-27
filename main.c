#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "include/client.h"
#include "include/tableFile.h"
#include "include/fileSystemUtylity.h"
#include "include/mexData.h"
#include "globalSet.h"
#include "treeFunx/include/avl.h"

connection *con;

char* UserID; // UserID restituito al termine della creazione utente
char* UserName; //Username restituito allo stesso punto

int ChatEntry; // ID della chat nella quale scriveremo quando saremo nella fase di messaggistica

char convName[64];

table *tabChats; //tabella locale delle chat

conversation *conv;

mail packRX;
mail packTX;
pthread_t tidRX, tidTX;

sem_t semConv;

avl_pp_S avlACK; // verranno messi i vessaggi in attesa di una risposta;
// se l'albero sara' verra' solo segnalato

mex *messageTX;
mex *messageRX;

int TypeMex = mess_p; //e' il tipo del messaggio, che sara' modificato dall'handler con exitRM


void closeHandler(int sig){
    close(con->ds_sock);
    exit(-1);
}

void changerType(int sig) {
    TypeMex = exitRm_p;
    printf("TypeMex changed; press ENTER to quit from room\n");
}

int clientDemo(int argc, char *argv[]) {

    char *buff;

    con = initSocket((u_int16_t) strtol(argv[2], NULL, 10), argv[1]);

    if (initClient(con) == -1) {
        exit(-1);
    }
    signal(SIGINT, closeHandler); // PER AIUTARE A GESTIRE LATO SERVER LA CHIUSURA INCONTROLLATA


    printf("Connection with server done. ");
    mail *pack = malloc(sizeof(mail));

    /** PARTE LOGIN O CREATE **/

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

    signal(SIGINT, closeHandler); // PER AIUTARE A GESTIRE LATO SERVER LA CHIUSURA INCONTROLLATA

    printf("You can talk over following chat:\n");

    printChats(tabChats);

    printf("\nPlease choose one command or the relative number: (otherwise help() will be shown)\n\n");
    printf("\t'1'/'createChat'\t'4'/'joinChat'\t\t'$p'/'printTab'\n"
           "\t'2'/'deleteChat'\t'5'/'openChat'\t\t'$e'/'exitProgram'\n"
           "\t'3'/'leaveChat'\t\t\n\n>>> ");

    buff = obtainStr(buff);

    if ((strcmp(buff,"exitProgram") == 0 || (strcmp(buff,"$e") == 0)))
    {
        close(con->ds_sock);
        return 0;
    }

    ChatEntry = chooseAction(buff, con, pack, tabChats);
    if (ChatEntry == -1) goto showChat;

    if (!(strcmp(buff,"openChat") == 0 || strtol(buff,NULL,10) == 5))
    {
        goto showChat;
    }

    printf("Benvenuto nella chat %s.\n", tabChats->data[ChatEntry].name);

    conv = startConv(pack, conv); //scarichiamo tutta la conversazione in locale
    if (conv == NULL) {
        printf("Conv not initialized.\n");
        return -1;
    }
    //* INIZIALIZZO OGNI VOLTA L'AVL SE NON ERA STATO CREATO*//

    avlACK = init_avl_S();
    printf("Avl initialized.\n");


    printf("Entro nella room...\n");

    signal(SIGINT, SIG_DFL); // PER AIUTARE A GESTIRE LATO SERVER LA CHIUSURA INCONTROLLATA

    signal(SIGINT, changerType); //inizio a gestire i l'handler per l'uscita di messaggio

    sem_init(&semConv,0,0); // inizializzamo il semaforo dei thread a 0,
    // aspetteremo che uno dei due faccia post e poi lo reinizializziamo

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_create(&tidTX, NULL, thUserTX, con);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_create(&tidRX, NULL, thUserRX, con);

    sem_wait(&semConv); // aspetto che uno dei due finisca la sua esecuzione

    void *resRX, *resTX;

    do { //devo ciclare perche' non e' detto che il CANCEL avvenga subito

        pthread_cancel(tidRX);
        pthread_cancel(tidTX);
        pthread_join(tidRX,&resRX);
        pthread_join(tidTX,&resTX);
    } while (resRX == PTHREAD_CANCELED || resTX == PTHREAD_CANCELED);

    // Elimino l'avl della conversazione, non piu' necessario

    destroy_avl(avlACK.avlRoot);

    signal(SIGINT, SIG_DFL);

    goto showChat;
}


void *thUserRX(connection *con) {

    do {
        if(readPack(con->ds_sock, &packRX) == -1){
            break;
        }
        if(packRX.md.type == delRm_p){ //potrei riceverlo di un'altra chat

            int entCH = atoi(packRX.md.whoOrWhy);
            delEntry(tabChats, entCH);
            if(entCH == ChatEntry) break; // se e' della chat esco dalla RX
            else continue;             // se e' di un'altra, continuo
        }
        if(packRX.md.type == messSuccess_p){
            delete_avl_node_S(avlACK, atoi(packRX.md.whoOrWhy));
            continue;
        }
        if(packRX.md.type == failed_p){
            printf("Failed received, cause: %s\n", packRX.md.whoOrWhy);
            continue;
        } //ignoro questo stato
        if(packRX.md.type != mess_p){
            printf("Unexpected pack; going to main menu...\n");
            break;
        }
        printTextPack(&packRX);
        //printPack(&packRX);

        /* PARTE INSERIMENTO IN CONV DEI MESSAGGI*/
        messageRX = makeMex(packRX.mex, (int)strtol(UserID,NULL,10));
        if (addMex(conv, messageRX) == -1){
            printf("Error writing mex on conv in RX.\n");
            break;
        }

    } while (packRX.md.type != delRm_p);

    // pthread_cancel(tidTX);
    if(packRX.md.type == delRm_p) delEntry(tabChats, ChatEntry);

    free(packRX.mex);
    free(packTX.mex);

    sem_post(&semConv);

    pause();
}


void* thUserTX(connection *con){

    char *buff;
    int codMex = 0;
    char WorW[wowDim];

    char userBuff[sendDim];
    sprintf(userBuff,"%s:%s",UserID,UserName); // UserID:UserName

    TypeMex = mess_p;

    printf("Inserire un messaggio ('$q' o CTRL+C per terminare):");

    do {
        printf("\n>>> ");
        buff = obtainStr(buff); //messaggio da mandare

        sprintf(WorW, "%d", codMex); //codice "univoco" per il messaggio
        codMex++;

        if (strcmp(buff, "$q") == 0) TypeMex = exitRm_p;

        if (TypeMex == exitRm_p) { // nel caso volessimo uscire NON mandiamo il messaggio attualmente in scrittura
            fillPack(&packTX, TypeMex, 0, NULL, userBuff, WorW);
            free(buff);
            break;
        }
        else { // altrimenti mandiamo come tipo mess_p e il messaggio scritto in precedenza
            fillPack(&packTX, TypeMex, strlen(buff) + 1, buff, userBuff, WorW);
        }
        insert_avl_node_S(avlACK, atoi(packTX.md.whoOrWhy), atoi(packTX.md.whoOrWhy)); // vedere il value da mettere

        if(writePack(con->ds_sock, &packTX) == -1){
            delete_avl_node_S(avlACK, atoi(packTX.md.whoOrWhy));
            if (errno == EPIPE) exit(-1);
            break;
        }

        lockReadSem(avlACK.semId);
        if((**(avlACK.avlRoot)).height > 4){ // quindi almeno 2^5 = 32 success pendenti
            printf("Attention, AVL height is %d, there could be some problems on the server.\n",(**(avlACK.avlRoot)).height);
            sleep(5);
        }
        unlockReadSem(avlACK.semId);

        printTextPack(&packTX);
        //printPack(&packTX);

        /* PARTE INSERIMENTO IN CONV DEI MESSAGGI*/
        messageTX = makeMex(packTX.mex, (int)strtol(UserID,NULL,10));
        if (addMex(conv, messageTX) == -1){
            printf("Error writing mex on conv in TX.\n");
            break;
        }
    } while (packTX.md.type != exitRm_p);

    // pthread_cancel(tidRX);

    free(packTX.mex);
    free(packRX.mex);

    sem_post(&semConv);

    pause();
}

void helpProject()
{
    printf("I parametri Client sono:\n");
    printf("[IP] [port]\tMi collego al server a IP e porta specificati (IP = 127.0.0.1 in locale)\n");
}

int main(int argc, char *argv[])
{

    if (argc == 3)
    {
        if (clientDemo(argc, argv) == -1){
            return -1;
        }
        close(con->ds_sock); //chiusura finale di sicurezza
    }
    else helpProject();
    return 0;
}