#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <cstdio>
#include <map>

using namespace std;

typedef struct
{
    void* n;
    int num;
} queue;

//functions for the queue
void enqueue(queue** q, int n)
{
    queue* t = *q;
    *q = (queue*)malloc(sizeof(queue));
    (*q)->n = (void*)t;
    (*q)->num = n;
}

int dequeue(queue** q)
{
	queue* t = *q;
	if (!t)
		return -1;
	queue* p = t;
	for (; t->n; p = t, t = (queue*)t->n);
	int r = t->num;
	p->n = 0;
	free(t);
	if (t == *q)
		*q = 0;
	return r;
}

//number of each of the characters
const int NUM_CUSTOMERS = 5;
const int NUMBER_TELLERS = 2;
const int NUMBER_LOAN_OFFICER = 1;

//semaphores that are used in the project
sem_t mutex1, mutex2;
sem_t teller;
sem_t LO;
sem_t readyForTeller;
sem_t requestMade;
sem_t loanRequest;
sem_t processDone;
sem_t receipt;
sem_t readyForLO;
sem_t loanProcessDone;
sem_t loanReceipt;
sem_t finished[NUM_CUSTOMERS];


int count = 0;

//various queues being used 
queue* LOq = 0;
queue* tellq = 0;
queue* tellAmount = 0;
queue* LOAmount = 0;
queue* taskNum = 0;


//map for the customer and teller numbers
map<int, int> custTell;

int balance[NUM_CUSTOMERS];
int loan[NUM_CUSTOMERS];

//this function represent the customer and all that they can do 
void* customer(void* param)
{
    time_t t;
    srand((unsigned) time(&t));
    int id = (int)(long)param;
    printf("Customer %d created \n", id);
    int task, amount;
    
    //will make each customer go to the bank 3 times 
    for(int x=0; x<3; x++)
    {
        sem_wait(&mutex1);
        printf("Customer %d enters the bank.\n", id);
        task = rand() % 3 + 1;
        amount = rand() % 5 + 1;
        amount *= 100;
        //printf("Customer %d: task: %d   amount: %d \n", id, task, amount);
        sem_post(&mutex1);

	//if customer wants to make a deposit
        if(task == 1)
        {
            sem_wait(&teller);
	    //puts various values in the queue
            sem_wait(&mutex2);
            enqueue(&tellq, id);
            enqueue(&tellAmount, amount);
            enqueue(&taskNum, task);
            sem_post(&readyForTeller);
            sem_post(&mutex2);
            usleep(100000);
            sem_post(&requestMade);
            printf("Customer %d requests of teller %d to make a deposit of $%d \n", id, custTell[id], amount);
            sem_wait(&processDone);
            sem_wait(&receipt);
            printf("Customer %d gets a receipt from the teller %d \n", id, custTell[id]);
            usleep(100000);
	    sem_post(&finished[id]);
        }

	//if customer wants to make a withdraw
        if(task == 2)
        {
            sem_wait(&teller);
	    //puts values in the queue
            sem_wait(&mutex2);
            enqueue(&tellq, id);
            enqueue(&tellAmount, amount);
            enqueue(&taskNum, task);
            sem_post(&readyForTeller);
            sem_post(&mutex2);
            usleep(100000);
            sem_post(&requestMade);
            printf("Customer %d requests of teller %d to make a withdraw of $%d \n", id, custTell[id], amount);
            sem_wait(&processDone);
            sem_wait(&receipt);
            printf("Customer %d gets cash and receipt from teller %d \n", id, custTell[id]);
            usleep(100000);
	    sem_post(&finished[id]);
        }

	//if the customer wants to get a loan
        if(task == 3)
        {
            sem_wait(&LO);
	    //puts values in the queue
            sem_wait(&mutex2);
            enqueue(&LOq, id);
            enqueue(&LOAmount, amount);
            sem_post(&readyForLO);
            sem_post(&mutex2);
            usleep(100000);
            sem_post(&loanRequest);
            printf("Customer %d requests of loan officer to apply for a loan of $%d \n", id, amount);
            sem_wait(&loanProcessDone);
            sem_wait(&loanReceipt);
            printf("Customer %d gets loan from loan officer \n", id);
            usleep(100000);
	    sem_post(&finished[id]);
        }
    }
}

