/* 
* 	
*	Gustavo J. Alfonso Ruiz
*		
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>

#include <unistd.h>

#include <semaphore.h>
#include <pthread.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "Ex2.h"

/**************************************************/

static bool is_finished = false;

/**************************************************/

static void split_file_data(char* line, char token, file_data_t* data);
void err_sys(const char* text);

/**************************************************/
/* Estructura de nuestra primera memoria compartida. */
struct shmseg1{
	int number;
	int process;
	char memory[100];
};

/* Estructura de nuestra segunda memoria compartida. */
struct shmseg2{
	int number;
	int process;
	char memory[100];
};

/**************************************************/

int main(int argc, char *argv[])
{
	int pid;					// Identificador de nuestro proceso.
	int countR = 0;				// Contador para el proceso de lectura.
	int countW;					// Contador para el proceso de escritura.
	
	sem_t* psem1;				// Variable de nuestro semaforo.

	FILE *fp;					// File para nuestra funcion de lectura.
	FILE *pf;					// File para nuestra funcion de escritura
		
	int bytes_read;				// Variable que utilizaremos para comprobar si una línea esta vacía.
    char * line = NULL;    		// Variable que utilizaremos para comprobar si una línea esta vacía.
    size_t len;					// Variable que utilizaremos para comprobar si una línea esta vacía.
    
    int shmid1;					// Identificador de nuestra primera memoria compartida
    int *shmp1;					// Variable que asignaremos a nuestra primera memoria compartida.
    
    int shmid2;					// Identificador de nuestra segunda memoria compartida.
    int *shmp2;					// Variable que asignaremos a nuestra segunda memoria compartida.
    
    /* Creamos nuestra memoria compartida. */
    shmid1=shmget(IPC_PRIVATE, sizeof(struct shmseg1), 0666|IPC_CREAT);
    if (shmid1 ==-1) err_sys("Shared MemoryError");
	
	/* Asignamos nuestra memoria hacia nuestra variable shmp1. */
    shmp1=(int *)shmat(shmid1,0,0);
    if (shmp1 ==(void*)(-1)) err_sys("SharedMemory attachment error");
    
    /* Creamos nuestra memoria compartida. */
    shmid2=shmget(IPC_PRIVATE, sizeof(struct shmseg2), 0666|IPC_CREAT);
    if (shmid2 ==-1) err_sys("Shared MemoryError");
	
	/* Asignamos nuestra memoria hacia nuestra variable shmp2. */
    shmp2=(int *)shmat(shmid2,0,0);
    if (shmp2 ==(void*)(-1)) err_sys("SharedMemory attachment error");

    
	/* Abrimos el archivo con el nombre introducido por el usuario. */  
    fp = fopen(argv[1], "r");

    if (fp == NULL)
    {
        perror("Unable to open the file.\n");
        exit(EXIT_FAILURE);
    }

	/* Creamos e inicializamos nuestro semaforo. */
	psem1 = (sem_t*)sem_open("/semaphore", O_CREAT, 0600, 0);
	if(psem1 == SEM_FAILED){
		err_sys("Open psem1");
	}
	
	/* Iniciamos nuestro proceso asignandolo a nuestra variable pid. */
	pid = fork();
	
	
	if(pid != 0)
	{
		/* 
		*	Proceso Padre. 
		*	Sera el encargado de leer el fichero introducido por el usuario.
		*/
     
	    /* Lee una linea del archivo. */
	    bytes_read = getline(&line, &len, fp);

	    /* ¿Hemos llegado al final del archivo? */
	    if(bytes_read == -1)
	    {
	        printf("End of file detected.\n");
	        is_finished = true;         
		}
	    
	    /* Loop hasta interrunpir el programa. */
	    while(!is_finished)
		{
			/* Iniciamos nuestra estructura. */	
			file_data_t file_data = {0,0,0,0};
	  		
	        /* Leemos una linea del archivo. */
	    	bytes_read = getline(&line, &len, fp);
	
	        /* ¿Hemos llegado al final del archivo? */
	        if(bytes_read == -1)
	    	{
	            printf("End of file detected.\n");
	            is_finished = true;
				break;         
	    	}
	        
			/* Separamos linea a linea la información del archivo y la movemos a nuestra nueva estructura. */    
	        split_file_data(line, ',', &file_data);
	
	        /* Como solo nos interesa la temperatura la copiamos a nuestra variable en la memoria compartida. */
	        shmp1[countR] = file_data.temperature;
	        
	        /* Imprime por pantalla el numero copiado. */
	        //printf("read temp: %d, suscesfully copied.\n", shmp1[countR]);
	        
	        countR++;
	    	    	    	
		}
		
		/*	
		*	Copiamos countR a nuestra segunda variable "shmp2[0]" en la memoria compartida.
		*	Esta variable se utilizara en el segundo proceso para saber la cantidad de lineas que tenemos que escribir.
		*/
		shmp2[0] = countR;
	    
	    /* Comprobamos por pantalla el número de lineas leidas. */
	    printf("Read %d lines.\n", countR);
	    
	    printf("Process 1 finished.\n");
		
		fclose(fp);			//Cerramos el fichero.
		
	    sem_post(psem1);	//Aumentamos nuestro semáforo.
	    
		exit(0);
		
	} else if(pid == 0) 
	{
		/* 
		*	Proceso Hijo.
		*	Este proceso será el encargado de escribir el nuevo archivo con los datos del proceso padre.
		*/
		sem_wait(psem1);	//Disminuimos nuestro semaforo.
		
		/* Creamos nuestro nuevo archivo. */
		pf = fopen("writeFile.txt", "w");
		
		if (pf == NULL)
		{
		    perror("Unable to write the file.\n");
	        exit(EXIT_FAILURE);
	    	
		}
		
		/*	
		*	Loop donde utilizaresmos las dos variables en la memoria compartida.
		*	shmp2[0] nos indicara el numero de veces que tenemos que ejecutar el loop y 
		*	shmp1[count] contiene la informacion leida del fichero.
		*/
		for(countW = 0; countW < shmp2[0]; countW++)
		{
			fprintf(pf, "%d \n", shmp1[countW]);
			
			/* Información por pantalla de lo que se esta copiando.*/
			//printf(" wr temp: %d, suscesfully copied.\n", shmp1[countW]);
					
		}
		
		/* Escribimos por pantalla la cantidad de lineas contadas. */
		printf("Write %d lines.\n", countW);
	
		printf("Process 2 finished.\n");
		
		fclose(pf);			//Cerramos el fichero.
		
		sem_post(psem1);	//Diminuimos el semaforo.
		
		exit(0);
		
	}else err_sys("Fork error.");
	
	/* Desconectamos la memoria compartida. */
    shmdt(shmp1);
    shmdt(shmp2);
    
	/* Eliminamos la memoria compartida. */
    shmctl(shmid1,IPC_RMID,NULL);
    shmctl(shmid2,IPC_RMID,NULL);
    
    /* Desconectamos el semaforo y lo cerramos. */
	sem_unlink ("/semaphore");   
    sem_close(psem1);
       
	/* Finalizamos el programa. */
    return(0);
                 
}

