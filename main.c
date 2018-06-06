/*
  _____ _______ _    _
 |_   _|__   __| |  | |
   | |    | |  | |  | |
   | |    | |  | |  | |
  _| |_   | |  | |__| |
 |_____|  |_|   \____/

@Author
Student Name: Orçun Özdemir
Student ID: 150140121
Date: 25/04/2018

Compile: "gcc main.c -o main -lpthread"
Execute: "./main file.txt"

-lpthread linking parameter is necessary for sem_init, sem_wait, sem_post, and sem_destroy.

*/

#include <stdio.h>  // I/O functions
#include <stdlib.h> // Basic functions
#include <unistd.h> // fork() and getpid()
#include <semaphore.h>  // Semaphore operations
#include <sys/mman.h> // Shared memory allocations
#include <signal.h> // Signal operations
#define SIZE 256  // Maximum number of requested coffees.
#define S_EMPLOYEES 5 // Maximum number of employees. (You can change it!)

int *W; // Current water level.
int fullW;  // Full water level as giving in the first line of the text file.
int *fillingComplete; // Filling complete check.
int *isFinished;  // Simulation ending check.

void fill_it (int signum) { // Signal for water filler process to wake him up.
  *W = fullW; // New water level is starting full level.
  printf("Employee %d wakes up and fills the coffee machine\n", getpid());
  printf("Current water level %d cups\n", fullW);
  *fillingComplete = 1; // Filling is finished.
}

void kill_it (int signum) { // Signal to terminate remaining processes whether the simulation finished.
  exit(0);
}

void fillerSignal (int num) { // Signal to trigger fill_it function.
  struct sigaction mysigaction;
  mysigaction.sa_handler = (void *) fill_it;
  mysigaction.sa_flags = 0;
  sigaction (num, &mysigaction, NULL);
}

void killerSignal (int num) { // Signal to trigger kill_it function.
  struct sigaction mysigaction2;
  mysigaction2.sa_handler = (void *) kill_it;
  mysigaction2.sa_flags = 0;
  sigaction (num, &mysigaction2, NULL);
}

int main(int argc, char const *argv[]) {
  pid_t temp, (*employees)[S_EMPLOYEES];
  int *emp; // Store processes IDs with the help of this.
  int *coffeeNumber;  // Number of coffee that will get the current coffee type and will be incremented as coffeeType[(*coffeeNumber)++]
  int *coffeeType = malloc(sizeof(int) * SIZE); // Array that contains types of coffees order by request.
  FILE *fp; // File pointer for reading operations.
  fillerSignal(1);  // Signal number = 1.
  killerSignal(2);  // Signal number = 2.

  // Shared variables to synchronize processes.
  W = mmap(NULL, sizeof *W, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  fillingComplete = mmap(NULL, sizeof *W, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *fillingComplete = 0;
  isFinished = mmap(NULL, sizeof *W, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *isFinished = 0;
  employees = mmap(NULL, sizeof *W, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  emp = mmap(NULL, sizeof *W, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *emp = 0;
  coffeeNumber = mmap(NULL, sizeof *W, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *coffeeNumber = 0;

  // Semaphores to indicate whether taking coffee is available or not.
  sem_t* machineBusy = (sem_t*)mmap(0, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0 );
  sem_t* fillingCheck = (sem_t*)mmap(0, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0 );

  // Initialize semaphore from 1 and do error handling.
  if (sem_init(machineBusy, 1, 1) == -1)
    printf("Semaphore couldn't be started!");

  // Initialize semaphore from 1 and do error handling.
  if (sem_init(fillingCheck, 1, 1) == -1)
    printf("Semaphore couldn't be started!");

  // Error handling on starting parameters.
  if (argc != 2){
    printf("Wrong command!\nExample Usage: ./program file.txt\n");
  }

  else {
    fp = fopen(argv[1], "r"); // Only read operation.

    if (fp == NULL) { // Error handling for file operation.
        perror(argv[1]);
        exit(EXIT_FAILURE);
    }

    fscanf(fp, "%d", W);  // Take the first line as W.
    fullW = *W;           // Initialize fullW.
    for (int i = 0; *(coffeeType + i) != EOF; i++)
      fscanf(fp, "%d", coffeeType + i);

    printf("W = %d\nRequested Coffee Types: ", *W);
    for (int i = 0; coffeeType[i] != '\0'; i++)
      printf("%d ", coffeeType[i]);

    fclose(fp);
  }

  *employees[(S_EMPLOYEES)] = getpid(); // Last element of the array will be the grandparent process' ID.

  printf("\n\nSIMULATION BEGINS\n");
  printf("Current water level %d cups\n", fullW);

  for (int i = 0; i < S_EMPLOYEES; i++) {
    temp = fork();
      if (temp == 0) {
        *employees[(*emp)++] = getpid();
        break;
      }
      else if (temp == -1)
        printf("Child couldn't be born!\n");
  }

  if (getpid() == (*employees)[0]) {
    pause(); // Catch the first employee make him "water filler" and get him to sleep.
  }

  while ((*isFinished) == 0) {
    sem_wait(machineBusy);

    if (coffeeType[(*coffeeNumber)] == 0)
      break;

  printf("Employee %d wants coffee Type %d\n", getpid(), coffeeType[(*coffeeNumber)]);

  sem_wait(fillingCheck);
  if ((*W) - coffeeType[(*coffeeNumber)] < 1) {
      printf("Employee %d WAITS\n", getpid());
      kill((*employees)[0], 1);
      while (*fillingComplete == 0) {}
      *fillingComplete = 0;
      sem_post(fillingCheck);
    }
  else {
      sem_post(fillingCheck);
  }

  (*W) -= coffeeType[(*coffeeNumber)++];
  printf("Employee %d SERVED\n", getpid());
  printf("Current water level %d cups\n", *W);

  sem_post(machineBusy);
  sleep(1); // Respect to other employees wait a little bit to take another coffee! (You can change it to make simulation faster.)
  }

  if (*isFinished == 0) {
    *isFinished = 1;
    printf("SIMULATION ENDS\n\n");

    for (int i = 0; i < S_EMPLOYEES + 1; i++) { // Kill the remaining processes.
        if (getpid() != (*employees[i])) {
          kill((*employees[i]), 2);
        }
    }
  }

    sem_destroy(machineBusy); // Release and delete semaphores.
    sem_destroy(fillingCheck);
    free(coffeeType); // Release memory resources.
    munmap(W, sizeof(W)); // Release shared memory resources.
    munmap(coffeeNumber, sizeof(coffeeNumber));
    munmap(fillingComplete, sizeof(fillingComplete));
    munmap(isFinished, sizeof(isFinished));
    munmap(employees, sizeof(employees));
    munmap(emp, sizeof(emp));

    return EXIT_SUCCESS;
}