//this function represents the bank teller and all they can do
void* bankTeller(void* param)
{
    int tellerNum = (int)(long)param;
    printf("Teller %d created \n", tellerNum);
    int custNum, amount, task;
    while(true)
    {
        sem_wait(&readyForTeller);
	//gets information from the queue
        sem_wait(&mutex2);
        custNum = dequeue(&tellq);
        amount = dequeue(&tellAmount);
        task = dequeue(&taskNum);
        custTell[custNum] = tellerNum;
	printf("Teller %d begins serving customer %d \n", tellerNum, custNum);
        sem_post(&mutex2);
        sem_wait(&requestMade);

	//if customer wants to make a deposit
        if(task == 1)
        {

            //process deposit
            balance[custNum] += amount;
            usleep(400000);
            sem_post(&processDone);
            printf("Teller %d processes deposit of $%d for customer %d \n", tellerNum, amount, custNum);
            sem_post(&receipt);
            sem_wait(&finished[custNum]);
	    sem_post(&teller);
        }

	//if customer wants to make a withdraw
        if(task == 2)
        {

            //process withdraw
            balance[custNum] -= amount;
            usleep(400000);
            sem_post(&processDone);
            printf("Teller %d processes withdraw of $%d for customer %d \n", tellerNum, amount, custNum);
            sem_post(&receipt);
	    sem_wait(&finished[custNum]);
            sem_post(&teller);
        }
    }
}

//this function represents the loan officer and all its functions
void* loanOfficer(void* param)
{
    int LONum = (int)(long)param;
    printf("Loan officer created \n");
    int custNum, amount;
    while(true)
    {
        sem_wait(&readyForLO);
	//gets information from the queue
        sem_wait(&mutex2);
        custNum = dequeue(&LOq);
        amount = dequeue(&LOAmount);
        sem_post(&mutex2);
        printf("Loan officer serving customer %d \n", custNum);
        sem_wait(&loanRequest);
        //process loan
        balance[custNum] += amount;
        loan[custNum] += amount;
        usleep(400000);
        sem_post(&loanProcessDone);
        printf("Loan officer approves loan for customer %d \n", custNum);
        sem_post(&loanReceipt);
	sem_wait(&finished[custNum]);
        sem_post(&LO);
    }
}

//creates all the semaphores and threads
int main()
{
  //sets the customers initial balance and loan balance to 0
    for(int x=0; x<NUM_CUSTOMERS; x++)
    {
        balance[x] = 1000;
        loan[x] = 0;
	sem_init(&finished[x], 0, 0);
    }


    //initializes all semaphores
    sem_init(&mutex1, 0, 1);
    sem_init(&mutex2, 0, 1);
    sem_init(&teller, 0, 2);
    sem_init(&LO, 0, 1);
    sem_init(&readyForTeller, 0, 0);
    sem_init(&requestMade, 0, 0);
    sem_init(&loanRequest, 0, 0);
    sem_init(&processDone, 0, 0);
    sem_init(&receipt, 0, 0);
    sem_init(&readyForLO, 0, 0);
    sem_init(&loanProcessDone, 0, 0);
    sem_init(&loanReceipt, 0, 0);


    //initializes all threads
    pthread_t customers[NUM_CUSTOMERS];
    pthread_t tellers[NUMBER_TELLERS];
    pthread_t loanOfficers[NUMBER_LOAN_OFFICER];

    //creates all the threads
    for(int x=0; x<NUM_CUSTOMERS; x++)
      pthread_create(&customers[x], NULL, customer, (void *)(intptr_t)x);

    for(int x=0; x<NUMBER_TELLERS; x++)
      pthread_create(&tellers[x], NULL, bankTeller, (void *)(intptr_t)x);

    for(int x=0; x<NUMBER_LOAN_OFFICER; x++)
      pthread_create(&loanOfficers[x], NULL, loanOfficer, (void *)(intptr_t)x);


    //joins the customer processes and kills the threads of tellers and loan officers
    for(int x=0; x<NUM_CUSTOMERS; x++)
    {

        pthread_join(customers[x], NULL);
        printf("Guest %d is joined by main. \n", x);
    }

    //kills tellers and loan officers
    for(int x=0; x<NUMBER_TELLERS; x++)
        pthread_kill(tellers[x], 0);
    for(int x=0; x<NUMBER_LOAN_OFFICER; x++)
        pthread_kill(loanOfficers[x], 0);

    //prints the loan and balance totals
    int balTotal = 0;
    int loanTotal = 0;
    printf("\n\n%20s %s \n\n", "", "Bank Simulation Summary");
    printf("%20s %-20s %-20s \n\n", "", "Ending Balance", "Loan Amount");
    for(int x=0; x<NUM_CUSTOMERS; x++)
    {
        printf("Customer %d %-9s %-20d %-20d \n", x, "", balance[x], loan[x]);
        balTotal += balance[x];
        loanTotal += loan[x];
    }
    printf("\n%-20s %-20d %-20d \n", "Totals", balTotal, loanTotal);

    return 0;
}