/**************************************************/

/*
*	Funcion para separar los datos del archivo y añadirlos a una nueva estructura. Es la misma funcion de la PR1 solo que con nombre distinto.
*/
static void split_file_data(char* line, char token, file_data_t* data)
{
	int16_t* data_ptr[4] = {&(data->temperature), &(data->altitude), &(data->velocity), &(data->power)};
	char* save_ptr = line;
	char* split = NULL;
	int16_t i = 0;
	
	//*  Miramos por el primer token y lo separamos de la linea. */
	split = strtok_r(save_ptr, &token, &save_ptr);
	if (split != NULL)
	{
	   * Convertimos el valor a integer y lo almacenamos en la posicion correspondiente. */
	   *data_ptr[i] = atoi(split);
	
	   i++;
	}
	
	/* Si quedan mas token, repetimos el proceso. */
	while (split != NULL)
	{
			    /* Miramos los token y los separamos de la linea. */
	    split = strtok_r(NULL, &token, &save_ptr);
	    if (split != NULL)
	    {
	      /* Convertimos el valor a integer y lo almacenamos en la posicion correspondiente. */
	      *data_ptr[i] = atoi(split);
	
	      i++;
	    }
	  }
    
}

/**************************************************/

void err_sys(const char* text)
{
	perror(text);
	exit(1);
}

/**************************************************/

