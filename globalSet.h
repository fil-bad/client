//
// Created by alfylinux on 29/07/18.
//
#ifndef CLIENT_GLOBALSETS_H
#define CLIENT_GLOBALSETS_H

#define chatTable "chatTableList.tab"

/** GLOBAL PIPE TO TALK **/
/*
 * LaS libreria ncurse usa lo stdout per printare gli schermi, di conseguenza redirizzando il flusso si perde
 * la possibilità di visualizzare a schermo le finestre.
 * Per lo stdErr non è così, di conseguenza vengono create 2 pipe, in cui quella dello stdErr viene redirezionata
 * a un thread per farla visualizzare quando serve, mentre se si vuole printare a schermo delle informazioni normali
 * si deve usare la pipe Stdout la quale ha dietro un thread che si occupa di visualizzare la cosa
 */

int fdOut;           //il riferimento per scrivere [dprintf(...)]
int fdDebug;

#define lenStr 64

char UserName[lenStr];
char UserID[lenStr];
char convName[lenStr];


#endif //CLIENT_DEFINESETS_H
