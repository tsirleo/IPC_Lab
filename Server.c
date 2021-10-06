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
int fd, N, semid_id, semid_data, shmid_id, shmid_data, end_flag=0, *shmaddr_id;
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

void Sem_creation_error(int semid)
{
    if (semid==-1)
    {
        fprintf(stderr, "Error: Failed to create semaphore \n");
        exit(1);
    }	
}

void Shm_creation_error(int shmid)
{
    if (shmid==-1)
    {
        fprintf(stderr, "Error: Failed to create shared memory \n");
        exit(1);
    }		
}


/*--------------------------------------Sistem tools-----------------------------------------*/

void IPC_for_client_id() //Creating IPC tools for issuing an individual number to the clients	
{
	key1=ftok("/dev/null", 'q');
	Key_creation_error(key1);
	semid_id=semget(key1, 1, 0666|IPC_CREAT|IPC_EXCL);
	//printf("Semid = %d \n", semid_id);
	Sem_creation_error(semid_id);
	shmid_id=shmget(key1, 2*sizeof(int), 0666|IPC_CREAT|IPC_EXCL);
	//printf("Shmid = %d \n", shmid_id);
	Shm_creation_error(shmid_id);
	shmaddr_id=shmat(shmid_id, NULL, 0);
}

void IPC_for_data_transmission() //Creating IPC tools for transmitting data to the clients
{
	key2=ftok("/dev/mem", 'x');
	Key_creation_error(key2);
	semid_data=semget(key2, N, 0666|IPC_CREAT|IPC_EXCL);
	//printf("Semid = %d \n", semid_data);
	Sem_creation_error(semid_data);
	shmid_data=shmget(key2, MAXLEN*N, 0666|IPC_CREAT|IPC_EXCL);
	//printf("Shmid = %d \n", shmid_data);
	Shm_creation_error(shmid_data);
	shmaddr_data=shmat(shmid_data, NULL, 0);
}

void Delete_IPC(int semid, int shmid, char ch) //Removing IPC tools
{
	if (ch=='i')
		shmdt(shmaddr_id);
	else if (ch=='d')
		shmdt(shmaddr_data);
	shmctl(shmid, IPC_RMID, NULL);
	semctl(semid, 0, IPC_RMID, 0);	
}


/*----------------------------------------Functions------------------------------------------*/

void Issuing_id() //Issuing an individual number to the clients
{
	semctl(semid_id, 0, SETVAL, 0); //Initialize semaphore with 0
	sops.sem_num=0;
	sops.sem_flg=0;
	for(int i=0;i<N;i++)
	{
		shmaddr_id[0]=i;
		shmaddr_id[1]=N;
		sops.sem_op=2;
		semop(semid_id, &sops, 1);
		sops.sem_op=0;
		semop(semid_id, &sops,1);
	}
}

void Read_transmit_data(char *input_file_name)//Reading and transmitting data to clients
{
	FILE *fp;
	
	//Opening the input file
	fp=fopen(input_file_name, "r");

	int *buf=(int*)calloc(N, sizeof(int));
	semctl(semid_data, 0, SETALL, buf);	//Initialize semaphore array with 0
	
	while (!end_flag)
	{
		for(int i=0;i<N;i++)
		{
			if (!fgets(data, MAXLEN, fp))
			{
				end_flag=1;
				break;
			}
			sops.sem_num=i;
			sops.sem_flg=0;
			sops.sem_op=2;
			strcpy(shmaddr_data+(MAXLEN*i), data);
			semop(semid_data, &sops, 1);
			sops.sem_op=0;
			semop(semid_data, &sops, 1);
		}
	}
	
	//Closing the input file
	fclose(fp);
	
	free(buf);
}


/*------------------------------------The main function--------------------------------------*/

int main(int argc, char **argv)
{
	FILE *fp;
	N=atoi(argv[1]);

	if (argc!=3)
	{
		fprintf(stderr, "Unsufficient number of arguments \n");
		exit(1);
	}
		
	//Clearing or creating a file for the output
	fp=fopen("output", "w");
	File_opening_error(fp);
	fclose(fp);
	
	//Creating IPC tools
	IPC_for_client_id();
	IPC_for_data_transmission();
	
	//Issue an id and transmit data to the clients
	Issuing_id();
	Read_transmit_data(argv[2]);
	
	//Removing IPC tools
	Delete_IPC(semid_id, shmid_id, 'i');
	Delete_IPC(semid_data, shmid_data, 'd');
	
	return 0;
}

