#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>


int semA, semB;
struct sembuf op;

void down(int sem)
{
    op.sem_op  = -1;
            
    if (semop(sem, &op, 1) < 0)
        perror("[semop -- down]");
}

void up(int sem)
{
    op.sem_op  = +1;
            
    if (semop(sem, &op, 1) < 0)
        perror("[semop -- up]");
}


int main()
{
    if ((semA = semget(46, 1, 0666|IPC_CREAT)) < 0)
        perror("[semget -- A]");
    
    if (semctl(semA, 0, SETVAL, 1) < 0)
        perror("[semctl -- A]");
        
    if ((semB = semget(47, 1, 0666|IPC_CREAT)) < 0)
        perror("[semget -- B]");
    
    if (semctl(semB, 0, SETVAL, 0) < 0)
        perror("[semctl -- B]");
        
    if (fork() == 0)
    {
        while (1)
        {
            down(semA);
            
            printf("A "); fflush(stdout);
            sleep(1);

            up(semB);
        }
        
        exit(0);
    }
    
    if (fork() == 0)
    {
        while (1)
        {
            down(semB);
            
            printf("B "); fflush(stdout);
            sleep(1);

            up(semA);
        }
        
        exit(0);
    }
    
    while (wait(0) >= 0);
    
    return 0;
}
