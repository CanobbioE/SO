# Operating Systems


A simulation of system's resource management by spawning and managing concurrent processes through IPC.

[Project requirements](https://github.com/CanobbioE/UNITO/blob/master/SO/progetto_finale15-16.pdf "In Italian")  
[Project Schema](https://github.com/CanobbioE/UNITO/blob/master/SO/schema_progetto.pdf "In Italian")

### Project info

Project name: HospitalOS.

The project can be compiled using the makefile:

```bash
$ make
```

Once executables are created the project can be launched by executing just the "main" file.

```bash
$ ./main
```
The "main" will start all the other executables.
Even if it is possible to launch single executables, they will stop since the resources they need (semaphores, shared memory, pipes...) are created and deleted by "main".
Every execution of the project creates a "log.txt" file for easy debugging.
The "tmp" folder is used for the creation of *FIFO*s.
Sending a SIGQUIT signal will prompt the user for confirmation, after that the execution will safely stops.

Inside "my_types.h" it is possible to change the value of:

- MAX_ACCESS: maximum number of process spwaned (default 10).
- MAX_WARDS: maximum number of times "reparto" get executed (default 2).
- TIMER: time (in seconds) before SIGALRM (default 20).
- TRESHOLD: maximum waiting time for a process (default 9).
- WARD_INTERVAL: time (in seconds) in-between processes get "treated" (default 3).
- LOGPATH: path to the logs file (default "log.txt")

Functions are fully documented inside "my_types.h".


### Project description:

Processes represents patients, the project is a simulation of an Hospital OS in  which patients gets treated (i.e. waits random time). A patient is a struct with ID equals to the process id, and symptom chosen randomly.

The project is divided into four parts: *"main", "triage", "reparto", "prestazione"*.

**Main:** creates all the resources used. Then processes get spawned untill the SIGALARM is recived. After the SIGALARM, *"main"* waits for all the patients (i.e. processes) to terminate. Finally the SIGQUIT from the user is waited to safely end the application. Each patient, after being created hat to pass a semaphore check then it get sent through a message queue to *"triage"*.

**Triage:** a random symptom and severity level get assigned to each patient. Based on the severity level each patient is sent to a different *"reparto"* (i.e. a different FIFO).

**Reparto:** Each patient gets into a linked list and waits to get treated. Each WARD_INTERVAL seconds a new patient get chosen from the list in the following order:

- Whoever is waiting for too long (avoid starvation).
- Whoever has maximum severity level.
- When both previous criterias have no match the first patient arrived is the first visited.

Treating a patient means writing it into a message queue read by the right *"prestazione"*.

**Prestazione:** the patient waits a random number of seconds, then a custom signal is sent to *"main"* which kills the patient process.

---

# Sistemi operativi
Progetto HospitalOS, Laboratorio SO.

Il progetto può essere compilato utilizzando il makefile.

$make

Una volta creati gli eseguibili è possibile avviare il progetto avviando solo "main".

$./main

L'avvio degli altri eseguibili è effettuato da "main", tuttavia è possibile avviarli singolarmente, tuttavia si noti che le risorse (semafori, code, pipe e fifo) sono create e rimosse da "main".

Il progetto crea un file "log.txt" in cui salva tutti gli output che compaiono a video. Tuttavia ci si aspetta che il file "log.txt" venga rimosso prima di avviare il progetto una seconda volta.

La cartella "tmp" è utilizzata per la creazione delle FIFO.

Il sistema può essere terminato con SIGQUIT. (dopo la conferma a video)

Tramite il file "my_types.h" è possibile modificare i valori di default di:

-Numero di accessi contemporanei. -Numero di reparti. -Tempo prima del SIGALRM. -Path del file con i sintomi. -Tempo massimo di attesa dei pazienti. -Intervalo di tempo dei reparti. -Path del file dei log.

Tutte le funzioni sono descritte nell'header "my_types.h". ############################################################################################

DESCRIZIONE PROGETTO:

Il progetto si divide in quattro moduli: "main", "triage", "reparto", "prestazione".

In "main" vengono create le risorse utilizzate da tutti i moduli. Successivamente fintanto che non si riceve il SIGALARM vengono creati processi (ovvero pazienti), che, dopo un controllo sul semaforo che regola gli accessi, vengono inseriti in una coda di messaggi che comunica a "triage" i dati del paziente (per chiarezza implementativa vengono utilizzate variabili del tipo patient_t, definito in "my_types.h").

Una volta in "triage" ai pazienti viene assegnato un sintomo (letto da file) e una gravità. A seconda della gravità del sintomo i pazienti vengono divisi nei reparti (ovvero viene scelta la FIFO su cui scrivere i dati del paziente).

I vari "reparti" leggono dalla FIFO corretta ("reparto1" leggerà da "myfifo1" e così via). Una volta che il paziente è nel reparto corretto viene inserito in una lista linkata insieme agli altri pazienti che arrivano al reparto e attende che sia passato l'intervallo di tempo specificato in "my_types.h". Una volta passato l'intervallo di tempo specificato viene visitato un paziente secondo i criteri:

-Chi attende da troppo tempo ha la precedenza (evitare starvation) -Chi ha gravità maggiore ha la precedenza -A parità di gravità si va in ordine di arrivo

Per evitare che il reparto si fermi sulla read una volta finiti i pazienti vengono usati due pazienti "speciali":

-TMP_STOP (ferma la read fino alla prossima visita) -EOS_PATIENT (ferma la read permanentemente)

I pazienti per essere visitati vengono inseriti su una coda di messaggi che dai vari "reparto" comunica alle adeguate "prestazione" i dati del paziente. Una volta in "prestazione" il paziente attende un tempo casuale compreso tra zero e la sua gravità.

Dopo la visita viene mandato un segnale a "main" che riprende l'esecuzione del processo che ha creato il paziente e libera il semaforo che regola gli accessi e infine termina.

Una volta ricevuto SIGALRM "main" attende la terminazione di tutti i processi-pazienti. Quando anche l'ultimo dei pazienti è uscito il "main" resta in attesa del SIGQUIT (inviato da tastiera dall'utente).

Ricevuto il SIGQUIT "main" procede alla rimozione delle risorse create.
