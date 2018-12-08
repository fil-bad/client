//
// Created by filippo on 25/09/18.
//

#include <unistd.h>
#include "../include/client.h"
#include "../include/fileSystemUtylity.h"


char *typeToText (int type){
	switch (type){
		case success_p:
			return "success_p";
		case failed_p:
			return "failed_p";
		case mess_p:
			return "mess_p";
		case messSuccess_p:
			return "messSuccess_p";
		case test_p:
			return "test_p";
		case login_p:
			return "login_p";
		case logout_p:
			return "logout_p";
		case delUs_p:
			return "delUs_p";
		case mkUser_p:
			return "mkUser_p";
		case dataUs_p:
			return "dataUs_p";
		case kConv_p:
			return "kConv_p";
		case mkRoom_p:
			return "mkRoom_p";
		case joinRm_p:
			return "joinRm_p";
		case openRm_p:
			return "openRm_p";
		case dataRm_p:
			return "dataRm_p";
		case leaveRm_p:
			return "leaveRm_p";
		case delRm_p:
			return "delRm_p";
		case exitRm_p:
			return "exitRm_p";
		default:
			return "Not expected type";
	}
}

/// GLOBAL FUNCTION
connection *initSocket (u_int16_t port, char *IP){
	connection *con = malloc (sizeof (connection));

	con->ds_sock = socket (AF_INET, SOCK_STREAM, 0);

	if (keepAlive (&con->ds_sock) == -1){
		return NULL;
	};

	bzero (&con->sock, sizeof (con->sock));
	con->sock.sin_family = AF_INET;
	con->sock.sin_port = htons (port);
	if (strcmp (IP, "INADDR_ANY") == 0){
		con->sock.sin_addr.s_addr = INADDR_ANY;
	}
	else{
		con->sock.sin_addr.s_addr = inet_addr (IP);
	}
	return con;
}

