//
// Created by filippo on 25/09/18.
//

#include "../include/client.h"
#include "../include/tableFile.h"
#include "../include/fileSystemUtylity.h"

extern char *UserName;
extern char* UserID;

/// GLOBAL FUNCTION
connection* initSocket(u_int16_t port, char* IP)
{
    connection *con = malloc(sizeof(connection));

    con->ds_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(keepAlive(&con->ds_sock) == -1){
        return NULL;
    };

    bzero(&con->sock, sizeof(con->sock));
    con->sock.sin_family = AF_INET;
    con->sock.sin_port = htons(port);
    if (strcmp(IP, "INADDR_ANY") == 0) {
        con->sock.sin_addr.s_addr = INADDR_ANY;
    }
    else {
        con->sock.sin_addr.s_addr = inet_addr(IP);
    }
    return con;
}

int keepAlive(int *ds_sock)
{
    /// KEEPALIVE FUNCTION: vedere header per breve documentazione
    int optval;
    socklen_t optlen;
    /// Impostamo i valori temporali degli ACK

    // Tempo di primo ACK (tcp_keepalive_time)
    optval = 90; //tempo in secondi
    optlen = sizeof(optval);
    if(setsockopt(*ds_sock, IPPROTO_TCP , TCP_KEEPIDLE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    // Numero di "sonde" prima dell'abort (tcp_keepalive_probes)
    optval = 5; // n. di tentativi
    optlen = sizeof(optval);
    if(setsockopt(*ds_sock, IPPROTO_TCP , TCP_KEEPCNT, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    //Tempo tra una sonda e l'altra (tcp_keepalive_intvl)
    optval = 6; // tempo in secondi tra l'uno e l'altro
    optlen = sizeof(optval);
    if(setsockopt(*ds_sock, IPPROTO_TCP , TCP_KEEPINTVL, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    // IN CASO DI MANCATA RISPOSTA IN DUE MINUTI, L'UTENTE RISULTERA' SCOLLEGATO!

    // Attiviamo il keepalive
    optval = 1;
    optlen = sizeof(optval);
    if(setsockopt(*ds_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    //printf("SO_KEEPALIVE set on socket\n");
    return 0;
}

void freeConnection(connection* con)
{
    free(con);
}

int readPack(int ds_sock, mail *pack) //todo: implementare controllo sulle read
{
    int iterContr = 0; // vediamo se la read fallisce


    ssize_t bRead = 0;
    ssize_t ret = 0;
    //dprintf(STDERR_FILENO, "readPack\n");
    do {
        ret = read(ds_sock, &pack->md + bRead, sizeof(metadata) - bRead);
        if (ret == -1) {
            perror("Read error; cause:");
            return -1;
        }
        if (ret == 0) {
            iterContr++;
            if (iterContr > 2) {
                dprintf(STDERR_FILENO, "Seems Read can't go further; test connection...\n");
                if (testConnection(ds_sock) == -1) {
                    return -1;
                }
            }
        }
        bRead += ret;
    } while (sizeof(metadata) - bRead != 0);

    size_t dimMex = pack->md.dim;

    if (dimMex == 0) {
        pack->mex = NULL;
        return 0;
    }

    pack->mex = malloc(dimMex);

    bRead = 0; //rimetto a zero per la nuova lettura
    ret = 0;
    iterContr = 0;
    do {
        ret = read(ds_sock, pack->mex + bRead, dimMex - bRead);
        if (ret == -1) {
            perror("Read error; cause:");
            return -1;
        }
        if (ret == 0) {
            iterContr++;
            if (iterContr > 2) {
                dprintf(STDERR_FILENO, "Seems Read can't go further; test connection...\n");
                if (testConnection(ds_sock) == -1) {
                    return -1;
                }
            }
        }
        bRead += ret;
    } while (dimMex - bRead != 0);

    return 0;
}

int writePack(int ds_sock, mail *pack) //dentro il thArg deve essere puntato un mail
{
    /// la funzione si aspetta che il buffer non sia modificato durante l'invio
    ssize_t bWrite = 0;
    ssize_t ret = 0;

    do {
        ret = send(ds_sock, pack + bWrite, sizeof(metadata) - bWrite, MSG_NOSIGNAL);
        if (ret == -1) {
            if (errno == EPIPE) {
                dprintf(STDERR_FILENO, "write pack pipe break 1\n");
                return -1;
                //GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
            }
        }
        bWrite += ret;

    } while (sizeof(metadata) - bWrite != 0);

    bWrite = 0;

    do {
        ret = send(ds_sock, pack->mex + bWrite, pack->md.dim - bWrite, MSG_NOSIGNAL);
        if (ret == -1) {
            if (errno == EPIPE) {
                dprintf(STDERR_FILENO, "write pack pipe break 2\n");
                return -1;
                //GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
            }
        }
        bWrite += ret;

    } while (pack->md.dim - bWrite != 0);

    return 0;
}

int testConnection(int ds_sock)
{

    mail packTest;
    fillPack(&packTest,test_p, 0, NULL, "SERVER", "testing_code");

    if (writePack(ds_sock, &packTest) == -1) {
        return -1;
    }
    printf("testpack Riuscito\n");
    return 0;
}

int fillPack(mail *pack, int type, int dim, void *mex, char *sender, char *whoOrWhy) {
    if (pack == NULL) {
        errno = EFAULT;
        return -1;
    }

    pack->md.type = type;
    pack->md.dim = (size_t) dim;

    if (dim == 0)pack->mex = NULL;
    else pack->mex = mex;

    if (sender == NULL) strncpy(pack->md.sender, "", 28);
    else {
        strncpy(pack->md.sender, sender, 27);
        pack->md.sender[27] = '\0'; // sono sicuro che possa venir letto come una stringa
    }

    if (whoOrWhy == NULL) strncpy(pack->md.whoOrWhy, "", 24);
    else{
        strncpy(pack->md.whoOrWhy, whoOrWhy, 23);
        pack->md.whoOrWhy[23] = '\0'; // sono sicuro che possa venir letto come una stringa
    }

    return 0;
}

void printPack(mail *pack)
{
    printf("######[][]I METADATI SONO[][]######\n");
    printf("Dim pack = %ld\n",pack->md.dim);
    printf("Type = %d\n",pack->md.type);
    printf("Sender = %s\n",pack->md.sender);
    printf("whoOrWhy = %s\n",pack->md.whoOrWhy);
    printf("------[][]IL MESSAGGIO[][]------\n");
    printf("TEXT :\n--> %p\n\n",pack->mex); //non sempre stringa
}

///         ### Client FUNCTION ###

int initClient(connection *c)
{
    if (connect(c->ds_sock,(struct sockaddr *) &c->sock,sizeof(c->sock)))
    {
        perror("Connect error; cause:");
        close(c->ds_sock);
        return -1;
    }
    return 0;
}

char *obtainStr(char *buff){
    antiSegFault:
    scanf("%m[^\n]", &buff);
    fflush(stdin);
    if (buff == NULL) goto antiSegFault;
    return buff;
}

int loginUser(int ds_sock, mail *pack){

    rescue: //risolvere consistenza stringhe

    printf("Fase di Login; inserire Username ed UserID\n");
    printf("USER (max 23 caratteri) >>> ");
    UserName = obtainStr(UserName);

    printf("\nUserID univoco >>> ");
    UserID = obtainStr(UserID);
    printf("\n");

    if (fillPack(pack,login_p,0,NULL,UserName, UserID) == -1){ //utente e id saranno passati da scanf
        return -1;
    }
    if (writePack(ds_sock, pack) == -1){
        return -1;
    }
    //Pacchetto mandato, in attesa di risposta server
    if (readPack(ds_sock,pack) == -1){
        return -1;
    }

    switch (pack->md.type){
        case dataUs_p: // otteniamo sempre una tabella (anche vuota, dove scrivere le chat)
            printf("Login effettuato come %s, %s\n",UserID, UserName);
            return 0;
            break;

        case failed_p:
            printf("Login non effettuato\nServer Report: %s\n", pack->md.whoOrWhy);
            errno = ENOMEM;
            return -1;
            break;
        default:
            printf("Unespected behaviour from server.\n");
            errno = EINVAL;
            return -1;
            break;
    }
    return 0;
}

int registerUser(int ds_sock, mail *pack){

    printf("Creazione Utente; inserire nuovo Username\n");
    printf("USER (max 23 caratteri) >>> ");
    UserName = obtainStr(UserName);
    printf("\n");

    if (fillPack(pack,mkUser_p,0,NULL,UserName, NULL) == -1){ //id sara' dato dal server
        return -1;
    }
    if (writePack(ds_sock, pack) == -1){
        return -1;
    }
    //Pacchetto mandato, in attesa di risposta server
    if (readPack(ds_sock,pack) == -1){
        return -1;
    }

    switch (pack->md.type){
        case dataUs_p: // otteniamo sempre una tabella (anche vuota, dove scrivere le chat)
            UserID = malloc(24);
            strcpy(UserID,pack->md.whoOrWhy); //cosi' non dovrei avere problemi con la deallocazione di pack

            printf("### UserID = %s ### (chiave d'accesso per successivi login)\n",UserID);
            int id = (int)strtol(pack->md.whoOrWhy,NULL,10);
            return id;
            break;

        case failed_p:
            printf("Creazione non effettuata\nServer Report: %s\n", pack->md.whoOrWhy);
            errno = ENOMEM;
            return -1;
            break;
        default:
            printf("Unespected behaviour from server.\n");
            errno = EINVAL;
            return -1;
            break;
    }
    return 0;
}

table *initClientTable(table *tabChats, mail *pack){

    if (StartClientStorage("ChatList") == -1){
        return NULL;
    }
    FILE *temp = fopen(chatTable, "w+");

    if(fileWrite(temp,pack->md.dim,1,pack->mex) == -1){
        printf("Error writing file\n");
        return NULL;
    }
    fclose(temp);

    tabChats = open_Tab(chatTable);
    return tabChats;
}

void printChats(table *tabChats){
    printf("\t**********\n");
    for (int i = 0; i < tabChats->head.len; i++) {
        printf("\t%s\n",tabChats->data[i].name);
    }
    printf("\t**********\n");
}

/* #### SCELTA AZIONI ####*/

int chooseAction(char *command, connection *con, mail *pack, table *tabChats){

    int value = -1;

    if(strcmp(command,"createChat") == 0 || strtol(command,NULL,10) == 1){
        value = createChat(con->ds_sock, pack, tabChats);
        if(value == -1) {
            printf("Creation failed.\n");
            return -1;
        }
    }
    else if(strcmp(command,"deleteChat") == 0 || strtol(command,NULL,10) == 2){
        if (deleteChat(con->ds_sock,pack, tabChats) == -1){
            printf("Unable to delete the chat. Returning to chat selection...\n");
            return -1;
        }
    }
    else if(strcmp(command,"leaveChat") == 0 || strtol(command,NULL,10) == 3){
        if (leaveChat(con->ds_sock,pack, tabChats) == -1){
            printf("Unable to delete the chat. Returning to chat selection...\n");
            return -1;
        }
    }
    else if(strcmp(command,"openChat") == 0 || strtol(command,NULL,10) == 4){
        value = openChat(con->ds_sock, pack, tabChats);
        if (value == -1){
            printf("Unable to open the chat. Returning to chat selection...\n");
            return -1;
        }
    }
    else if(strcmp(command,"joinChat") == 0 || strtol(command,NULL,10) == 5){
        value = joinChat(con->ds_sock,pack, tabChats);
        if (value == -1){
            printf("Unable to join the chat. Returning to chat selection...\n");
            return -1;
        }
    }else if(strcmp(command,"printTab") == 0 || strcmp(command,"$p") == 0){
	    tabPrint(tabChats);
	    printf("\n\n\n");
    }
    else {
        helpChat();
        return -1;
    }
    return value;
}


/* ##### AZIONI DELLE SCELTE SELEZIONATE ##### */

int createChat(int ds_sock, mail *pack, table *tabChats){
    printf("Creazione nuova chat; scegliere il nome (max 23 caratteri):\n");

    char *buff;
    buff = obtainStr(buff);
    // va in mex il nome della chat perche' lato server l'id serve a definire l'amministratore della chat
    fillPack(pack,mkRoom_p,strlen(buff)+1,buff,UserName,UserID); //todo: vedere schema leaveChat

    if (writePack(ds_sock, pack) == -1){
        return -1;
    }
    //Pacchetto mandato, in attesa di risposta server
    if (readPack(ds_sock,pack) == -1){
        return -1;
    }

    switch (pack->md.type){
        case success_p:
            // (anche vuota, dove scrivere le chat)
            printf("Creazione effettuata\n<Id>:<Nome_Room> = %s\n",pack->md.whoOrWhy);
            addEntry(tabChats,pack->md.whoOrWhy,0);
            return searchFirstOccurrence(tabChats, pack->md.whoOrWhy);
            break;

        case failed_p:
            printf("Creazione non effettuata\nServer Report: %s\n", pack->md.whoOrWhy);
            errno = ENOMEM;
            return -1;
            break;
        default:
            printf("Unespected behaviour from server.\n");
            errno = EINVAL;
            return -1;
            break;
    }
    return 0;
}

int deleteChat(int ds_sock, mail *pack, table *tabChats){
    //possibile solo se l'utente e' ADMIN di tale chat

    printf("Eliminazione chat; scegliere l'ID:\n");

    char *buff;
    buff = obtainStr(buff);

    int indexEntry = searchFirstOccurrenceKey(tabChats, (int) strtol(buff, NULL, 10));
    if (indexEntry == -1){
        printf("ID not found.\n");
        return -1;
    }

    char newBuff[24]; //non e' un problema se di dimensione fissa, perche' prima cerchiamo se esista!
    sprintf(newBuff,"%s:%d",buff,indexEntry); // idKey:EntryPosition

    char userBuff[28];
    sprintf(userBuff,"%s:%s",UserID,UserName); // UserID:UserName


    fillPack(pack,delRm_p,0,NULL, userBuff, newBuff);

    if (writePack(ds_sock, pack) == -1){
        return -1;
    }
    if (readPack(ds_sock,pack) == -1){
        return -1;
    }

    switch (pack->md.type){
        case success_p:
            // (anche vuota, dove scrivere le chat)
            printf("Creazione effettuata\n<Id>:<Nome_Room> = %s\n",pack->md.whoOrWhy);
            delEntry(tabChats, indexEntry);
            return 0;
            break;

        case failed_p:
            printf("Creazione non effettuata\nServer Report: %s\n", pack->md.whoOrWhy);
            errno = ENOMEM;
            return -1;
            break;
        default:
            printf("Unespected behaviour from server.\n");
            errno = EINVAL;
            return -1;
            break;
    }
    return 0;
}

int leaveChat(int ds_sock, mail *pack, table *tabChats){
    //possibile per qualunque utente; se era l'ultimo della chat,

    printf("Leaving chat; scegliere l'ID:\n");

    char *buff;
    buff = obtainStr(buff);

    int indexEntry = searchFirstOccurrenceKey(tabChats, (int) strtol(buff, NULL, 10));
    if (indexEntry == -1){
        printf("ID not found.\n");
        return -1;
    }

    char newBuff[24];
    sprintf(newBuff,"%s:%d",buff,indexEntry); // idKey:EntryPosition

    char userBuff[28];
    sprintf(userBuff,"%s:%s",UserID,UserName); // UserID:UserName


    fillPack(pack,leaveRm_p,0,NULL, userBuff, newBuff);

    if (writePack(ds_sock, pack) == -1){
        return -1;
    }
    if (readPack(ds_sock,pack) == -1){
        return -1;
    }

    switch (pack->md.type){
        case success_p:
            // (anche vuota, dove scrivere le chat)
            printf("Leave riuscito\n");
            delEntry(tabChats,indexEntry);
            return 0;
            break;

        case failed_p:
            printf("Leave non effettuato\nServer Report: %s\n", pack->md.whoOrWhy);
            errno = ENOMEM;
            return -1;
            break;
        default:
            printf("Unespected behaviour from server.\n");
            errno = EINVAL;
            return -1;
            break;
    }
    return 0;
}

int openChat(int ds_sock, mail *pack, table *tabChats){
    printf("Selezione chat esistente; scegliere il nome:\n");

    char *buff;
    buff = obtainStr(buff);

    int numEntry = searchFirstOccurrence(tabChats, buff);
    if( numEntry == -1){
        printf("Chat not exists, please choose one of the following, or create one.\n");
        return -1;
    }
    sprintf(buff,"%d",numEntry); // GESTIRE COSA MANDARE AL SERVER!!!

    fillPack(pack,openRm_p,strlen(buff)+1,buff,UserName,UserID);

    if (writePack(ds_sock, pack) == -1){
        return -1;
    }
    //Pacchetto mandato, in attesa di risposta server
    if (readPack(ds_sock,pack) == -1){
        return -1;
    }

    switch (pack->md.type){
        case success_p:
            // (anche vuota, dove scrivere le chat)
            printf("Open successful\n"); //o mex, a seconda della decisione sopra
            return numEntry;
            break;

        case failed_p:
            printf("Open error.\nServer Report: %s\n", pack->md.whoOrWhy);
            errno = ENOMEM;
            return -1;
            break;

        case delRm_p: //La chat e' stata cancellata e non e' piu' esistente
            printf("Open error. %s is a no-existing more chat. Deleting...\n", buff);
            delEntry(tabChats,numEntry);
            return -1;
            break;
        default:
            printf("Unespected behaviour from server.\n");
            errno = EINVAL;
            return -1;
            break;
    }
    return 0;
}

int joinChat(int ds_sock, mail *pack, table *tabChats){
    printf("Join nuova chat; scrivere ID.\n<ID>: ");

    char *buff;
    buff = obtainStr(buff);

    int numEntry = searchFirstOccurrenceKey(tabChats, (int)strtol(buff, NULL, 10));
    if( numEntry != -1){
        printf("Chat < %s > already exists, please use 'openChat'/'4' + 'chatName'.\n", buff);
        return -1;
    }

    fillPack(pack, joinRm_p, 0, NULL, UserName, buff);

    if (writePack(ds_sock, pack) == -1){
        return -1;
    }
    //Pacchetto mandato, in attesa di risposta server
    if (readPack(ds_sock,pack) == -1){
        return -1;
    }

    switch (pack->md.type){
        case dataRm_p:
            printf("Join effettuato\n");
            entry *newChat = (entry *)pack->mex;
            addEntry(tabChats,newChat->name,newChat->point);
            int search= searchFirstOccurrence(tabChats, newChat->name);
            return (int) strtol(tabChats->data[search].name, NULL, 10);
            break;

        case failed_p:
            printf("Join non effettuato\nServer Report: %s\n", pack->md.whoOrWhy);
            errno = ENOMEM;
            return -1;
            break;
        default:
            printf("Unespected behaviour from server.\n");
            errno = EINVAL;
            return -1;
            break;
    }
    return 0;
}

void helpChat(void){
    printf("\n\nCreateChat: crea un nuovo canale di conversazione.\n");
    printf("DeleteChat: elimina un canale di conversazione (possibile solo se si e' admin dello stesso).\n");
    printf("LeaveChat: lascia un canale di conversazione.\n");
    printf("OpenChat: apre un canale esistente di conversazione.\n");
    printf("JoinChat: partecipa a un nuovo canale di conversazione.\n");
}