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

/*---------------------------------------------------------------+
|Legge un messaggio, inviato dal reparto, ed eroga la prestazione|
+----------------------------------------------------------------*/

int main(int argc, char const *argv[])
{
	int s_msgq_id, ward_num;
	struct mymsg msgp;

	FILE *fp;
	if (!(fp = fopen(LOGPATH, "a+"))) errExit("[ERRORE]fopen");

	ward_num = atoi(argv[1]); /*Numero del reparto*/
	

	printf("---------------Iniziata erogazione prestazione%d------------\n\n", ward_num);
	fprintf(fp, "---------------Iniziata erogazione prestazione%d------------\n\n", ward_num);
	
	/*Asssocio alla corretta coda di messaggi*/
	while (1){
		s_msgq_id = msgget(S_START_MSGQ_KEY + ward_num, 0666);
		if (s_msgq_id == -1) errExit("[ERRORE]msgget 5");

		/*Leggo dalla coda di messaggi ed erogo la prestazione*/
		if (msgrcv(s_msgq_id, &msgp, sizeof(patient_t), 0, 0) == -1)  errExit("[ERRORE]msgrcv 2");
		printf("[PRESTAZIONE %d]Paziente [%d] sto per essere servito.\n\n",ward_num, msgp.patient.ID);
		fprintf(fp, "[PRESTAZIONE %d]Paziente [%d] sto per essere servito.\n\n",ward_num, msgp.patient.ID);
		giveService(msgp.patient);
		if (kill (msgp.patient.ID, SIGUSR1) == -1) errExit("[ERRORE]kill");
	}
	fclose(fp);
	exit(EXIT_SUCCESS);
}