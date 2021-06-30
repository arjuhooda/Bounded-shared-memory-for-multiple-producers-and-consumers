#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define BUFF_SIZE 5

union semun {
	int val;		// used for SETVAL 
	struct semid_ds *buf;	// used for IPC_STAT and IPC_SET 
	ushort *array;		// used for GETALL and SETALL
};


int init(key_t key, char* data)  // to initialize semaphores
{
	int i;
	union semun argument;
	struct semid_ds buffer; 	
	struct sembuf semaphores; 					// declare semaphore structure
	int semaphore_id; 						// declare variable for sem_id
	semaphore_id = semget(key, 3, IPC_CREAT | IPC_EXCL | 0666); 	//  to get semaphore id
	
	if (semaphore_id >= 0) 	// if it got the semaphore first 
	{    
		argument.val = 1;
		semaphores.sem_flg = 0;
		
		semaphores.sem_num = 0;			// mutex semaphore
		semaphores.sem_op = 1;			// set mutex to 1
		semop(semaphore_id, &semaphores, 1);	// to perform set operation
		
		semaphores.sem_num = 1;			// empty semaphore
		semaphores.sem_op = BUFF_SIZE; 		// set empty to BUFF_SIZE
		semop(semaphore_id, &semaphores, 1);	// to perform set operation
		
		semaphores.sem_num = 2;			// full semaphore
		semaphores.sem_op = 0;			// set full to 0
		semop(semaphore_id, &semaphores, 1);	// to perform set operation
		
		data[BUFF_SIZE] = '0';			// front 
		data[BUFF_SIZE+1] = '0';		// rear
		
	} 
	else if (errno == EEXIST) 	// if someone else got semaphores first
	{ 
		int ready = 0;
		semaphore_id = semget(key, 3, 0);		//  to get semaphore id
		if (semaphore_id < 0) return semaphore_id; 	// for handling error
		
		// wait for other process to initialize the semaphores  
		argument.buf = &buffer;
		for(i = 0; i < 10 && !ready; i++) 
		{
			semctl(semaphore_id, 2, IPC_STAT, argument);
			if (argument.buf->sem_otime != 0) 
			{
				ready = 1;	// set ready to 1
			} 
			else 
			{
				sleep(1);	
			}
		}
		if (!ready) 		// if ready is zero
		{
			errno = ETIME;	// set errno to ETIME 
			return -1;
		}
	} 
	else 
	{
		return semaphore_id; 	// for handling error
	}
	return semaphore_id;
}

int main()
{
		key_t semaphore_key, shared_key;
		int semaphore_id, shared_id, i;
		char *pointer, prod;
		struct sembuf semaphores;
		semaphores.sem_flg = SEM_UNDO;
		if((semaphore_key = ftok("producer.c", 'J')) == -1) 
		{
			perror("semaphore ftok");
			exit(1);
		}
		if((shared_key = ftok("producer.c", 'A')) == -1) 
		{
			perror("shared memory ftok");
			exit(1);
		}
		if ((shared_id = shmget(shared_key, BUFF_SIZE+2, 0644 | IPC_CREAT)) == -1) 
		{
			perror("shmget");
			exit(1);
		}
		// attach to the segment to get a pointer to it
		pointer = (char*)shmat(shared_id, (void *)0, 0);
		if (pointer == (char *)(-1)) 
		{
			perror("shmat");
			exit(1);
		}
		if ((semaphore_id = init(semaphore_key, pointer)) == -1) 
		{
			perror("init function");
			exit(1);
		}
		
		while(1)
		{
			printf("\nTrying to produce...\n");
			
			semaphores.sem_num = 1;			// empty semaphore
			semaphores.sem_op = -1; 		// decrement of empty sem by 1
			semop(semaphore_id, &semaphores, 1);	// wait(empty)
			semaphores.sem_num = 0;			// mutex
			semaphores.sem_op = -1;			// decrement of mutex by 1
			semop(semaphore_id, &semaphores, 1);	// wait(mutex)
			
			// producer
			printf("Enter a character to produce: ");
			scanf(" %c", &prod);
			if (prod == '0')
				break;
			i = pointer[BUFF_SIZE]-'0';
			pointer[i] = prod;
			printf("Producer is producing a character %c at %d\n", pointer[i], i);
			i = (i+1)%BUFF_SIZE;
			pointer[BUFF_SIZE] = '0'+i;
			printf("Produced\n");
			
			semaphores.sem_num = 0;			// mutex
			semaphores.sem_op = 1;			// increment of mutex by 1
			semop(semaphore_id, &semaphores, 1); 	// signal(mutex)
			semaphores.sem_num = 2;			// full semaphore
			semaphores.sem_op = 1;			// increment of full sem by 1
			semop(semaphore_id, &semaphores, 1);	// signal(full)
			
		}
		// detach from the segment
		if (shmdt(pointer) == -1)
		{
			perror("shmdt");
			exit(1);
		}
		return 0;
}
