//
// Created by filippo on 25/09/18.
//

#include "../include/client.h"
#include "../include/tableFile.h"

char *USERNAME;
char* ID;

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

int keepAlive(int *ds_sock){
    /// KEEPALIVE FUNCTION: vedere header per breve documentazione
    int optval;
    socklen_t optlen;
    /// Impostamo i valori temporali degli ACK

    // Tempo di primo ACK (tcp_keepalive_time)
    optval = 30; //tempo in secondi
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

    // IN CASO DI MANCATA RISPOSTA IN UN MINUTO, L'UTENTE RISULTERA' SCOLLEGATO!

    // Attiviamo il keepalive
    optval = 1;
    optlen = sizeof(optval);
    if(setsockopt(*ds_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        perror("setsockopt()");
        close(*ds_sock);
        return -1;
    }

    printf("SO_KEEPALIVE set on socket\n");
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
            if (iterContr > 10) {
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
            if (iterContr > 10) {
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

int testConnection(int ds_sock) {

    mail packTest;
    fillPack(&packTest,test_p, 0, NULL, "SERVER", "testing_code");

    if (writePack(ds_sock, &packTest) == -1) {
        return -1;
    }
    return 0;
}

void freeConnection(connection* con){
    free(con);
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

    if (sender == NULL)strncpy(pack->md.sender, "", 28);
    else strncpy(pack->md.sender, sender, 28);

    if (whoOrWhy == NULL)strncpy(pack->md.whoOrWhy, "", 24);
    else strncpy(pack->md.whoOrWhy, whoOrWhy, 24);

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
///Server FUNCTION

int initServer(connection *c, int coda)
{
    if(bind(c->ds_sock, (struct sockaddr *) &c->sock, sizeof(c->sock)))
    {
        perror("Bind failed, cause:");
        return -1;
    }
    if(listen(c->ds_sock, coda)){
        perror("Listen failed, cause:");
        return -1;
    }
    return 0;
}



///Client FUNCTION

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

int loginUserSide(int ds_sock, mail *pack){

    printf("Fase di Login; inserire Username ed ID\n");
    printf("USER (max 24 caratteri): ");
    USERNAME = obtainStr(USERNAME);
    printf("\nID univoco: ");
    ID = obtainStr(ID);
    printf("\n");

    if (fillPack(pack,login_p,0,NULL,USERNAME, ID) == -1){ //utente e id saranno passati da scanf
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
            printf("Login effettuato\n");
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

int  createUser(int ds_sock, mail *pack){

    printf("Creazione Utente; inserire nuovo Username\n");
    printf("USER (max 24 caratteri): ");
    USERNAME = obtainStr(USERNAME);
    printf("\n");

    if (fillPack(pack,mkUser_p,0,NULL,USERNAME, NULL) == -1){ //id sara' dato dal server
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
            printf("Creazione effettuata\nID = %s (chiave d'accesso per successivi login)\n",pack->md.whoOrWhy);

            ID = malloc(24);
            strcpy(ID,pack->md.whoOrWhy); //cosi' non dovrei avere problemi con la deallocazione di pack

            printf("### ID = %s ###\n",ID);
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

int createRoom(int ds_sock, mail *pack, table *tabChats){
    printf("Creazione nuova chat; scegliere il nome:\n");

    char *buff;
    buff = obtainStr(buff);
    // va in mex il nome della chat perche' lato server l'id serve a definire l'amministratore della chat
    fillPack(pack,mkRoom_p,strlen(buff)+1,buff,USERNAME,ID);

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
            printf("Creazione effettuata\nResult = %s\n",pack->md.whoOrWhy); //o mex, a seconda della decisione sopra
            addEntry(tabChats,pack->md.whoOrWhy,0); //todo: non so quale sia il valore data (deve dirlo il server)
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

int joinRoom(int ds_sock, mail *pack, int numEntry){
    char *buff[sizeof(pack->md.whoOrWhy)]; //sempre capire se limitiamo il nome chat a 24 caratteri

    sprintf(buff,"%d",numEntry);

    fillPack(pack,joinRm_p,strlen(buff)+1,buff,USERNAME,ID);

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
            printf("Join effettuato\n"); //o mex, a seconda della decisione sopra
            return 0;
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