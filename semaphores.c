#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/sem.h>

#define SIZE 16

/********************************************************************
 * This program demonstrates the use of semaphores by using shared
 * memory and critical section collision avoidence.
 *
 * @authors Cade Baker, Louis Sullivan
 * @version 1.0 (10/11/2018)
 *
 *******************************************************************/

int main (int argc, char **argv)
{
    int status;
    long int i, loop, temp, *shmPtr;
    int shmId, semId;
    pid_t pid;

    //Structure of wait and signal
    struct sembuf sbuf;

	//setup the sembuf
	sbuf.sem_num = 0;
	sbuf.sem_op = 0;
	sbuf.sem_flg = 0;
	
    //check for a second command line arg
    if(argc < 2){
        printf("Invalid argument. Please supply 2 args.\n");
        exit(0);
    }
    // get value of loop variable (from command-line argument)
    loop = atol(argv[1]);
    printf("Loop variable: %ld\n", loop);

	//get shared memory
    if ((shmId = shmget (IPC_PRIVATE, SIZE, IPC_CREAT|S_IRUSR|S_IWUSR)) 
			< 0) {
        perror ("i can't get no..\n");
        exit (1);
    }
	//attach shared mmeory topointer
    if ((shmPtr = shmat (shmId, 0, 0)) == (void*) -1) {
        perror ("can't attach\n");
        exit (1);
    }

    shmPtr[0] = 0;
    shmPtr[1] = 1;

    /*  create a new semaphore set for use by this (and other) processes.. 
     */ 
    semId = semget (IPC_PRIVATE, 1, 00600);

    /*  initialize the semaphore set referenced by the previously obtained
	 * semId handle. 
     */ 
    semctl (semId, 0, SETVAL, 1);



    if (!(pid = fork())) {
        //setup sbuf for writing
		sbuf.sem_op = -1;
        // signal semaphore lock
		if(semop (semId, &sbuf, 1) == -1)        
			printf("Error running semop lock on child.\n");
	
		//identify pid
		printf("pid: %d\n", pid);

        for (i=0; i<loop; i++) 
        {
            // swap the contents of shmPtr[0] and shmPtr[1]
            temp = shmPtr[0];
            shmPtr[0] = shmPtr[1];
            shmPtr[1] = temp;

     		// output the current value of the shared memory in the child
            printf ("CHILD values:  %li \t %li\n", shmPtr[0], shmPtr[1]);
        }
        printf("\n");

		//set sbuf to release control
		sbuf.sem_op = 1;
        // signal semaphore unlock
		if(semop (semId, &sbuf, 1) == -1)        
			printf("Error running semop release on child.\n");

		//detatch share memory pointer
        if (shmdt (shmPtr) < 0) {
            perror ("just can't let go\n");
            exit (1);
        }
      	exit(0);
      
    }
    else{
		//setup sbuf for writing
		sbuf.sem_op = -1;
        // signal semaphore lock
		if(semop (semId, &sbuf, 1) == -1)        
			printf("Error running semop lock on parent.\n");

		//identify pid
        printf("pid:%d\n", pid);

        for (i=0; i<loop; i++) {
            // swap the contents of shmPtr[1] and shmPtr[0]
            temp = shmPtr[1];
            shmPtr[1] = shmPtr[0];
            shmPtr[0] = temp;
            printf ("PARENT values: %li \t %li\n", shmPtr[0], shmPtr[1]);
        }
        printf("\n");

		//set sbuf to release control
		sbuf.sem_op = 1;
        // signal semaphore unlock
		if(semop (semId, &sbuf, 1) == -1)        
			printf("Error running semop lock on parent.\n");

        wait (&status);
		
        //printf ("values: %li\t%li\n", shmPtr[0], shmPtr[1]);

	}

	//remove the smeaphore
	if (semctl(semId, 0, IPC_RMID) < 0){
	   perror ("can't deallocate semaphore");
	   exit(1);
        }
        
        // detach the shared memory segment from the pointer 
        if (shmdt (shmPtr) < 0) {
        	perror ("just can't let go\n");
        	exit (1);
        }

        //  remove the shared memory
        if (shmctl (shmId, IPC_RMID, 0) < 0) {
            perror ("can't deallocate\n");
            exit(1);
        } 


   return 0;
}
