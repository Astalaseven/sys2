/* Généralisez le programme du producteur-consommateur au cas où on a plusieurs producteurs
et un seul consommateur. Chaque producteur produit les 26 lettres de l’alphabet. Le consommateur les
affiche. Par exemple, pour deux producteurs, le résultat doit être 52 lettres, peu importe l’ordre.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define NB_PROD 2  // nb productors
#define ARRAY_SIZE 5


struct strci
{
    char text[ARRAY_SIZE];
    int pos;
    pid_t pids[NB_PROD + 1]; // pids of productors + consommator
};

int empty, filled, position, shm;
struct strci * ci;


void opsem(int sem, int i)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op  = i;
    op.sem_flg = 0;
    
    if ((semop(sem, &op, 1)) < 0)
    {
        perror("[semop]");
        exit(-1);
    }
}

void down(int sem)
{
    opsem(sem, -1);
}

void up(int sem)
{
    opsem(sem, +1);
}

void clean(int signal)
{
    int i = 0;
    for (; i < (NB_PROD + 1); ++i)
        kill((*ci).pids[i], SIGKILL);

    // detach shared memory
    if ((shmdt(ci)) < 0)
    {
        perror("[shmdt]");
    }
    
    // delete shared memory
    if ((shmctl(shm, IPC_RMID, 0)) < 0)
    {
        perror("[shmctl -- delete]");
    }

    // delete filled sem
    if ((semctl(filled, 1, IPC_RMID, 0)) < 0)
    {
        perror("[semctl -- delete]");
    }
    
    // delete empty sem
    if ((semctl(empty, 1, IPC_RMID, 0)) < 0)
    {
        perror("[semctl -- delete]");
    }
    
    // delete position sem
    if ((semctl(position, 1, IPC_RMID, 0)) < 0)
    {
        perror("[semctl -- delete]");
    }
    
    exit(signal);
}

int main()
{
    int i;
    
    // handler for ctrl-c, needed to stop the program and clean memory/semaphores
    signal(SIGINT, clean);
    
    
    // init ARRAY_SIZE bytes memory zone
    if ((shm = shmget(7, sizeof(struct strci), 0666|IPC_CREAT)) < 0)
    {
        perror("[shmget]");
        exit(-1);
    }
    
    // attach memory zone
    ci = (struct strci *) shmat(shm, 0, 0666);
    (*ci).pos = 0;
    
    // create semaphore filled cases
    if ((filled = semget(8, 1, 0666|IPC_CREAT)) < 0)
    {
        perror("[semget -- filled]");
        exit(-1);
    }
    
    // init semaphore filled cases at 0
    if (semctl(filled, 0, SETVAL, 0) < 0)
    {
        perror("[semctl -- filled]");
        exit(-1);
    }
    
    // create semaphore empty cases
    if ((empty = semget(9, 1, 0666|IPC_CREAT)) < 0)
    {
        perror("[semget -- empty]");
        exit(-1);
    }
    
    // init semaphore empty cases at ARRAY_SIZE
    if ((semctl(empty, 0, SETVAL, ARRAY_SIZE)) < 0)
    {
        perror("[semctl -- empty]");
        exit(-1);
    }
    
    // create semaphore position
    if ((position = semget(10, 1, 0666|IPC_CREAT)) < 0)
    {
        perror("[semget -- position]");
        exit(-1);
    }
    
    // init semaphore position at 1
    if ((semctl(position, 0, SETVAL, 1)) < 0)
    {
        perror("[semctl -- position]");
        exit(-1);
    }
    
    // spawn of new productors
    for (i = 0; i < NB_PROD; ++i)
    {
        if (fork() == 0)
        {
            (*ci).pids[i] = getpid();
            
            for (i = 0; i < 26; ++i)
            {
                down(empty);
                (*ci).text[(*ci).pos] = 'a' + i;
                
                down(position);
                //printf("[debug − %d] :: %d −− %c\n", getpid(), (*ci).pos, (*ci).text[(*ci).pos]); fflush(stdout);
                (*ci).pos += 1;
                (*ci).pos %= ARRAY_SIZE;
                up(position);
                
                sleep(1);
                up(filled);
            }
            
            exit(0);    
        }
    }
    
    // spawn only consommator
    if (fork() == 0)
    {
        (*ci).pids[NB_PROD] = getpid();
        
        for (i = 0; i < (NB_PROD * 26); ++i)
        {
            down(filled);
            printf("%c", (*ci).text[i % ARRAY_SIZE]); fflush(stdout);
            sleep(1);
            up(empty);
        }
        
        printf("\n");
        exit(0);
    }
    
    // wait for the forks to finish
    while (wait(0) > 0);
    
    clean(0);
}
