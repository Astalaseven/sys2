/* Écrivez 5 process. Synchronisez ces process de telle façon que les process 3 et 4 ne peuvent s’exécuter
que quand les process 1 et 2 sont terminés et que le process 5 ne peut s’exécuter que quand les process 3
et 4 sont terminés. L’ordre de lancement des process est quelconque. Vous devez utiliser les sémaphores
de linux sans utiliser semaphor.c fourni à l’Esi. Prouvez que votre synchronisation est correcte. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SEM_0 0
#define SEM_1 1

int sem;

int main()
{
    int sem; // semaphore set identifier
    struct sembuf op;
    
    op.sem_flg = 0;
    

    /* Crée les 2 sémaphores */
    if ( (sem = semget(20, 2, 0666|IPC_CREAT)) == -1 )
    {
        perror("[semget]");
    }
    
    /* Initialise le premier sémaphore à 2 */
    if ( semctl(sem, SEM_0, SETVAL, 2) == -1 )
    {
        perror("[semctl -- SEM_0]");
    }
    
    /* Initialise le second sémaphore à 2 */
    if ( semctl(sem, SEM_1, SETVAL, 2) == -1 )
    {
        perror("[semctl -- SEM_1]");
    }

    /* Process 1 */
    if (fork() == 0)
    {
        printf("process 1\n"); fflush(stdout);
	    
	    sleep(2);
	    
	    /* BEGIN DOWN */
	    op.sem_num = SEM_0; // agit sur la première sémaphore
	    op.sem_op  = -1;    // pour faire un down dessus
	    
	    /* semop -> semaphore set identifier, sembuf et nb d’opérations dans le sembuf */
	    if ( semop(sem, &op, 1) == -1 )
	    {
	        perror("[process1 -- down]");
	    }
	    /* END DOWN */
	    
        exit(0);
    }
    
    /* Process 2 */
    if (fork() == 0)
    {
	    printf("process 2\n"); fflush(stdout);
	    
	    sleep(2);
	    
	    /* BEGIN DOWN */
	    op.sem_num = SEM_0;
	    op.sem_op  = -1;
	    
	    if ( semop(sem, &op, 1) == -1 )
	    {
	        perror("[process2 -- down]");
	    }
	    /* END DOWN */

        exit(0);
    }
    
    /* Process 3 */
    if (fork() == 0)
    {
        /* BEGIN ZERO */
	    op.sem_num = SEM_0;
	    op.sem_op  = 0;
    
	    if ( semop(sem, &op, 1) == -1 )
	    {
	        perror("[process3 -- zero]");
	    }
	    /* END ZERO */
	    
	    printf("process 3\n"); fflush(stdout);
	    
	    sleep(1);
	    
	    /* BEGIN DOWN */
	    op.sem_num = SEM_1;
	    op.sem_op  = -1;
	    
	    if ( semop(sem, &op, 1) == -1 )
	    {
	        perror("[process3 -- down]");
	    }
	    /* END DOWN */

        exit(0);
    }
    
    /* Process 4 */
    if (fork() == 0)
    {
        /* BEGIN ZERO */
	    op.sem_num = SEM_0;
	    op.sem_op  = 0;
	    
	    if ( semop(sem, &op, 1) == -1 )
	    {
	        perror("[process4 -- zero]");
	    }
	    /* END ZERO */
	    
	    printf("process 4\n"); fflush(stdout);
	    
	    sleep(1);
	    
	    /* BEGIN DOWN */
	    op.sem_num = SEM_1;
	    op.sem_op  = -1;
	    
	    if ( semop(sem, &op, 1) == -1 )
	    {
	        perror("[process4 -- down]");
	    }
	    /* END DOWN */

        exit(0);
    }
    
    /* Process 5 */
    if (fork() == 0)
    {
        /* BEGIN ZERO */
        op.sem_num = SEM_1;
	    op.sem_op  = 0;
	    
	    if ( semop(sem, &op, 1) == -1 )
	    {
	        perror("[process5 -- zero]");
	    }
	    /* END ZERO */
    
	    printf("process 5\n"); fflush(stdout);

        exit(0);
    }
    
    while (wait(0) >= 0);
    
    semctl(sem, 0, IPC_RMID);   
    
    exit(0);
}

