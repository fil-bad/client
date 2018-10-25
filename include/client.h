//
// Created by filippo on 25/09/18.
//

#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include "tableFile.h"

#define fflush(stdin) while(getchar() != '\n')


/* TIPI DI PACCHETTO*/

enum typePack {
    success_p = 0, failed_p, mess_p, test_p,               /** SYSTEM **/
    login_p, logout_p, delUs_p, mkUser_p, dataUs_p,    /** USER**/
    mkRoom_p, joinRm_p, openRm_p, dataRm_p, leaveRm_p, delRm_p, exitRm_p   /** ROOM**/
};




typedef struct metadata_{
    size_t dim;
    int type; // dobbiamo definire dei tipi di comandi: es. 0 per il login => password in campo mex, ...
    char sender[28];
    char whoOrWhy[24];
}metadata;

typedef struct mail_{
    metadata md;
    void *mex;
} mail;

typedef struct connection_{
    int ds_sock;
    struct sockaddr_in sock;
} connection;

typedef struct thConnArg_{
    connection con;
    void *arg;
}thConnArg;

// PROTOTIPI DI FUNZIONE:

// GLOBALI

connection* initSocket(u_int16_t port, char* IP);
int keepAlive(int *ds_sock);
void freeConnection(connection* con);

int readPack(int ds_sock, mail *pack);  // queste due funzioni prendono il pacchetto thread-specifico
int writePack(int ds_sock, mail *pack); // ma all'interno contengono la struct mail con i dati
int testConnection(int ds_sock);

int fillPack(mail *pack, int type, int dim, void *mex, char *sender, char *whoOrWhy);
void printPack(mail *pack);

///Client FUNCTION



int initClient(connection *c);

char *obtainStr(char *);
char *fitInWoW(char *);

int loginUserSide(int ds_sock, mail *pack);
int createUser(int ds_sock,mail *pack);

table* initClientTable(table *tabChats, mail *pack);
void printChats(table* tabChats);

int createChat(int ds_sock, mail *pack, table *tabChats);
int deleteChat(int ds_sock, mail *pack, table *tabChats);
int leaveChat(int ds_sock, mail *pack, table *tabChats);
int openChat(int ds_sock, mail *pack, table *tabChats);
int joinChat(int ds_sock, mail *pack, table *tabChats);

void helpChat(void);

void* thUserRX(connection *con);
void* thUserTX(connection *con);

#endif //CLIENT_CLIENT_H

/// ### DOCUMENTAZIONE ### ///

/* SO_KEEPALIVE
 *
 * http://tldp.org/HOWTO/TCP-Keepalive-HOWTO/usingkeepalive.html
 * Al paragrafo 3.1.1, e' mostrato come modificare il parametro temporale di sistema tramite CAT.
 * Sara' necessario ridurre i tempi per rendersene conto;
 * I cambiamenti dovranno essere impostati ad ogni avvio;
 *
 * E' POSSIBILE FARLO CON DEI VALORI DA MODIFICARE!!! VEDERE PARAGRAFO 4.2
 *
 */




