#define MAX_ACCESS 10 /*Numero massimo di pazienti esaminati*/
#define MAX_WARDS 2 /*Numero di reparti*/
#define TIMER 20 /*Tempo prima del SIGALRM*/
#define FILEPATH "sintomi.txt" /*Path del file con i sintomi*/
#define TRESHOLD 9 /*Massima attesa per un paziente*/
#define WARD_INTERVAL 3 /*Ogni quanto i pazienti vengono visitati*/
#define LOGPATH "log.txt" /*Path del file dei log*/

#define EOS_PATIENT -1
#define TMP_STOP -999
#define BUFFSIZE 32

/*---------------Chiavi per le varie risorse---------------*/
#define SEM_KEY 1994
#define TRIAGE_MSGQ_KEY 705
#define S_START_MSGQ_KEY 507
#define START_KEY 7594
#define SHM_KEY 2016
/*---------------------------------------------------------*/

/*Dichiarazione del tipo ptient_t*/
typedef struct {
	int ID; /*ID paziente*/
	int  severity;/*Grazità*/
	double insTime; /*Tempo di arrivo*/
	double age;/*Tempo di attesa*/
}patient_t;

/*Struttura dei messaggi per msgsnd e msgrcv*/
struct mymsg {
	long mtype;/*Tipo di messaggio*/
	patient_t patient;/*Paziente*/
};

/*Dichiarazione del tipo node, per le liste di pazienti*/
typedef struct node_s{
	patient_t p;/*Dati paziente*/
	struct node_s *next;/*Puntatore all'elemento successivo*/
}node;

/*Dichiarazione del tipo i_node, per le liste di interi*/
typedef struct i_node_s {
	int val;/*Valore dell'elemento*/
	struct i_node_s *next;/*Puntatore all'elemento successivo*/
}i_node;

/*Per semctl quando è necessario arg*/
union semun {
	int val;
	struct semid_ds* buf;
	unsigned short* array;
#if defined(_linux_)
		struct seminfo* _buf;
#endif
};

/*--------------------------------------------------------------+
| Stampa l'ultimo errore e esce con codice EXIT_FAILURE			|
| Vaalore di ritorno: void										|
| Parametri: char *s (per capire dove si è generato l'errore) 	|
+---------------------------------------------------------------*/
void errExit(char *s);

/*--------------------------------------------------------------+
|Inizializza il semaforo associato a semId al valore val 		|
|Valore di ritorno: int (0, -1 in caso di errore)				|
|Parametri: semId (id del semaforo), int semNum (Numero del 	|
|semaforo), int val (valore a cui inizializzare)				|
+---------------------------------------------------------------*/
int initSem(int semId, int semNum, int val);

/*--------------------------------------------------------------+
|Esegue un operazione P sul semaforo associato a semId 			|
|Valore di ritorno: int (0, -1 in caso di errore)	  			|
|Parametri:  int semId (id del semaforo), int semNum (numero 	|
|del  semaforo)								  					|
+---------------------------------------------------------------*/
int reserveSem(int semId, int semNum);

/*--------------------------------------------------------------+
|Esegue un operazione V sul semaforo associato a semId			|
|Valore di ritorno: int (0, -1 in caso di errore)	  			|
|Parametri:  int semId (id del semaforo), int semNum (numero del|
|semaforo)								  						|
+---------------------------------------------------------------*/
int releaseSem(int semId, int semNum);

/*--------------------------------------------------------------+
|Controlla se il semaforo associato a semId è libero			|
|Valore di ritorno: int (0, 1)									|
|Parametri:  int semId (id del semaforo), int semNum (numero del|
|semaforo)														|
+---------------------------------------------------------------*/
int semAvailable(int semId, int semNum);

/*--------------------------------------------------------------+
|Assegna ad un paziente sintomo casuale e rispettivo codice di 	|
|gravità leggendo da FILEPATH									|
|Valore di ritorno: void										|
|Parametri: patient_t *p (puntatore a una struttura paziente di | 
|cui modificare i campi)										|
+---------------------------------------------------------------*/
void assignSymp(patient_t *p);

/*--------------------------------------------------------------+
|Attende un tempo casuale (compreso tra 0 e gravità del paziente|
|) di secondi poi invia un segnale al processo che ha creato il	|
|paziente 														|
+---------------------------------------------------------------*/
void giveService(patient_t p);

/*--------------------------------------------------------------+
|Trasforma un intero in una stringa 							|
|Valore di ritorno: char * (la stringa su cui è stato scritto)	|
|Parametri: int (da trasformare) char* (destinazione)			|
+---------------------------------------------------------------*/
char *my_itoa(int n, char* N);

/*--------------------------------------------------------------+
|Aggiunge un elemento alla fine di una lista di interi			|
|Valore di ritorno: void										|
|Parametri:i_node**(puntatore a testa),int(valore dell'elemento)|
+---------------------------------------------------------------*/
void i_addToList(i_node **h, int v);

/*--------------------------------------------------------------+
|Inserisce un elemento alla fine di una lista di pazienti		|
|Valore di ritorno: void										|
|Parametri: node** (puntatore alla testa), patient_t (paziente 	|
|da inserire)													|
+---------------------------------------------------------------*/
void addToList(node **h, patient_t p);

/*--------------------------------------------------------------+
|Rimuove l'elemento con il campo paziente corrispondente		|
|Valore di ritorno: void										|
|Parametri node** (puntatore a testa), int (ID del paziente da 	|
|rimuovere)														|
+---------------------------------------------------------------*/
void deletePatient(node **h, int pID);

/*--------------------------------------------------------------+
|Cerca il primo paziente con gravità maggiore (oppure che sta 	|
|aspettando da più tempo)										|
|Valore di ritorno: patient_t (paziente corrispondente ai 		|
|criteri)														|
|Parametri: node** (puntatore alla testa)						|
+---------------------------------------------------------------*/
patient_t calcNext(node **h);

/*--------------------------------------------------------------+
|Calcola la gravità più alta nella lista di pazienti			|
|Valore di ritorno: int (gravità massima)						|
|Parametri: node** (puntatore a testa )							|
+---------------------------------------------------------------*/
int maxSeverity(node **h);

/*--------------------------------------------------------------+
|Libera la memoria occupata da un lista di interi				|
|Valore di ritorno: void										|
|Parametri: i_node** (puntatore a testa)						|
+---------------------------------------------------------------*/
void i_freeList(i_node **h);

/*--------------------------------------------------------------+
|Aggiorna il valore dell'età dei pazienti in una lista			|
|Valore di ritorno: void										|
|Parametri: node** (puntatore a testa), int (tempo trascorso)	|
+---------------------------------------------------------------*/
void refreshAges(node **h, double elapsedTime);

