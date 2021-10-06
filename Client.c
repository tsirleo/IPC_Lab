#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <assert.h>

#define MAXLEN 256


/*-------------------------------------Global variables--------------------------------------*/
key_t key1, key2;
FILE *fp;
int Clients, id, semid_id, semid_data, shmid_id, shmid_data, *shmaddr_id;
struct sembuf sops;
char *shmaddr_data, data[MAXLEN];


/*---------------------------------Error checking functions----------------------------------*/

void File_opening_error(FILE *fp)
{
    if (fp==NULL)
    {
        fprintf(stderr, "Error: File opening failure \n");
        fclose(fp);
        exit(1);
    }
}

void Key_creation_error(key_t key)
{
    if (key==-1)
    {
        fprintf(stderr, "Error: Failed to create key \n");
        exit(1);
    }
}

void Sem_connection_error(int semid)
{
    if (semid==-1)
    {
        fprintf(stderr, "Error: Failed to connect semaphore \n");
        exit(1);
    }	
}

void Shm_connection_error(int shmid)
{
    if (shmid==-1)
    {
        fprintf(stderr, "Error: Failed to connect shared memory \n");
        exit(1);
    }		
}


/*--------------------------------------Sistem tools-----------------------------------------*/

void IPC_for_getting_id() //Connecting to IPC tools to get an individual number from the server
{
	key1=ftok("/dev/null", 'q');
	Key_creation_error(key1);
	semid_id=semget(key1, 1, 0);
	//printf("Semid = %d \n", semid_id);
	Sem_connection_error(semid_id);
	shmid_id=shmget(key1, 2*sizeof(int), 0);
	//printf("Shmid = %d \n", shmid_id);
	Shm_connection_error(shmid_id);
	shmaddr_id=shmat(shmid_id, NULL, 0);
}

void IPC_for_data_receive() //Connecting to IPC tools for receive data from the server
{
	key2=ftok("/dev/mem", 'x');
	Key_creation_error(key2);
	semid_data=semget(key2, Clients, 0);
	//printf("Semid = %d \n", semid_data);
	Sem_connection_error(semid_data);
	shmid_data=shmget(key2, MAXLEN*Clients, 0);
	//printf("Shmid = %d \n", shmid_data);
	Shm_connection_error(shmid_data);
	shmaddr_data=shmat(shmid_data, NULL, 0);	
}


/*----------------------------------------Functions------------------------------------------*/

void Get_id() //Getting individual number
{
	sops.sem_num=0;
	sops.sem_flg=0;
	sops.sem_op=-1;
	semop(semid_id, &sops, 1);
	id=shmaddr_id[0];
	Clients=shmaddr_id[1];
	sops.sem_op=-1;
	semop(semid_id, &sops,1);
	shmdt(shmaddr_id); //Disconnecting from shared memory
}

void Receive_data()
{
	FILE *fp;	
	while (1)
	{
		sops.sem_num=id;
		sops.sem_flg=0;
		sops.sem_op=-1;
		if (semop(semid_data, &sops, 1)!=-1)
		{
			fp=fopen("output", "a");
			File_opening_error(fp);
			fprintf(fp, "%d %s", id, shmaddr_data+(MAXLEN*id));
			fclose(fp);
			sops.sem_op=-1;
			semop(semid_data, &sops, 1);
		}
		else
		{
			shmdt(shmaddr_data); //Disconnecting from shared memory
			break;
		}
	}
}


/*------------------------------------The main function--------------------------------------*/

int main(int argc, char **argv)
{
	IPC_for_getting_id();
	Get_id();
	IPC_for_data_receive();
	Receive_data();
	return 0;
}

