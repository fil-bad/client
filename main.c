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
char* UserName; //Username retituito allo stesso punto

int ChatID; // ID della chat nella quale scriveremo quando saremo nella fase di messaggistica

char convName[64];

table *tabChats; //tabella locale delle chat

conversation *conv;

mail packRX;
mail packTX;
pthread_t tidContr, tidRX, tidTX;

sem_t semConv;

avl_pp_S avlACK; // verranno messi i vessaggi in attesa di una risposta;
                 // se l'albero sara' verra' solo segnalato

mex *messageTX;
mex *messageRX;

int pipe_inside[2];

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

    // INIZIALIZZO LE PIPE PER I THREAD
    if(pipe(pipe_inside) == -1){
        perror("pipe_inside broken.\n");
        return -1;
    }

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

    ChatID = chooseAction(buff, con, pack, tabChats);

    if (!(strcmp(buff,"openChat") == 0 || strtol(buff,NULL,10) == 5))
    {
        goto showChat;
    }

    printf("Benvenuto nella chat n.%d.\n", ChatID);

    //* INIZIALIZZO OGNI VOLTA L'AVL SE NON ERA STATO CREATO*//

    avlACK = init_avl_S();

    conv = startConv(pack, conv); //scarichiamo tutta la conversazione in locale
    if (conv == NULL){
        printf("Conv not initialized.\n");
        return -1;
    }

    printf("Entro nella room...\n");

    signal(SIGINT, SIG_DFL); // PER AIUTARE A GESTIRE LATO SERVER LA CHIUSURA INCONTROLLATA

    signal(SIGINT, changerType); //inizio a gestire i l'handler per l'uscita di messaggio

    sem_init(&semConv,0,0); // inizializzamo il semaforo dei thread a 0,
                        // aspetteremo che uno dei due faccia post e poi lo reinizializziamo

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_create(&tidContr, NULL, thUserContr, con);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_create(&tidRX, NULL, thUserRX, pipe_inside);

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    pthread_create(&tidTX, NULL, thUserTX, pipe_inside);

    sem_wait(&semConv); // aspetto che uno dei due finisca la sua esecuzione

    void *resContr, *resRX, *resTX;

    do { //devo ciclare perche' non e' detto che il CANCEL avvenga subito
        pthread_cancel(tidContr);
        pthread_cancel(tidRX);
        pthread_cancel(tidTX);
        pthread_join(tidContr,&resContr);
        pthread_join(tidRX,&resRX);
        pthread_join(tidTX,&resTX);
    } while (resContr == PTHREAD_CANCELED || resRX == PTHREAD_CANCELED || resTX == PTHREAD_CANCELED);

    // Elimino l'avl della conversazione, non piu' necessario

    destroy_avl(avlACK.avlRoot);

    signal(SIGINT, SIG_DFL);

    goto showChat;
}

void *thUserContr(connection *con){
    mail packService;
    do {
        if(readPack(con->ds_sock, &packService) == -1){
            break;
        }
        if(writePack_inside(pipe_inside[1], &packService) == -1){
            break;
        }
    } while (packService.md.type != delRm_p);
    delEntry(tabChats, ChatID);
    sem_post(&semConv);
    pause();
}

void *thUserRX(int pipeInside[2]) {

    char WorW[wowDim];
    sprintf(WorW, "%d",ChatID); //Immettiamo il ChatID per comunicare al server a chi scriviamo

    char userBuff[sendDim];
    sprintf(userBuff,"%s:%s",UserID,UserName); // UserID:UserName


    do {
        if(readPack_inside(pipeInside[0], &packRX) == -1){
            break;
        }
        if(packRX.md.type == delRm_p){
            delEntry(tabChats, ChatID);
            break;
        }
        if(packRX.md.type == success_p){
            // conviene che ogni thread in caso vede se in arrivo e' un ack, e in caso accede all'albero?
            delete_avl_node_S(avlACK, atoi(packRX.md.whoOrWhy));
            continue;
        }
        if(packRX.md.type != mess_p){
            printf("Unexpected pack; going to main menu...\n");
            break;
        }

        printPack(&packRX); //todo: funzione per il print a schermo

        /* PARTE INSERIMENTO IN CONV DEI MESSAGGI*/
        messageRX = makeMex(packRX.mex, (int)strtol(UserID,NULL,10));
        if (addMex(conv, messageRX) == -1){
            printf("Error writing mex on conv in RX.\n");
            break;
        }

        // Conferma che tutto sia andato a buon fine
        fillPack(&packRX, success_p, 0, NULL, userBuff, packRX.md.whoOrWhy);
        writePack(con->ds_sock, &packRX);

    } while (packRX.md.type != delRm_p);

    // pthread_cancel(tidTX);
    if(packRX.md.type == delRm_p) delEntry(tabChats, ChatID);

    free(packRX.mex);
    free(packTX.mex);

    sem_post(&semConv);

    pause();
}


void* thUserTX(connection *con){

    char *buff;
    long int codMex;
    char WorW[wowDim];

    char userBuff[sendDim];
    sprintf(userBuff,"%s:%s",UserID,UserName); // UserID:UserName

    TypeMex = mess_p;

    printf("Inserire un messaggio ('$q' o CTRL+C per terminare):");

    do {
        printf("\n>>> ");
        buff = obtainStr(buff); //messaggio da mandare

        codMex = random();
        sprintf(WorW, "%ld", codMex); //codice "univoco" per il messaggio

        if (strcmp(buff, "$q") == 0) TypeMex = exitRm_p;
        // nel caso volessimo uscire NON mandiamo il messaggio attualmente in scrittura
        if (TypeMex == exitRm_p) {
            fillPack(&packTX, TypeMex, 0, NULL, userBuff, WorW);
            free(buff);
            break;
        }
        // altrimenti mandiamo come tipo mess_p e il messaggio scritto in precedenza
        else {
            fillPack(&packTX, TypeMex, strlen(buff) + 1, buff, userBuff, WorW);
            free(buff);
        }
        if(writePack(con->ds_sock, &packTX) == -1){
            break;
        }

        insert_avl_node_S(avlACK, atoi(packTX.md.whoOrWhy), atoi(packTX.md.whoOrWhy)); // vedere il value da mettere

        if((**(avlACK.avlRoot)).height > 4){ // quindi almeno 2^5 = 32 success pendenti
            printf("Attention, AVL height is %d, there could be some problems on the server.\n",(**(avlACK.avlRoot)).height);
            sleep(5);
        }


        printPack(&packTX); //todo: funzione per il print a schermo

        /* PARTE INSERIMENTO IN CONV DEI MESSAGGI*/
        messageTX = makeMex(packRX.mex, (int)strtol(UserID,NULL,10));
        if (addMex(conv, messageTX) == -1){
            printf("Error writing mex on conv in TX.\n");
            break;
        }


        //readPack_inside(pipeInside[0], &packTX);
        //if(packTX.md.type == delRm_p){
        //    delEntry(tabChats,ChatID);
        //    break;
        //}
        //if(packTX.md.type == mess_p){
        //    writePack_inside(pipeInside[1], &packTX);
        //    //usleep(1000); // non dovrebbe servire per come funziona il thread
        //    continue;
        //}
        //if(packTX.md.type != success_p){
        //    printf("Unexpected pack; going to main menu...\n");
        //    break;
        //}


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