int keepAlive (int *ds_sock){
	/// KEEPALIVE FUNCTION: vedere header per breve documentazione
	int optval;
	socklen_t optlen;
	/// Impostamo i valori temporali degli ACK

	// Tempo di primo ACK (tcp_keepalive_time)
	optval = 90; //tempo in secondi
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	// Numero di "sonde" prima dell'abort (tcp_keepalive_probes)
	optval = 5; // n. di tentativi
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	//Tempo tra una sonda e l'altra (tcp_keepalive_intvl)
	optval = 6; // tempo in secondi tra l'uno e l'altro
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	// IN CASO DI MANCATA RISPOSTA IN DUE MINUTI, L'UTENTE RISULTERA' SCOLLEGATO!

	// Attiviamo il keepalive
	optval = 1;
	optlen = sizeof (optval);
	if (setsockopt (*ds_sock, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0){
		perror ("setsockopt()");
		close (*ds_sock);
		return -1;
	}

	//printf("SO_KEEPALIVE set on socket\n");
	return 0;
}

void freeConnection (connection *con){
	free (con);
}

int readPack (int ds_sock, mail *pack){

	int iterContr = 0; // vediamo se la read fallisce
	ssize_t bRead = 0;
	ssize_t ret = 0;

    sigset_t newSet, oldSet;
    sigfillset (&newSet);
    pthread_sigmask (SIG_SETMASK, &newSet, &oldSet);

    mailChar mailCRead;

	do{
		ret = read (ds_sock, &mailCRead.md + bRead, sizeof (metadataChar) - bRead);
		if (ret == -1){
			perror ("Read error; cause:");
			return -1;
		}
		if (ret == 0){
			iterContr++;
			if (iterContr > 2){
				dprintf (STDERR_FILENO, "Seems Read can't go further; test connection...\n");
				if (testConnection (ds_sock) == -1){
					return -1;
				}
			}
		}
		bRead += ret;
	}
	while (sizeof (metadataChar) - bRead != 0);

	//** CONVERSIONE MAIL IN INTERI **//

	sscanf(mailCRead.md.dim, "%ld", &pack->md.dim);
	sscanf(mailCRead.md.type, "%d", &pack->md.type);
	memcpy(pack->md.sender, mailCRead.md.sender,sendDim);
	memcpy(pack->md.whoOrWhy, mailCRead.md.whoOrWhy,wowDim);

	//****//

	//** Modifica per il network order**//

	pack->md.type = ntohl(pack->md.type);
	pack->md.dim = ntohl(pack->md.dim);

	size_t dimMex = pack->md.dim; // manteniamo in ordine della macchina il valore del messaggio

	//****//

	if (dimMex == 0){
		pack->mex = NULL;
		return 0;
	}

	pack->mex = malloc (dimMex);

	bRead = 0; //rimetto a zero per la nuova lettura
	ret = 0;
	iterContr = 0;
	do{
		ret = read (ds_sock, pack->mex + bRead, dimMex - bRead);
		if (ret == -1){
			perror ("Read error; cause:");
			return -1;
		}
		if (ret == 0){
			iterContr++;
			if (iterContr > 2){
				dprintf (STDERR_FILENO, "Seems Read can't go further; test connection...\n");
				if (testConnection (ds_sock) == -1){
					return -1;
				}
			}
		}
		bRead += ret;
	}
	while (dimMex - bRead != 0);
    pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
    return 0;
}

int writePack (int ds_sock, mail pack) //dentro il thArg deve essere puntato un mail
{
    //** Modifica per il network order**//

    size_t dimMex = pack.md.dim; // manteniamo in ordine della macchina il valore del messaggio

    pack.md.type = htonl(pack.md.type);
    pack.md.dim = htonl(pack.md.dim);

    //****//

    //** CONVERSIONE IN MAIL A CARATTERI **//

    mailChar mailCWrite;
    sprintf(mailCWrite.md.dim, "%ld", pack.md.dim);
	sprintf(mailCWrite.md.type, "%d", pack.md.type);
	memcpy(mailCWrite.md.sender,pack.md.sender,sendDim);
	memcpy(mailCWrite.md.whoOrWhy,pack.md.whoOrWhy,wowDim);
	mailCWrite.mex = pack.mex;

	//****//

	/// la funzione si aspetta che il buffer non sia modificato durante l'invio
	ssize_t bWrite = 0;
	ssize_t ret = 0;

    sigset_t newSet, oldSet;
    sigfillset (&newSet);
    pthread_sigmask (SIG_SETMASK, &newSet, &oldSet);

	do{
		ret = send (ds_sock, &mailCWrite + bWrite, sizeof (metadataChar) - bWrite, MSG_NOSIGNAL);
		if (ret == -1){
			if (errno == EPIPE){
				dprintf (STDERR_FILENO, "write pack pipe break 1\n");
				return -1;
				//GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
			}
		}
		bWrite += ret;

	}
	while (sizeof (metadataChar) - bWrite != 0);

	if (dimMex == 0) return 0; //cosi' evitiamo un periodo di scrittura

	bWrite = 0;

	do{
		ret = send (ds_sock, mailCWrite.mex + bWrite, dimMex - bWrite, MSG_NOSIGNAL);
		if (ret == -1){
			if (errno == EPIPE){
				dprintf (STDERR_FILENO, "write pack pipe break 2\n");
				return -1;
				//GESTIRE LA CHIUSURA DEL SOCKET (LA CONNESSIONE E' STATA INTERROTTA IMPROVVISAMENTE)
			}
		}
		bWrite += ret;

	}
	while (dimMex - bWrite != 0);
    pthread_sigmask (SIG_SETMASK, &oldSet, &newSet);   //restora tutto
    return 0;
}


int testConnection (int ds_sock){

	mail packTest;
	fillPack (&packTest, test_p, 0, NULL, "SERVER", "testing_code");

	if (writePack (ds_sock, packTest) == -1){
		return -1;
	}
	printf ("testpack Riuscito\n");
	return 0;
}

int fillPack (mail *pack, int type, int dim, void *mex, char *sender, char *whoOrWhy){
	if (pack == NULL){
		errno = EFAULT;
		return -1;
	}

	pack->md.type = type;
	pack->md.dim = (size_t)dim;

	if (dim == 0) pack->mex = NULL;
	else pack->mex = mex;

	if (sender == NULL) strncpy (pack->md.sender, "", sendDim);
	else{
		strncpy (pack->md.sender, sender, sendDim);
		pack->md.sender[sendDim - 1] = '\0'; // sono sicuro che possa venir letto come una stringa
	}

	if (whoOrWhy == NULL) strncpy (pack->md.whoOrWhy, "", wowDim);
	else{
		strncpy (pack->md.whoOrWhy, whoOrWhy, wowDim);
		pack->md.whoOrWhy[wowDim - 1] = '\0'; // sono sicuro che possa venir letto come una stringa
	}

	return 0;
}

void printPack (mail *pack){
	printf ("######[][]I METADATI SONO[][]######\n");
	printf ("Dim pack = %ld\n", pack->md.dim);
	printf ("Type = %d[%s]\n", pack->md.type, typeToText (pack->md.type));
	printf ("Sender = %s\n", pack->md.sender);
	printf ("whoOrWhy = %s\n", pack->md.whoOrWhy);
	printf ("------[][]IL MESSAGGIO[][]------\n");
	printf ("TEXT :\n--> %p\n\n", pack->mex); //non sempre stringa
}

void printTextPack (mail *pack){
	if (pack)
		if (pack->mex)
			printf ("(%s):\t%s\n\n", pack->md.sender, (char *)pack->mex);
}




///         ### Client FUNCTION ###



int initClient (connection *c){
	if (connect (c->ds_sock, (struct sockaddr *)&c->sock, sizeof (c->sock))){
		perror ("Connect error; cause:");
		close (c->ds_sock);
		return -1;
	}
	return 0;
}

char *obtainStr (char *buff, int len){
	char bufApp[2048];
antiSegFault:
	scanf ("%[^\n]", bufApp);
	while (getchar () != '\n');
	//if (buff == NULL) goto antiSegFault;
	strncpy (buff, bufApp, len);
	buff[len - 1] = '\0';
	return buff;
}

int loginUser (int ds_sock, mail *pack){

rescue: //risolvere consistenza stringhe

	printf ("Fase di Login; inserire Username ed UserID\n");
	printf ("USER (max 23 caratteri) >>> ");
	obtainStr (UserName, lenStr);

	printf ("\nUserID univoco >>> ");
	obtainStr (UserID, lenStr);
	printf ("\n");

	if (fillPack (pack, login_p, 0, NULL, UserName, UserID) == -1){ //utente e id saranno passati da scanf
		return -1;
	}
	if (writePack (ds_sock, *pack) == -1){
		return -1;
	}
	//Pacchetto mandato, in attesa di risposta server
	if (readPack (ds_sock, pack) == -1){
		return -1;
	}

	switch (pack->md.type){
		case dataUs_p: // otteniamo sempre una tabella (anche vuota, dove scrivere le chat)
			printf ("Login effettuato come %s, %s\n", UserID, UserName);
			return 0;


		case failed_p:
			printf ("Login non effettuato\nServer Report: %s\n", pack->md.whoOrWhy);
			errno = ENOMEM;
			return -1;

		default:
			printf ("Unespected behaviour from server.\n");
			errno = EINVAL;
			return -1;

	}
}

int registerUser (int ds_sock, mail *pack){

	printf ("Creazione Utente; inserire nuovo Username\n");
	printf ("USER (max 23 caratteri) >>> ");
	obtainStr (UserName, lenStr);
	printf ("\n");

	if (fillPack (pack, mkUser_p, 0, NULL, UserName, NULL) == -1){ //id sara' dato dal server
		return -1;
	}
	if (writePack (ds_sock, *pack) == -1){
		return -1;
	}
	//Pacchetto mandato, in attesa di risposta server
	if (readPack (ds_sock, pack) == -1){
		return -1;
	}

	switch (pack->md.type){
		case dataUs_p: // otteniamo sempre una tabella (anche vuota, dove scrivere le chat)
			//UserID = malloc(wowDim);
			strcpy (UserID, pack->md.whoOrWhy); //cosi' non dovrei avere problemi con la deallocazione di pack

			printf ("### UserID = %s ### (chiave d'accesso per successivi login)\n", UserID);
			int id = (int)strtol (pack->md.whoOrWhy, NULL, 10);
			return id;


		case failed_p:
			printf ("Creazione non effettuata\nServer Report: %s\n", pack->md.whoOrWhy);
			errno = ENOMEM;
			return -1;

		default:
			printf ("Unespected behaviour from server.\n");
			errno = EINVAL;
			return -1;

	}
}

table *initClientTable (table *tabChats, mail *pack){

	if (StartClientStorage ("ChatList") == -1){
		return NULL;
	}
	FILE *temp = fopen (chatTable, "w+");

	if (fileWrite (temp, pack->md.dim, 1, pack->mex) == -1){
		printf ("Error writing file\n");
		return NULL;
	}
	fclose (temp);

	tabChats = open_Tab (chatTable);

	return tabChats;
}

void printChats (table *tabChats){
	printf ("\t**********\n");
	for (int i = 0; i < tabChats->head.len; i++){
		printf ("\t%s\n", tabChats->data[i].name);
	}
	printf ("\t**********\n");
}

conversation *startConv (mail *pack, conversation *conv){
	conv = openConf (convName);
	printf("The entire chat conversation has been received;\nWould you print it? (y/N)\n>>> ");

	char buff[16];
	obtainStr(buff, 16);
	if (strcmp(buff, "y") == 0) printConv(conv, STDOUT_FILENO);
	return conv;
}

/* #### SCELTA AZIONI ####*/

int chooseAction (char *command, connection *con, mail *pack, table *tabChats){

	int value = -1;

	if (strcmp (command, "createChat") == 0 || strtol (command, NULL, 10) == 1){
		value = createChat (con->ds_sock, pack, tabChats);
		if (value == -1){
			printf ("Creation failed.\n");
			return -1;
		}
	}
	else if (strcmp (command, "deleteChat") == 0 || strtol (command, NULL, 10) == 2){
		if (deleteChat (con->ds_sock, pack, tabChats) == -1){
			printf ("Unable to delete the chat. Returning to chat selection...\n");
			return -1;
		}
	}
	else if (strcmp (command, "leaveChat") == 0 || strtol (command, NULL, 10) == 3){
		if (leaveChat (con->ds_sock, pack, tabChats) == -1){
			printf ("Unable to delete the chat. Returning to chat selection...\n");
			return -1;
		}
	}
	else if (strcmp (command, "joinChat") == 0 || strtol (command, NULL, 10) == 4){
		if (joinChat (con->ds_sock, pack, tabChats) == -1){
			printf ("Unable to join the chat. Returning to chat selection...\n");
			return -1;
		}
	}
	else if (strcmp (command, "openChat") == 0 || strtol (command, NULL, 10) == 5){
		value = openChat (con->ds_sock, pack, tabChats);
		if (value == -1){
			printf ("Unable to open the chat. Returning to chat selection...\n");
			return -1;
		}
	}
	else if (strcmp (command, "printTab") == 0 || strcmp (command, "$p") == 0){
		tabPrint (tabChats);
		printf ("\n\n\n");
	}
	else{
		helpChat ();
		return -1;
	}
	printf ("Index Of My Tab = %d\n", value);
	return value;
}

/* ##### AZIONI DELLE SCELTE SELEZIONATE ##### */

int createChat (int ds_sock, mail *pack, table *tabChats){
	printf ("Creazione nuova chat; scegliere il nome (max 23 caratteri):\n");

	char buff[1024];
	obtainStr (buff, 1024);
	// va in mex il nome della chat perche' lato server l'id serve a definire l'amministratore della chat
	fillPack (pack, mkRoom_p, strlen (buff) + 1, buff, UserName, UserID);

	if (writePack (ds_sock, *pack) == -1){
		return -1;
	}
	//Pacchetto mandato, in attesa di risposta server
	if (readPack (ds_sock, pack) == -1){
		return -1;
	}
	switch (pack->md.type){
		case success_p:
			// (anche vuota, dove scrivere le chat)
			printf ("Creazione effettuata\n<Id>:<Nome_Room> = %s\n", pack->md.whoOrWhy);
			addEntry (tabChats, pack->md.whoOrWhy, 0);
			return searchFirstOccurrence (tabChats, pack->md.whoOrWhy);


		case failed_p:
			printf ("Creazione non effettuata\nServer Report: %s\n", pack->md.whoOrWhy);
			errno = ENOMEM;
			return -1;

		default:
			printf ("Unespected behaviour from server.\n");
			errno = EINVAL;
			return -1;

	}
}

int deleteChat (int ds_sock, mail *pack, table *tabChats){
	//possibile solo se l'utente e' ADMIN di tale chat
	printf ("Eliminazione chat; scegliere l'ID:\n");

	char buff[1024];
	obtainStr (buff, 1024);

	int indexEntry = searchFirstOccurrenceKey (tabChats, (int)strtol (buff, NULL, 10));
	if (indexEntry == -1){
		printf ("ID not found.\n");
		return -1;
	}

	char newBuff[wowDim]; //non e' un problema se di dimensione fissa, perche' prima cerchiamo se esista!
	sprintf (newBuff, "%s:%d", buff, indexEntry); // idKey:EntryPosition

	char userBuff[sendDim];
	sprintf (userBuff, "%s:%s", UserID, UserName); // UserID:UserName

	fillPack (pack, delRm_p, 0, NULL, userBuff, newBuff);

	if (writePack (ds_sock, *pack) == -1){
		return -1;
	}

	/*============================== Wait Server Pack ==============================*/

	if (readPack (ds_sock, pack) == -1){
		return -1;
	}
	switch (pack->md.type){
		case success_p:
			// (anche vuota, dove scrivere le chat)
			printf ("Eliminazione effettuata\n<Id>:<Nome_Room> = %s\n", pack->md.whoOrWhy);
			delEntry (tabChats, indexEntry);
			return 0;


		case delRm_p:
			printf ("Eliminazione gia' effettuata da qualcuno.\nServer Report: %s\n", pack->md.whoOrWhy);
			delEntry (tabChats, indexEntry);
			errno = EINVAL;
			return -1;


		case failed_p:
			printf ("Eliminazione non effettuata\nServer Report: %s\n", pack->md.whoOrWhy);
			errno = ENOMEM;
			return -1;


		default:
			printf ("Unespected behaviour from server.\n");
			errno = EINVAL;
			return -1;

	}
}

int leaveChat (int ds_sock, mail *pack, table *tabChats){
	// possibile per qualunque utente; se era l'ultimo della chat,
	// il server elimina tutta la persistenza della chat
	printf ("Leaving chat; scegliere l'ID:\n");

	char buff[1024];
	obtainStr (buff, 1024);

	int indexEntry = searchFirstOccurrenceKey (tabChats, (int)strtol (buff, NULL, 10));
	if (indexEntry == -1){
		printf ("ID not found.\n");
		return -1;
	}

	char newBuff[wowDim];
	sprintf (newBuff, "%s:%d", buff, indexEntry); // idKey:EntryPosition

	char userBuff[sendDim];
	sprintf (userBuff, "%s:%s", UserID, UserName); // UserID:UserName


	fillPack (pack, leaveRm_p, 0, NULL, userBuff, newBuff);

	if (writePack (ds_sock, *pack) == -1){
		return -1;
	}

	/*============================== Wait Server Pack ==============================*/

	if (readPack (ds_sock, pack) == -1){
		return -1;
	}
	switch (pack->md.type){
		case success_p:
			// (anche vuota, dove scrivere le chat)
			printf ("Leave riuscito\n");
			delEntry (tabChats, indexEntry);
			return 0;


		case delRm_p:
			printf ("Eliminazione gia' effettuata da qualcuno.\nServer Report: %s\n", pack->md.whoOrWhy);
			delEntry (tabChats, indexEntry);
			errno = EINVAL;
			return -1;


		case failed_p:
			printf ("Leave non effettuato\nServer Report: %s\n", pack->md.whoOrWhy);
			errno = ENOMEM;
			return -1;

		default:
			printf ("Unespected behaviour from server.\n");
			errno = EINVAL;
			return -1;

	}
}

int joinChat (int ds_sock, mail *pack, table *tabChats){
	printf ("Join nuova chat; scrivere ID.\n<ID>: ");

	char buff[1024];
	obtainStr (buff, 1024);

	int numEntry = searchFirstOccurrenceKey (tabChats, (int)strtol (buff, NULL, 10));
	if (numEntry != -1){
		printf ("Chat < %s > already exists, please use 'openChat'/'4' + 'chatName'.\n", buff);
		return -1;
	}

	fillPack (pack, joinRm_p, 0, NULL, UserName, buff);

	if (writePack (ds_sock, *pack) == -1){
		return -1;
	}

	/*============================== Wait Server Pack ==============================*/

	if (readPack (ds_sock, pack) == -1){
		return -1;
	}
	switch (pack->md.type){
		case dataRm_p:
			printf ("Join effettuato\n");
			entry *newChat = (entry *)pack->mex;
			addEntry (tabChats, newChat->name, newChat->point);
			return 0;


		case failed_p:
			printf ("Join non effettuato\nServer Report: %s\n", pack->md.whoOrWhy);
			errno = ENOMEM;
			return -1;

		default:
			printf ("Unespected behaviour from server.\n");
			errno = EINVAL;
			return -1;

	}
}

int openChat (int ds_sock, mail *pack, table *tabChats){
	printf ("Selezione chat esistente; scegliere il nome:\n");

	char buff[1024];
	obtainStr (buff, 1024);

	int numEntry = searchFirstOccurrenceKey (tabChats, atoi (buff));
	if (numEntry == -1){
		printf ("Chat not exists, please choose one of the following, or create one.\n");
		return -1;
	}

	char newBuff[wowDim]; //non e' un problema se di dimensione fissa, perche' prima cerchiamo se esista!
	sprintf (newBuff, "%s:%d", buff, numEntry); // idKey:EntryPosition

	char userBuff[sendDim];
	sprintf (userBuff, "%s:%s", UserID, UserName); // UserID:UserName

	fillPack (pack, openRm_p, 0, NULL, userBuff, newBuff);

	if (writePack (ds_sock, *pack) == -1){
		return -1;
	}

	/*============================== Wait Server Pack ==============================*/

	int i = 0;
	mex *mexBuff[4096];
retry: // label per aggiungere i mess se ne sono arrivati nel frattempo
	if (readPack (ds_sock, pack) == -1){
		return -1;
	}
	switch (pack->md.type){
		case kConv_p:
			// (anche vuota, dove scrivere le chat)
			printf ("Open successful\n");
			sprintf (convName, "convList_%s.conv", pack->md.whoOrWhy); // in WoW il nome della room

			FILE *temp = fopen (convName, "w+");
			if (fileWrite (temp, pack->md.dim, 1, pack->mex) == -1){
				printf ("Error writing file\n");
				return -1;
			}
			for (int j = 0; j < i; j++){
				saveNewMexF (mexBuff[j], temp);
			}
			fclose (temp);

			while (i > 0){ //if null is error
				freeMex (mexBuff[i]);
				mexBuff[i] = NULL;
				i--;
			}
			printf ("Save K-conv successful\n");
			return numEntry;


		case mess_p: //mi e' arrivato un messaggio prima della conversazione
			mexBuff[i] = makeMexBuf (pack->md.dim, pack->mex);
			if (!mexBuff[i]){  //if null is error
				while (i > 0){
					freeMex (mexBuff[i]);
					mexBuff[i] = NULL;
					i--;
				}
			}
			i++;
			goto retry;


		case failed_p:
			printf ("Open error.\nServer Report: %s\n", pack->md.whoOrWhy);
			errno = ENOMEM;
			while (i > 0){
				freeMex (mexBuff[i]);
				mexBuff[i] = NULL;
				i--;
			}
			return -1;


		case delRm_p: //La chat e' stata cancellata e non e' piu' esistente
			printf ("Open error. %s is a no-existing more chat. Deleting...\n", buff);
			delEntry (tabChats, numEntry);
			while (i > 0){
				freeMex (mexBuff[i]);
				mexBuff[i] = NULL;
				i--;
			}
			return -1;


		default:
			printf ("Unespected behaviour from server.\n");
			errno = EINVAL;
			while (i > 0){
				freeMex (mexBuff[i]);
				mexBuff[i] = NULL;
				i--;
			}
			return -1;

	}
}

void helpChat (void){
	printf ("\n\nCreateChat: crea un nuovo canale di conversazione.\n");
	printf ("DeleteChat: elimina un canale di conversazione (possibile solo se si e' admin dello stesso).\n");
	printf ("LeaveChat: lascia un canale di conversazione.\n");
	printf ("OpenChat: apre un canale esistente di conversazione.\n");
	printf ("JoinChat: partecipa a un nuovo canale di conversazione.\n");
}