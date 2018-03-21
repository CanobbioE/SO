#include "my_types.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

/*Variabili utilizzate per coordinare il programma e l'handler*/
volatile int recived_alarm = 0;
volatile int usr_interrupt = 0;

/*Handler dei segnali*/
static void handler(int sig) {
	if (sig == SIGALRM) {
		/*Preferibilmente sarebbe meglio non stampare nulla da un handler*/
		printf("\n[MAIN]Ricevuto SIGALRM, impedisco altri accessi.\n\n");
		recived_alarm = 1;
		return;
	}
	if (sig == SIGQUIT) {
		return;
	}
	if (sig == SIGUSR1) {
		usr_interrupt = 1;
		return;
	}
}

int main(int argc, char const *argv[]) {

	sigset_t mask, oldmask;
	patient_t patient, EOS_patient, TMP_STOP_patient;
	struct mymsg msgp;
	int semId, t_msgq_id, s_msgq_id, mutex;
	int i;
	char myfifo[15], N[3];
	int pCount = 0;
	i_node *head = NULL;
	i_node *tmp;

	FILE *fp;
	


	EOS_patient.ID = EOS_PATIENT;
	TMP_STOP_patient.ID = TMP_STOP;

	

	/*---------------Creazione delle risorse---------------*/
	
	/*Semaforo: valore iniziale = MAX_ACCESS specificato in my_types.h*/
	semId = semget(SEM_KEY, 1, IPC_CREAT | 0666);
	if (semId == -1) errExit("[ERRORE]semget 1");
	if (initSem(semId, 0, MAX_ACCESS) == -1) errExit("[ERRORE]initSem");

	/*Semaforo: coordina invio TMP_STOP*/
	mutex = semget(ftok(FILEPATH, 1), 1, IPC_CREAT | 0666);
	if (mutex == -1) errExit("[ERRORE]semget 2");
	if (initSem(mutex, 0, 1) == -1) errExit("[ERRORE]initSem 2");


	/*Coda di messaggi: da entrata verso il triage */
	t_msgq_id = msgget(TRIAGE_MSGQ_KEY, IPC_CREAT | 0666);
	if (t_msgq_id == -1) errExit("[ERRORE]msgget 1");

	/*Code di messaggi: dai reparti verso prestazione */
	for(i = 1; i <= MAX_WARDS; ++i) {
		s_msgq_id = msgget(S_START_MSGQ_KEY + i, IPC_CREAT | 0666);
		if (s_msgq_id == -1) errExit("[ERRORE]msgget 2");
		i_addToList(&head, s_msgq_id);
	}	
	
	/*----------------Fine creazione risorse----------------*/



/*###############################################################################*/
/*#############AVVIO DI TRIAGE, N (MAX_WARDS) REPARTI, N PRESTAZIONI#############*/
	
	/*-------------------------------Avvio triage--------------------------------*/
	switch (fork()) {
		case -1:
			errExit("[ERRORE]fork 0");
		case 0:
			execl("triage", "triage", NULL);
			errExit("[ERRORE]execl 1");
	}

 	/*-------------------Avvio di N reaprti e altrettante FIFO-------------------*/
	for(i = 1; i <= MAX_WARDS; ++i) {
		switch (fork()) {
			case -1:
				errExit("[ERRORE]fork 2");
			case 0:
				/*Viene creata una fifo per ogni reparto*/
		        sprintf(myfifo, "./tmp/myfifo%d", i);
		        if (mkfifo(myfifo, 0666) == -1) errExit("[ERRORE]mkfifo");
		        /*Apertura del reparto*/
				execl("reparto", "reparto", my_itoa(i,N),  NULL);
				errExit("[ERRORE]execl 2");
		}
	}

	/*--------------------------Avvio di N prestazioni---------------------------*/
	for (i = 1; i <= MAX_WARDS; ++i) {
		switch (fork()) {
			case -1:
				errExit("[ERRORE]fork 3");
			case 0:
				execl("prestazione", "prestazione", my_itoa(i,N), NULL);
				errExit("[ERRORE]execl 3");
		}
	}
/*NB per fork() in cui il figlio ha il solo scopo di chiamare una execl è
preferibile usare vfork() poichè evitiamo di copiare l'intera immagine del del 
processo genitore*/
/*###################################FINE AVVIO##################################*/
/*###############################################################################*/

	/*-------------Associazione segnale-handler-------------*/
	if (signal(SIGALRM, handler) == SIG_ERR) errExit("[ERRORE]signal 2");
	if (signal(SIGQUIT, handler) == SIG_ERR) errExit("[ERRORE]signal 3");

	/*Creazione del timer, dopo TIMER (specificato in my_types.h) 
	secondi il sistema impedisce ad altri pazienti di accedere*/
	alarm(TIMER);
	printf("[MAIN]Timer di %d secondi avviato.\n\n",TIMER);

	if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
	fprintf(fp, "[MAIN]Timer di %d secondi avviato.\n\n",TIMER);
	fclose(fp);


	/*Entra in un loop "infinito" fintanto che non si riceve SIGALRM*/
	while (!recived_alarm) {
		if (semAvailable(semId, 0)) {
			switch (fork()) {
				case -1:
					errExit("[ERRORE]fork 1");
				case 0: /*Figlio: tenta di accedere all'ospedale*/
					
					if (semAvailable(semId, 0)) {
						/*Set up gestione segnale*/
						if (signal(SIGUSR1, handler) == SIG_ERR) errExit("[ERRORE]signal 1");
						sigemptyset (&mask);
						sigaddset (&mask, SIGUSR1);

						/*compilazione dei campi paziente*/
						patient.ID = getpid();
						msgp.mtype = 1;
						msgp.patient = patient;

						printf("[MAIN]Paziente [%d] tento di accedere all'ospedale.\n\n", patient.ID);

						if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
						fprintf(fp, "[MAIN]Paziente [%d] tento di accedere all'ospedale.\n\n", patient.ID);
						fclose(fp);

						if (reserveSem(semId, 0) == -1) errExit("[ERRORE]reserveSem");
						printf("[MAIN]Paziente [%d] il semaforo era libero, accedo all'ospedale.\n\n", patient.ID);

						if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
						fprintf(fp, "[MAIN]Paziente [%d] il semaforo era libero, accedo all'ospedale.\n\n", patient.ID);
						fclose(fp);

						if (msgsnd(t_msgq_id, &msgp, sizeof(patient_t), 0) == -1) errExit("[ERRORE]msgsnd 1");
						
						/*attende sigusr1*/
 						sigprocmask (SIG_BLOCK, &mask, &oldmask);
						while (!usr_interrupt) sigsuspend (&oldmask);
						sigprocmask (SIG_UNBLOCK, &mask, NULL);

						if (releaseSem(semId, 0) == -1) errExit("[ERRORE]releaseSem");
						printf("[MAIN]Paziente [%d] ho liberato il semaforo e ora esco.\n\n",patient.ID );

						if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
						fprintf(fp, "[MAIN]Paziente [%d] ho liberato il semaforo e ora esco.\n\n",patient.ID );
						fclose(fp);

					}
					exit(EXIT_SUCCESS);

				default:
					++pCount;
			}
		} else { /*Invio il paziente speciale TMP_STOP se il semaforo è bloccato,
		 così che reparto non si blocchi sulla read*/
			if (reserveSem(mutex, 0) == -1) {
				if (errno != EINTR) errExit("[ERRORE]reserveSem 2");
				else break;
			}

			msgp.mtype = 1;
			msgp.patient = TMP_STOP_patient;
			if (msgsnd(t_msgq_id, &msgp, sizeof(patient_t), 0) == -1) errExit("[ERRORE]msgsnd EOS");}
	}
	

	/*Ricevuto SIGALRM invio EOS_patient ai reparti, indicando la fine dei pazienti*/
	msgp.mtype = 1;
	msgp.patient = EOS_patient;
	if (msgsnd(t_msgq_id, &msgp, sizeof(patient_t), 0) == -1) errExit("[ERRORE]msgsnd EOS");


	/*attende tutti i figli*/
	printf("\n[MAIN]Padre [%d] attendo l'uscita dei pazienti.\n\n",getpid());

	if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
	fprintf(fp, "\n[MAIN]Padre [%d] attendo l'uscita dei pazienti.\n\n",getpid());
	fclose(fp);

	while (pCount) {
		wait(NULL);
		--pCount;
	}
	

	printf("\n[MAIN]Tutti i pazienti sono usciti, attendo SIGQUIT.\n\n");

	if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
	fprintf(fp, "\n[MAIN]Tutti i pazienti sono usciti, attendo SIGQUIT.\n\n");
	fclose(fp);

	/*Ora attendiamo il SGQUIT, una volta ricevuto 
	liberiamo tutte le risorse che abbiamo creato */
	pause();
	printf("\n[MAIN]Libero le risorse prima di chiudere.\n");

	if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
	fprintf(fp, "\n[MAIN]Libero le risorse prima di chiudere.\n");
	fclose(fp);

	

 	/*Libero le risorse: semaforo, coda verso triage, code verso prestazioni, FIFO, lista*/
	if (semctl(semId, 0, IPC_RMID) == -1) errExit("[ERRORE]semctl 1");
	if (semctl(mutex, 0, IPC_RMID) == -1) errExit("[ERRORE]semctl 2");
	if (msgctl(t_msgq_id, IPC_RMID, NULL) == -1) errExit("[ERRORE]msgctl 1");
	for (tmp = head; tmp != NULL; tmp = tmp->next) 
		if (msgctl(tmp->val, IPC_RMID, NULL) == -1) errExit("[ERRORE]msgctl 2");
	
	i_freeList(&head);

	for (i = 1; i <= MAX_WARDS; ++i) {
	    sprintf(myfifo, "./tmp/myfifo%d", i);
	    if (unlink(myfifo) == -1) errExit("[ERRORE]unlink");
	}
	exit(EXIT_SUCCESS);
}