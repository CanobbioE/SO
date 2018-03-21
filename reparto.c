#include "my_types.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>


/*--------------------------------------------------------------+
|A intervalli costanti ai pazienti ricevuti da  triage viene	|
|assegnato un numero di turno in base al principio: più gravi 	|
|prima, stessa gravità in ordine di arrivo, evitare la 	 		|
|starvation dei meno gravi					 					|
+---------------------------------------------------------------*/

int main(int argc, char const *argv[]) {

	int ward_num, s_msgq_id, fd, mutex;
	char myfifo[15];
	struct timeval start, end, in, begin;
	double elapsedTime = 0.0, totElapsedTime = 0.0;
	struct mymsg msgp;
	patient_t tmp;
	node *head = NULL;
	int recivedEOS = 0, recivedTmpStop = 0;

	FILE *fp;
	


	ward_num = atoi(argv[1]); /*Il numero del reparto*/


	printf("----------------------AVVIATO REPARTO %d-------------------\n\n", ward_num);
	
	if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
	fprintf(fp,"----------------------AVVIATO REPARTO %d-------------------\n\n", ward_num);
	fclose(fp);

	/*Semaforo: coordina invio TMP_STOP*/
	mutex = semget(ftok(FILEPATH, 1), 1,  0666);
	if (mutex == -1) errExit("[ERRORE]semget 3");

	/*Associazione alla coda di messaggi cvorretta*/
	s_msgq_id = msgget((S_START_MSGQ_KEY + ward_num), 0666);
	if (s_msgq_id == -1) errExit("[ERRORE]msgget 4");

	/*Apertura della FIFO corretta*/
	sprintf(myfifo, "./tmp/myfifo%d", ward_num);
	fd = open(myfifo, O_RDONLY);
	if (fd == -1) errExit("[ERRORE]open 3");


	gettimeofday(&start, NULL);
	gettimeofday(&begin, NULL);

	while (1) {
		
		/*Fintanto che non ho ricevuto l'ultimo paziente leggo*/
		if (!recivedEOS && !recivedTmpStop){

			if (read(fd, &tmp, sizeof(patient_t)) == -1) errExit("[ERRORE]read 1");
			if (tmp.ID == EOS_PATIENT) {
				recivedEOS = 1;
				if (close(fd) == -1)errExit("[ERRORE]close 3");
			}else if (tmp.ID == TMP_STOP) {
				recivedTmpStop = 1;
			}else {
				if (tmp.ID){
				printf("[REPARTO%d]Paziente [%d], sono arrivato nel reparto.\n\n", ward_num, tmp.ID);
				
				if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
				fprintf(fp, "[REPARTO%d]Paziente [%d], sono arrivato nel reparto.\n\n", ward_num, tmp.ID);
				fclose(fp);

				/*assegna tempo di arrivo*/
				gettimeofday(&in, NULL);
				tmp.insTime = (double) (in.tv_usec - start.tv_usec) / 1000000 + (double) (in.tv_sec - start.tv_sec);
				
				/*aggiungi alla lista*/
				addToList(&head, tmp);}
	
			}
		}
		/*aggiorna il tempo*/
		gettimeofday(&end, NULL);
		elapsedTime = (double) (end.tv_usec - begin.tv_usec) / 1000000 + (double) (end.tv_sec - begin.tv_sec);
		totElapsedTime = (double) (end.tv_usec - start.tv_usec) / 1000000 + (double) (end.tv_sec - start.tv_sec);

		/*Ogni tot secondi i pazienti vengono visitati*/
		if ((int)elapsedTime >= WARD_INTERVAL) {

			printf("[REPARTO%d]Sono passati %d secondi, servo.\n\n",ward_num, (int)totElapsedTime);

			if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
			fprintf(fp, "[REPARTO%d]Sono passati %d secondi, servo.\n\n",ward_num, (int)totElapsedTime);
			fclose(fp);

			if (head != NULL) {	


					/*aggiorna i tempi di attsa, calcola il successivo e invialo alla prestazione*/
					refreshAges(&head, totElapsedTime);

					/*invia il paziente corretto*/
					msgp.mtype = 1;
					msgp.patient = calcNext(&head);
					if (msgsnd(s_msgq_id, &msgp, sizeof(patient_t), 0) == -1) errExit("[ERRORE]msgsnd 2");
					deletePatient(&head, msgp.patient.ID);

			}else {
			printf("[REPARTO%d]Nessuno da servire!\n\n",ward_num);
			
			if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
			fprintf(fp, "[REPARTO%d]Nessuno da servire!\n\n",ward_num);
			fclose(fp);}	

			recivedTmpStop = 0;
			if (releaseSem(mutex, 0) == -1) errExit("[ERRORE]releaseSem 2");
			gettimeofday(&begin, NULL);
		}
	}	
	exit(EXIT_SUCCESS);
}				