/*************************************************************************************
 Studente: Pastore Luca
 Matricola: N97000431

 Anno Accademico: 2023/24
 Corso: Parallel And Distributed Computing
 Prof.: Laccetti Giuliano, Mele Valeria
 Corso di Laurea Magistrale in Informatica
 Università degli studi Federico II - Napoli

 Traccia:
	Sviluppare un algoritmo per il calcolo della somma di N numeri reali, 
	in ambiente di calcolo parallelo su architectura MIMD a memoria distribuita,
	che utilizzi la libreria MPI
**************************************************************************************/

//SEZIONE DICHIARAZIONE HEADER FILE
#include <stdio.h>
#include <stdlib.h> 
#include <time.h>
#include <math.h>

#include "mpi.h"

/*
INPUT DA TERMINALE
l'input è costituito da due valori passati via terminale
	- argv[1] = numero di elementi
	- argv[2] = modalità scelta
N.B.: argv[0] contiene il nome del programma
	- argc = contiene il numero di parametri di input passati da terminale
*/

//SEZIONE DICHIARAZIONE PROTOTIPI FUNZIONI
int verifyInputParameter(int argc, char *argv[]);
double *generateRandomArray(int n); 
double *inputValuesArray(int argc, char **argv);
void printDoubleArray(double *arr, int n);
int isPowerOftwo(int x);
void firstComunicationStrategy(int processorID, int numberOfProcessor, double *sum, MPI_Status *status);
void secondComunicationStrategy(int processorID, int numberOfProcessor, double *sum, MPI_Status *status);
void thirdComunicationStrategy(int processorID, int numberOfProcessor, double *sum, MPI_Status *status);

//INIZIO MAIN PROGRAMM
int main(int argc, char *argv[]){
	//DICHIARAZIONE VARIABILI LOCALI AL MAIN PROGRAM
	MPI_Status status; 

	int menum, //identificatore processo
		nproc, //numero di processi 
		numelem, //numero di elementi da salvare
	 	nloc, //numero di elementi dell'array da considerare per il processore attuale
		rest, //resto della divisione numelem/nproc da  distribuire tra i vari processori se diverso da 0
		comunicationSrtategy, //valore compreso tra 1 e 3 che indica la strategia da utilizzare per il calcolo distribuito della somma
		tag, tmp, startPosition, i;

	double  *arr, //array contenente i valori da sommare
		    sum, //variabile che conterrà il risultato della somma
		    *xloc,
            t0, t1, var_time, timetot; //variabili utilizzate per il computo dei tempi di esecuzione del calcolo su macchina parallela

    if(verifyInputParameter(argc, argv) == 0){ //verifica della correttezza dei parametri di input passati da terminale
        perror("\n ATTENZIONE: Il valore fornito come input non e' un valore valido\n\n");
	    return 0; //parametri non corretti, uscita dal programma
	} 

    //inizializzazione funzioni MPI
	MPI_Init(&argc, &argv); //inizializzazione ambiente MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &menum); //salvo identificativo processo
	MPI_Comm_size(MPI_COMM_WORLD, &nproc); //salvo numero di processi totale
	
	if(menum==0){
		//INIZIO CODICE ESEGUITO DA PROCESSORE CON IDENTIFICATIVO 0		
	    numelem = atoi(argv[1]); //salvataggio numero di elementi sa sommare
        comunicationSrtategy = atoi(argv[2]); //salvataggio della strategia scelta

		//creazione array di elementi interin da sommare
	    arr = (numelem > 20) ? generateRandomArray(numelem) : inputValuesArray(argc, argv);

		//printDoubleArray(arr, numelem); //stampa dell'array generato
		//FINE CODICE ESEGUITO DA PROCESSORE CON IDENTIFICATIVO 0
	}

    /********** inizio distribuzione della strategia scelta ***********/
 	//la strategia da utilizzare per comunicare il risultato delle somme parziali viene inviato a tutti i processi
	MPI_Bcast(&comunicationSrtategy, 1, MPI_INT, 0, MPI_COMM_WORLD);

	/********** fine distribuzione della strategia scelta ***********/



	/********** inizio distribuzione del numero di elementi ***********/
	//il numero di elementi da sommare viene inviato a tutti i processi
	MPI_Bcast(&numelem, 1, MPI_INT, 0, MPI_COMM_WORLD); 

	/********** fine distribuzione del numero di elementi ***********/



	/********** inizio distribuzione degli elementi da sommare (array arr) ***********/
	nloc = numelem/nproc; //calcolo il numero di elementi da sommare per il processo attuale
	rest = numelem%nproc; //calcolo il numero di elementi restanti dall'operazione numelem/nproc che devono essere distribuiti agli altri porocessi

	if(menum < rest) nloc = nloc + 1; //distribuzione del resto

	xloc = (double *)malloc(nloc * sizeof(double));

	if(menum == 0){ 
		//INIZIO CODICE ESEGUITO DA PROCESSORE CON IDENTIFICATIVO 0
		xloc = arr; //quest'assegnazione permette di evitare di copiare gli elementi nella variabile xloc, 
					//dato che il processore con id=0 è gia in possesso dell'intero array di valori da sommare
		tmp = nloc; //permette di non modificare il valore della variabile nloc 
		startPosition = 0; //variabile utilizzata per indicare la posizione di partenza per la distribuzione degli elementi ad ogni processo

		//distribuzione degli elemnti dell'array
		for(i=1;i<nproc;i++){
			startPosition = startPosition + tmp; //elemento di partenza dell'array
			tag = 10 + i;
			if(rest == i) tmp = tmp-1;
			MPI_Send(&arr[startPosition], tmp, MPI_DOUBLE, i, tag, MPI_COMM_WORLD); //invio degli elemnti ai vari processori
		}
		//FINE CODICE ESEGUITO DA PROCESSORE CON IDENTIFICATIVO 0
	}else{
		//INIZIO CODICE ESEGUITO DA PROCESSORE CON IDENTIFICATIVO DIVERSO DA 0
		tag = 10 + menum;
		MPI_Recv(xloc, nloc, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &status); //ricezione dei dati inviati da processore con id=0
		//FINE CODICE ESEGUITO DA PROCESSORE CON IDENTIFICATIVO DIVERSO DA 0	
	}
	/********** fine distribuzione degli elementi da sommare ***********/




	/********* inizio delle somme parziali locali ad ogni processore **********/

	MPI_Barrier(MPI_COMM_WORLD); //sincronizzo i processori in modo che ciascuno inizi la somma nello stesso momento
    t0 = MPI_Wtime();

	sum = 0; //inizializzazione a 0 della varibile che conterrà la somma parziale svolta dal processore "i"
	for(i=0;i<nloc;i++) sum = sum + xloc[i]; //calcolo della somma parziale

	//printf("\n Sono il processo %d, la mia somma parziale è %.2lf\n\n", menum, sum);

	/********* fine delle somme parziali **********/

	

	/********* Inizio Strategia di comunicazione somme parziali *************/
	if(comunicationSrtategy == 1 || isPowerOftwo(nproc)==0){
        if(isPowerOftwo(nproc)==0)  perror("ATTENZIONE: Non e' stato possibile applicare la strategia 2/3 pertanto e' stata esguita la strategia 1");
        firstComunicationStrategy(menum, nproc, &sum, &status);
    }else if(comunicationSrtategy == 2)  
        secondComunicationStrategy(menum, nproc, &sum, &status);
	else    
        thirdComunicationStrategy(menum, nproc, &sum, &status);
	/********* fine Strategia di comunicazione somme parziali  *************/

    t1 = MPI_Wtime(); //tempo finale dopo le operazioni di somma
    var_time = t1 - t0; //tempo impiegato da ogni processorID
    MPI_Reduce(&var_time, &timetot, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD); //operazione che permette di memorizzare nella variabile timeout del processo 0 il massimo tra i tempi di tutti i processori
    if(menum == 0)  printf("Il tempo totale e': %lf secondi\n", timetot);

	//comunicazione risultato finale
	if(menum == 0)  printf("\n\n Sono il processo %d e la somma totale è = %.2lf\n\n", menum, sum);

	//terminazione MPI
	MPI_Finalize();

	return 0;
} //FINE MAIN PROGRAMM

