#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

int sem;
struct sembuf op;

void down(int value)
{
    op.sem_op  = value;
            
    if (semop(sem, &op, 1) == -1)
        perror("[semop]");
}

void up(int value)
{
    down(value);
}

int main()
{
    if ((sem = semget(46, 1, 0666|IPC_CREAT)) == -1)
        perror("[semget]");
    
    if (semctl(sem, 0, SETVAL, 1) == -1)
        perror("[semctl]");
        
    if (fork() == 0)
    {
        while (1)
        {
            down(-1);
            
            printf("A "); fflush(stdout);
            sleep(1);

            up(+2);
        }
        
        exit(0);
    }
    
    if (fork() == 0)
    {
        while (1)
        {
            down(-2);
            
            printf("B "); fflush(stdout);
            sleep(1);

            up(+1);
        }
        
        exit(0);
    }
    
    while (wait(0) >= 0);
    
    return 0;
}
