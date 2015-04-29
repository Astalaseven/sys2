/* Généralisez-le programme du producteur-consommateur au cas ou on a un producteur et plu-
sieurs consommateurs. Le producteur produit les 26 lettres de l’alphabet. Chaque consommateur affiche.
ce qu’il lit. Le résultat doit être 26 lettres, peu importe qui consomme quoi. */
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>


#define NB_CONS 5  // nb consommators
#define ARRAY_SIZE 5


struct strcii
{
    char text[ARRAY_SIZE];
    int head;
    int queue;
};

static int empty, filled, tq, shm, sem;
static struct strcii * cii;
static pid_t pids[NB_CONS + 1]; // pids of productors + consommator


int opsem(int sem, int i)
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

void delete_shm(int shm)
{
    if ((shmctl(shm, IPC_RMID, 0)) < 0)
    {
        perror("[shmctl -- delete]");
        exit(-1);
    }
}

void delete_sem(int sem)
{
    if ((semctl(sem, 1, IPC_RMID, 0)) < 0)
    {
        perror("[semctl -- delete]");
        exit(-1);
    }
}


void clean(int signal)
{
    printf("\n");
    
    // kill all child process before memory/semaphore cleaning
    int i = 0;
    for (; i < (NB_CONS + 1); ++i)
        kill(pids[i], SIGPIPE);
    
    // detach and delete shared memory
    shmdt(cii);
    delete_shm(shm);
    
    // delete semaphores
    delete_sem(filled);
    delete_sem(empty);
    
    exit(0);
}

int main()
{
    int i, j;

    // handler for ctrl-c, needed to stop the program and clean memory/semaphores
    signal(SIGINT, clean);
    
    
    // init ARRAY_SIZE bytes memory zone
    if ((shm = shmget(10, ARRAY_SIZE, 0777|IPC_CREAT)) < 0)
    {
        perror("[shmget]");
        exit(-1);
    }
    
    // attach memory zone
    cii = (struct strcii *) shmat(shm, 0, 0777);
    
    (*cii).head  = 0;
    (*cii).queue = 0;
    
        
    // create semaphore filled
    if ((filled = semget(10, 1, 0666|IPC_CREAT)) < 0)
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
    if ((empty = semget(11, 1, 0666|IPC_CREAT)) < 0)
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
    
    // create semaphore head queue
    if ((tq = semget(8, 1, 0666|IPC_CREAT)) < 0)
    {
        perror("[semget -- tq]");
        exit(-1);
    }
    
    // init semaphore head queue at 1
    if (semctl(tq, 0, SETVAL, 1) < 0)
    {
        perror("[semctl -- tq]");
        exit(-1);
    }
    
    // spawn only productor
    if (fork() == 0)
    {
        pids[0] = getpid(); // save all pids to kill them later
        
        int i = 0;
        
        while (1)
        {
            down(empty);
            
            (*cii).queue += 1; // increment queue position
            (*cii).queue %= ARRAY_SIZE; // array has still length of ARRAY_SIZE
            
            (*cii).text[(*cii).queue] = ('a' + (i++ % 26));
            usleep(100000);
            
            up(filled);
        }
        
        exit(0);    
    }
    
    // spawn multiple consommators
    for (i = 0; i < NB_CONS; ++i)
    {
        if (fork() == 0)
        {
            pids[i + 1] = getpid(); // + 1 because productor pid
        
            while (1)
            {
                down(filled);
               
                down(tq);
                (*cii).head += 1;
                (*cii).head %= ARRAY_SIZE;
                up(tq);
                             
                printf("%c ", (*cii).text[(*cii).head]); fflush(stdout);
                
                up(empty);
            }

            exit(0);
        }
    }
    
    // wait for the children to finish
    while (wait(0) > 0);

    exit(0);
}
