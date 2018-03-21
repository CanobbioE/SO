#include "my_types.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

/*--------------------------------------------------------------+
|Legge i messaggi sulla coda associata a t_msgq_id e associa ad	| 
|ogni paziente un sintomo e un codice di gravità. Scrive sulla	|
|FIFO del reparto corretto (sulla base della gravità) la  	 	|
|struttura paziente.							 				|
+---------------------------------------------------------------*/

int main(int argc, char const *argv[]) {
	
	int t_msgq_id, fd, a_fd[MAX_WARDS], n;
	struct mymsg msgp;
	patient_t patient;
	char myfifo[15];

	FILE *fp;
	

	printf("----------------------AVVIATO TRIAGE----------------------\n\n");

	if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
	fprintf(fp, "----------------------AVVIATO TRIAGE----------------------\n\n");
	fclose(fp);

	sleep(1);
	/*Associazione alla coda di messaggi*/
	t_msgq_id = msgget(TRIAGE_MSGQ_KEY, 0666);
	if (t_msgq_id == -1) errExit("[ERRORE]msgget 3");

	/*Apertura delle FIFO*/
	for (n = 1; n <= MAX_WARDS; n++ ) {
		sprintf(myfifo, "./tmp/myfifo%d", n);
		a_fd[n-1] = open(myfifo, O_WRONLY);

		if (a_fd[n-1] == -1) errExit("[ERRORE]open 1");
	}

	/*Leggo dalla coda di messaggi e scrivo sulle fifo*/
	while (1) {
		/*Ricezione del messaggio*/
		if (msgrcv(t_msgq_id, &msgp, sizeof(patient_t), 0, 0) == -1) errExit("[ERRORE]msgrcv 1");
		patient = msgp.patient;

		if (patient.ID != EOS_PATIENT && patient.ID != TMP_STOP) {
			printf("[TRIAGE]Paziente [%d] sono arrivato nel triage.\n\n",patient.ID);

			if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
			fprintf(fp, "[TRIAGE]Paziente [%d] sono arrivato nel triage.\n\n",patient.ID);
			fclose(fp);

			/*Assegnazione del sintomo e codice di gravità*/
			assignSymp(&patient);
			printf("[TRIAGE]Paziente [%d] il mio sintomo ha gravità: %d\n\n", patient.ID, patient.severity);

			if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");
			fprintf(fp, "[TRIAGE]Paziente [%d] il mio sintomo ha gravità: %d\n\n", patient.ID, patient.severity);
			fclose(fp);

			/*Scelgo, in base alla gravità del sintomo, su quale FIFO scrivere il paziente*/
			for (n = 1; n <= MAX_WARDS; ++n) {
				if (patient.severity <= n * (10/MAX_WARDS)) {
					fd = a_fd[n-1];
					break;
				}else{ 
					fd = a_fd[MAX_WARDS-1];
					break;
				}
			}
			/*Scrivo il paziente*/
			if (write(fd, &patient, sizeof(patient_t)) == -1) errExit("[ERRORE]write 1");
		}else if (patient.ID == TMP_STOP) {
			for (n = 1; n <= MAX_WARDS; ++n) {
				fd = a_fd[n-1];
				if (write(fd, &patient, sizeof(patient_t)) == -1)errExit("[ERRORE]write 2");
			}

		}else{
			for (n = 1; n <= MAX_WARDS; ++n) {
				fd = a_fd[n-1];
				if (write(fd, &patient, sizeof(patient_t)) == -1)errExit("[ERRORE]write 3");
			}
		}
	} 
	
	if (close(fd) == -1)errExit("[ERRORE]close 1");
	exit(EXIT_SUCCESS);
}
