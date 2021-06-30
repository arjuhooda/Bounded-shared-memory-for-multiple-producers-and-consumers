#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define BUFF_SIZE 5

union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
};

int main(void)
{
	key_t sem_key, shared_key;
	union semun arg;
	int semid, shmid;
	
	// For deleting semphore array
	if ((sem_key = ftok("producer.c", 'J')) == -1) 
	{
		perror("ftok");
		exit(1);
	}
	/* grab the semaphore set created by producer.c or consumer.c */
	if ((semid = semget(sem_key, 3, 0)) == -1) 
	{
		perror("semget");
		exit(1);
	}
	/* remove it */
	if (semctl(semid, 0, IPC_RMID, arg) == -1) {
		perror("semctl");
		exit(1);
	}
	printf("The semaphore array is successfully deleted !!\n");
	// For deleting shared memory segment
	if((shared_key = ftok("producer.c", 'A')) == -1) 
	{
		perror("shared memory ftok");
		exit(1);
	}
	/* grab the shared memory segment created by producer.c or consumer.c */
	if ((shmid = shmget(shared_key, BUFF_SIZE+2, 0644 | 0)) == -1) 
	{
		perror("shmget");
		exit(1);
	}
	/* remove it */
	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
		perror("semctl");
		exit(1);
	}
	printf("The shared memory segment is successsfully deleted !!\n");
	return 0;
}