//SEZIONE DEFINIZIONE FUNZIONI E PROCEDURE

/*
 funzione utilizzata per verificare la correttezza dei parametri passati in input
 output: 1 se il numero di valori in input è almeno 2, di cui il secondo compreso nel dominio [1, 2, 3]
*/
int verifyInputParameter(int argc, char *argv[]){
	return (argc <= 1 || atoi(argv[2])<0 || atoi(argv[2])>3) ? 0 : 1;
}

/*
 funzione per acquisire in input i 20 valori da sommare e da aggiungere all'array
 - argc = numero di valori in input
 - argv = array conenente i valori passati da riga di comando, di tipo char
 - output: l'array di double appena generato
*/
double *inputValuesArray(int argc, char **argv){
	int i, numelem=argc-3;
    double *arr;

    arr = (double *)malloc(numelem*sizeof(double));
	for(i=0;i<numelem;i++)  arr[i] = (double)atoi(argv[i+3]);

    return arr;
}

/*
 funzione utilizzata per generare in modo casuale i valori da sommare per l'array di reali
 - n = numero di elementi dell'array
 - output: l'array di double appena generato
*/
double *generateRandomArray(int n){
	srand(time(NULL));

	int i; double *arr;

    arr = (double *)malloc(n*sizeof(double));
	for(i=0;i<n;i++) arr[i] = rand()%1000-1;

    return arr;
}

/*
 funzione utilizzata per stampare a video i valori di un array di interi
 - arr = array di interi contenente i valori
 - n = numero di elementi dell'array
*/
void printDoubleArray(double *arr, int n){
	int i;
	printf("\n");
	for(i=0;i<n;i++) printf("%.2lf\t", arr[i]);
	printf("\n\n");
}

/*
 funzione utilizzata per verificare se un numero è una potenza di 2
 x: valore da verificare
 output: 1 se x è potenza di due, 0 altrimenti
*/
int isPowerOftwo(int x){
	return ((x != 0) && ((x & (x-1)) == 0)) ? 1 : 0;
}

