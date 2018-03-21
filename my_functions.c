#include "my_types.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

/*--------------------------------------------------------------+
| Stampa l'ultimo errore e esce con codice EXIT_FAILURE			|
| Vaalore di ritorno: void										|
| Parametri: char *s (per capire dove si è generato l'errore) 	|
+---------------------------------------------------------------*/
void errExit(char *s) {
	FILE *fp;
	if ((fp = fopen(LOGPATH, "a+"))) {
		fprintf(fp, "%s\n", s);
		fclose(fp);
	}
	perror(s);
	exit(EXIT_FAILURE);
}

/*--------------------------------------------------------------+
|Inizializza il semaforo associato a semId al valore val 		|
|Valore di ritorno: int (0, -1 in caso di errore)				|
|Parametri: semId (id del semaforo), int semNum (Numero del 	|
|semaforo), int val (valore a cui inizializzare)				|
+---------------------------------------------------------------*/
int initSem(int semId, int semNum, int val) {
		union semun arg;

		arg.val = val;
		return semctl(semId, semNum, SETVAL, arg);

} 

/*--------------------------------------------------------------+
|Esegue un operazione P sul semaforo associato a semId 			|
|Valore di ritorno: int (0, -1 in caso di errore)	  			|
|Parametri:  int semId (id del semaforo), int semNum (numero 	|
|del  semaforo)								  					|
+---------------------------------------------------------------*/
int reserveSem(int semId, int semNum) {
	struct sembuf sops;

	sops.sem_num = semNum;
	sops.sem_op = -1;
	sops.sem_flg = 0;

	return semop(semId, &sops, 1);
}

/*--------------------------------------------------------------+
|Esegue un operazione V sul semaforo associato a semId			|
|Valore di ritorno: int (0, -1 in caso di errore)	  			|
|Parametri:  int semId (id del semaforo), int semNum (numero del|
|semaforo)								  						|
+---------------------------------------------------------------*/
int releaseSem(int semId, int semNum) {
	struct sembuf sops;

	sops.sem_num = semNum;
	sops.sem_op = 1;
	sops.sem_flg = 0;

	return semop(semId, &sops, 1);
}

/*--------------------------------------------------------------+
|Controlla se il semaforo associato a semId è libero			|
|Valore di ritorno: int (0, 1)									|
|Parametri:  int semId (id del semaforo), int semNum (numero del|
|semaforo)														|
+---------------------------------------------------------------*/
int semAvailable(int semId, int semNum) {
	union semun arg;
	int n;

	n = semctl(semId, semNum, GETVAL, arg);

	if (n == -1) errExit("[ERRORE]semAvailable: semctl");
	return (n > 0); 
}

/*--------------------------------------------------------------+
|Assegna ad un paziente sintomo casuale e rispettivo codice di 	|
|gravità leggendo da FILEPATH									|
|Valore di ritorno: void										|
|Parametri: patient_t *p (puntatore a una struttura paziente di | 
|cui modificare i campi)										|
+---------------------------------------------------------------*/
void assignSymp(patient_t *p) {
	FILE *fp;
	int n, iterator, s;
	char buff[BUFFSIZE];

	n = rand() % 10;

	if ((fp = fopen(FILEPATH, "r"))) {
		for (iterator = 0; iterator <= n; ++iterator)
			fscanf(fp, "%s %d", buff, &s);
		fclose(fp);
	}else errExit("[ERRORE]fopen");
	p->severity = s;
		
}

/*--------------------------------------------------------------+
|Attende un tempo casuale (compreso tra 0 e gravità del paziente|
|) di secondi poi invia un segnale al processo che ha creato il	|
|paziente 														|
+---------------------------------------------------------------*/
void giveService(patient_t p) {
	sleep(rand() % p.severity);
	printf("[PRESTAZIONE]Paziente [%d] sono stato servito!\n\n", p.ID);
}

/*--------------------------------------------------------------+
|Trasforma un intero in una stringa 							|
|Valore di ritorno: char * (la stringa su cui è stato scritto)	|
|Parametri: int (da trasformare) char* (destinazione)			|
+---------------------------------------------------------------*/
char *my_itoa(int n, char* N) {
	sprintf(N, "%d", n);
	return N;
}

