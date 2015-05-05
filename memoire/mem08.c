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


#define NB_PROD 5  // nb productors
#define ARRAY_SIZE 5


int opsem(int sem, int i)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op  = i;
    op.sem_flg = 0;
    
    if ((i = semop(sem, &op, 1)) < 0)
    {
        perror("[semop]");
        exit(-1);
    }
    
    return i;
}

void down(int sem)
{
    opsem(sem, -1);
}

void up(int sem)
{
    opsem(sem, +1);
}


int empty, filled, position, shm;
struct strci * ci;
pid_t pids[NB_PROD + 1]; // pids of productors + consommator


void clean()
{
    int i = 0;
    for (; i < (NB_PROD + 1); ++i)
        kill(pids[i], SIGPIPE);
        
    printf("\n"); fflush(stdout);

    // detach shared memory
    shmdt(ci);
    
    // delete shared memory
    shmctl(shm, IPC_RMID, 0);


    // delete filled sem
    if ((semctl(filled, 1, IPC_RMID, 0)) < 0)
    {
        perror("[semctl -- delete]");
        exit(-1);
    }
    
    // delete empty sem
    if ((semctl(empty, 1, IPC_RMID, 0)) < 0)
    {
        perror("[semctl -- delete]");
        exit(-1);
    }
    
    // delete position sem
    if ((semctl(position, 1, IPC_RMID, 0)) < 0)
    {
        perror("[semctl -- delete]");
        exit(-1);
    }
}

struct strci
{
    char text[ARRAY_SIZE];
    int pos;
};


int main()
{
    int i;
    
    signal(SIGINT, clean);
   
    
    // init 5 bytes memory zone
    if ((shm = shmget(8, 5, 0777|IPC_CREAT)) < 0)
    {
        perror("[shmget]");
        exit(-1);
    }
    
    // attach memory zone
    ci = (struct strci *) shmat(shm, 0, 0777);
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
    
    // init semaphore empty cases at 5
    if ((semctl(empty, 0, SETVAL, 5)) < 0)
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
            pids[i] = getpid();
            
            for (i = 0; i < 26; ++i)
            {
                down(empty);
                (*ci).text[(*ci).pos] = 'a' + i;
                
                down(position);
                //printf("[debug − %d] :: %d −− %c\n", getpid(), (*ci).pos, (*ci).text[(*ci).pos]); fflush(stdout);
                (*ci).pos += 1;
                (*ci).pos %= 5;
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
        for (i = 0; i < (NB_PROD * 26); ++i)
        {
            down(filled);
            printf("%c", (*ci).text[i % 5]); fflush(stdout);
            sleep(1);
            up(empty);
        }
        
        printf("\n");
        exit(0);
    }
    
    // wait for the forks to finish
    while (wait(0) > 0);
    
    clean();

    exit(0);
}
