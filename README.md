#clientDemo

Progetto di sistemi operativi del professore Quaglia:

Realizzazione di un servizio "chat" via internet offerto tramite server
che gestisce un insieme di processi client (residenti, in generale, su
macchine diverse). Il server deve essere in grado di acquisire in input
da ogni client una linea alla volta e inviarla a tutti gli altri client
attualmente presenti nella chat.
Ogni client potrà decidere se creare un nuovo canale di conversazione,
effettuare il join ad un canale già esistente, lasciare il canale,
o chiudere il canale (solo nel caso che l'abbia istanziato).

_Si precisa che la specifica richiede la realizzazione del software sia per
l'applicazione client che per l'applicazione server._

Per progetti misti Unix/Windows e' a scelta quale delle due applicazioni
sviluppare per uno dei due sistemi.


Questo repository contiene la parte client del programma

il progetto principale si trova a :

https://github.com/Alfystar/Chat-multicanale-Linux-Windows

Il tutto fa parte del progetto per il server multi chat di sistemi operativi.

La libreria è scritta in standard C, eccetto per la open del file

Questa operazione tramite dei define, verrà calibrata tra windows e linux per rendere la libreria uniersale