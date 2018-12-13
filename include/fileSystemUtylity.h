//
// Created by alfylinux on 06/07/18.
//

#ifndef CLIENT_FILESYSTEMUTYLITY_H
#define CLIENT_FILESYSTEMUTYLITY_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <semaphore.h>

#include "../globalSet.h"
#include "tableFile.h"
#include "mexData.h"



/** PROTOTIPI   **/

///CLIENT
// Inizializzazione client
int StartClientStorage (char *storage_name);

#endif //CLIENT_FILESYSTEMUTYLITY_H


/*Funzioni per creare la struttura delle room nel file-sistem  */

/*
 * Dentro il server le cartelle contengono le nameList => una dir è una nameList
 *
 * Dir-Server/
 *  |---Dir-ROOM
 *  |   |---Dir-chat-Name 1
 *  |   |   |--- ...
 *  |   |
 *  |   |---Dir-chat-Name 2
 *  |   |   |--- ...
 *  |   |
 *  |   |---Dir-chat-Name 3
 *  |   |   |--- ...
 *  |
 *  |---Dir-USER
 *  |   |--- Table-User-Login-data 1
 *  |   |--- Table-User-Login-data 2
 *  |   |--- ...
 *
 *
 * "attuale chat" = Name-chat
 * Storico-chat : Name-chat-firstMessageData
 *
 * /"Dir-chat-Name"\
 *  |--- File: attuale chat
 *  |---Dir-history/
 *  |   |--- File-hystory xxxx-mm-gg
 *  |   |--- File-hystory xxxx-mm-gg
 *  |   ...
 *
 *  I file in hystory sono read only,
 *  Il file attuale ha una dimensone limite di 1 MB
 *  superata tale soglia deve essere copiato e spostato nello storico
 *  e iniziato un nuovo file, ciò a impedire una dimensione eccessiva di
 *  dati da caricare in ram
 *
 * /"File-User-Login-data"\
 *  Il nome utente è nel nome del file.
 *  il file inizia conInfo la pw
 *  c'è un elenco delle white list, ovvero tutte le chat a cui sono collegato
 *  es:
 *  chat1:chat2:pippo:baudo:ecc:....
 *  conInfo strtok possiamo trovare le singole chat
 *
 */
