//
// Created by alfylinux on 06/07/18.
//

#include "../include/fileSystemUtylity.h"


int StartClientStorage (char *storage)  //apre o crea un nuovo storage per il database
{
	/* modifica il path reference dell'env per "spostare" il programma nella nuova locazione
	 * la variabile PWD contiene il path assoluto, della working directory, ed è aggiornata da una sheel
	 * MA I PROGRAMMI SCRITTI IN C usano un altra variabile per dire il proprio percorso di esecuzione.
	 * Di conseguenza bisogna prima modificare il path del processo e sucessivamente aggiornare l'env     *
	 */

	//printf("[1]---> Fase 1, aprire lo storage\n");

	int errorRet;
	errorRet = chdir (storage);                        //modifico l'attuale directory di lavoro del processo
	if (errorRet != 0)    //un qualche errore nel raggiungimento della cartella
	{
		switch (errno){
			case 2: //No such file or directory
				printf ("directory non esistente, procedo alla creazione\n");
				errorRet = mkdir (storage, 0777);
				if (errorRet == -1){
					perror ("mkdir fails:");
					return -1;
				}
				else{
					printf ("New directory create\n");
					errorRet = chdir (storage);
					if (errorRet == -1){
						perror ("nonostante la creazione chdir():");
						return -1;
					}
				}
				break;
			default:
				perror ("chdir:");
				return -1;
		}
	}
	char curDirPath[100];
	errorRet = setenv ("PWD", getcwd (curDirPath, 100), true);    //aggiorno l'env per il nuovo pwd
	if (errorRet != 0) perror ("setEnv('PWD')");
	//printf("Current Directory set:\n-->\tgetcwd()=%s\n-->\tPWD=%s\n\n", curDirPath, getenv("PWD"));
	//printf("[1]---> success\n\n");

	return 0;   //avvio conInfo successo
}