/*
 Funzione utilizzata per implementare la comunicazione tra processori secondo la strategia I, in modo da poter effettuare la somma totale degli elementi.
 La strategia I prevede che ogni processore comunica la propria somma parziale al processore di ID=0, Quest'ultimo si occuperà di computare la somma totale.
 processorID: indica l'id del processore che richiama la funzione
 numberOfProcessor: indica il numero totale di processori
 sum: variabile passata per indirizzo, contiene il valore della somma totale
 status: informazioni MPI su ricezione messaggi
*/
void firstComunicationStrategy(int processorID, int numberOfProcessor, double *sum, MPI_Status *status){
	int i, tag;
	double partialSum;

	if(processorID == 0){ 
		//inizio computazione per processore con id = 0
		for(i=1;i<numberOfProcessor;i++){
			tag = 80+i;
			//il processore 0 riceve le somme parziali da tutti gli altri processori
			MPI_Recv(&partialSum, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD, status); 
            *sum = *sum + partialSum;//computo della somma 
		}
		//fine computazione per processore con id = 0
	}else{
		//inizio computazione per processori con id != 0
		tag = 80+processorID;
		MPI_Send(sum, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD); //invio della somma parziale al processore con id=0
		//fine computazione per processori con id != 0
	}
}

/*
 Funzione utilizzata per implementare la comunicazione tra processori secondo la strategia II, in modo da poter effettuare la somma totale degli elementi.
 La strategia II prevede che, ad ongi passo, coppie di processori comunicano contemporaneamente le proprie somme parziali. Per ogni coppia, un processore 
 invia all'altro la propria somma parziale. Quest'ultimo provvede al computo della somma. Il risultato è in un unico processore
 processorID: indica l'id del processore che richiama la funzione
 numberOfProcessor: indica il numero totale di processori
 sum: variabile passata per indirizzo, contiene il valore della somma totale
 status: informazioni MPI su ricezione messaggi
*/
void secondComunicationStrategy(int processorID, int numberOfProcessor, double *sum, MPI_Status *status){
	int i, tag, log_nproc, powi, powi_succ;
	double partialSum;

	log_nproc = (int)log2(numberOfProcessor);

	for(i=0;i<log_nproc;i++){ //passi di comunicazione

		powi = pow(2, i);

		if(processorID % powi == 0){ //chi partecipa alla comunicazione

			powi_succ = pow(2, i+1);

			if(processorID % powi_succ == 0){ //chi riceve
				tag = 88+processorID;
				MPI_Recv(&partialSum, 1, MPI_DOUBLE, processorID+powi, tag, MPI_COMM_WORLD, status); //riceve da processore processorID + 2^i
                
                *sum = *sum + partialSum; //computo della somma
			}else{
                tag = 88+processorID-powi;
				MPI_Send(sum, 1, MPI_DOUBLE, processorID-powi, tag, MPI_COMM_WORLD); //invia da processore processorID - 2^i
			}
		}
	}
}


/*
 Funzione utilizzata per implementare la comunicazione tra processori secondo la strategia III, in modo da poter effettuare la somma totale degli elementi.
 La strategia III prevede che, ad ongi passo, coppie di processori comunicano contemporaneamente le proprie somme parziali. Per ogni coppia, entrambi i 
 processore inviano la propria somma parziale. Il risultato è in entrambi i processori.
 processorID: indica l'id del processore che richiama la funzione
 numberOfProcessor: indica il numero totale di processori
 sum: variabile passata per indirizzo, contiene il valore della somma totale
 status: informazioni MPI su ricezione messaggi
*/
void thirdComunicationStrategy(int processorID, int numberOfProcessor, double *sum, MPI_Status *status){
	int i, tag, log_nproc, powi, powi_succ;
	double partialSum;

	log_nproc = (int)log2(numberOfProcessor);
	for(i=0;i<log_nproc;i++){ //passi di comunicazione

		powi = pow(2, i);
		powi_succ = pow(2, i+1);

		if((processorID % powi_succ) < powi){ //si decide solo a quale processore inviare e da quale processore ricevere
			tag = 88+processorID;	
            MPI_Recv(&partialSum, 1, MPI_DOUBLE, processorID+powi, tag, MPI_COMM_WORLD, status); //riceve da processore processorID + 2^i
            MPI_Send(sum, 1, MPI_DOUBLE, processorID+powi, tag, MPI_COMM_WORLD); //invia da processore processorID + 2^i
					      
            *sum = *sum + partialSum; //computo della somma
		}else{
            tag = 88+processorID-powi;
            MPI_Send(sum, 1, MPI_DOUBLE, processorID-powi, tag, MPI_COMM_WORLD); //invia da processore processorID - 2^i
            MPI_Recv(&partialSum, 1, MPI_DOUBLE, processorID-powi, tag, MPI_COMM_WORLD, status); //riceve da processore processorID - 2^i	     
                     		       
            *sum = *sum + partialSum; //computo della somma
		}
	}
}