/*--------------------------------------------------------------+
|Aggiunge un elemento alla fine di una lista di interi			|
|Valore di ritorno: void										|
|Parametri:i_node**(puntatore a testa),int(valore dell'elemento)|
+---------------------------------------------------------------*/
void i_addToList(i_node **h, int v) {
	i_node *i, *new;
	i = *h;
	new = (i_node *)malloc(sizeof(i_node));
	new->val = v;
	new->next = NULL;
	if (*h == NULL)	*h = new;
	else {
		while(i->next != NULL) i = i->next;
		i->next = new;
	}
}

/*--------------------------------------------------------------+
|Inserisce un elemento alla fine di una lista di pazienti		|
|Valore di ritorno: void										|
|Parametri: node** (puntatore alla testa), patient_t (paziente 	|
|da inserire)													|
+---------------------------------------------------------------*/
void addToList(node **h, patient_t p) {
	node *i, *new;
	i= *h;
	new = (node *)malloc(sizeof(node));
	new->p  = p;
	new->next = NULL;

	if (*h == NULL) *h = new;
	else {
		while (i->next != NULL) i = i->next;
		i->next = new;
	}
}

/*--------------------------------------------------------------+
|Rimuove l'elemento con il campo paziente corrispondente		|
|Valore di ritorno: void										|
|Parametri node** (puntatore a testa), int (ID del paziente da 	|
|rimuovere)														|
+---------------------------------------------------------------*/
void deletePatient(node **h, int pID) {
	node *tmp, *curr, *prev;
	if ((*h)->p.ID == pID) {
		tmp = *h;
		*h = (*h)->next;
		free(tmp);
		return;
	}
	curr = (*h)->next;
	prev = *h;
	while (curr != NULL && prev != NULL) {
		if (curr->p.ID == pID) {
			tmp = curr;
			prev->next = curr->next;
			free(tmp);
			return;
		}
		prev = curr;
		curr = curr->next;
	} 
	return;
}

/*--------------------------------------------------------------+
|Cerca il primo paziente con gravità maggiore (oppure che sta 	|
|aspettando da più tempo) [si assumono i pazienti nella lista 	|
|in ordine di arrivo]											|
|Valore di ritorno: patient_t (paziente corrispondente ai 		|
|criteri)														|
|Parametri: node** (puntatore alla testa)						|
+---------------------------------------------------------------*/
patient_t calcNext(node **h) {
	node *i;
	i = *h;
	while (i != NULL) {
		if (i->p.age >= TRESHOLD){
			printf("[CALCOLO TURNO]Paziente [%d] ha superato il tempo limite di attesa!(%fs)\n\n", i->p.ID, i->p.age);
			return i->p;
		}
		else if (i->p.severity >= maxSeverity(h)){
			printf("[CALCOLO TURNO]Paziente [%d] ha gravità maggiore!(%d)\n\n",i->p.ID, i->p.severity);
			return i->p;
			}
		i = i->next;
		}
	printf("[CALCOLO TURNO]Nessuno da servire!\n\n"); 
	return i->p;
}

/*--------------------------------------------------------------+
|Calcola la gravità più alta nella lista di pazienti			|
|Valore di ritorno: int (gravità massima)						|
|Parametri: node** (puntatore a testa )							|
+---------------------------------------------------------------*/
int maxSeverity(node **h) {
	int max = 0;
	node *i = *h;
	while (i != NULL) {
		if (i->p.severity >= max)
			max = i->p.severity;
		i = i->next;
	}
	return max;
}

/*--------------------------------------------------------------+
|Libera la memoria occupata da un lista di interi				|
|Valore di ritorno: void										|
|Parametri: i_node** (puntatore a testa)						|
+---------------------------------------------------------------*/
void i_freeList(i_node **h) {
	i_node *tmp;
	while((tmp = *h) != NULL) {
		*h = tmp->next;
		free(tmp);
	}
}

/*--------------------------------------------------------------+
|Aggiorna il valore dell'età dei pazienti in una lista			|
|Valore di ritorno: void										|
|Parametri: node** (puntatore a testa), int (tempo trascorso)	|
+---------------------------------------------------------------*/
void refreshAges(node **h, double elapsedTime) {
	node *i;
	i = *h;
	while(i != NULL) {
		i->p.age = elapsedTime - (i->p.insTime);
		i = i->next;
	}
}

