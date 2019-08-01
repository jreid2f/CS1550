#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <linux/unistd.h>
#include <sys/mman.h>

/*
  Encapsulating the semaphore
*/
struct cs1550_sem{
    int value;
    struct cs_node* head;
    struct cs_node* tail;
};

/*
   down semaphore operation
*/
void down(struct cs1550_sem *sem){
    syscall(__NR_sys_cs1550_down, sem);
}

/*
   up semaphore operation
*/
void up(struct cs1550_sem *sem){
    syscall(__NR_sys_cs1550_up, sem);
}

int main(int argc, char* argv[]){
    int x;              // For loop variable
    int cars = 10;      // Max number of cars on the road
    srand(time(NULL));  // Random number generator
    int stall;          // Used to help wait for the program to finish
    int totalAmount;    // Helps to check and display the car number when they leave
    char direction;     // Helps to check and display the direction of either north or south
    char north = 'N';
    char south = 'S'; 

    /*
        This allows the user to change the buffer size
    */
    if(argc == 2){
        cars = atoi(argv[1]);
    }

    /*
        First, we have to create space in memory for specific variables 
        This will keep track of the start time for the program
    */
    void *start_time = mmap(NULL, sizeof(time_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    
    /*
        This will keep track if a car honked
    */
    void *car_honk = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    
    /*
        This will keep track of the direction the flag person is looking
    */
    void *flag_dir = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    
    /*
        This will keep track the number of cars going in both directions
    */
    void *car_dir = mmap(NULL, sizeof(long), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

    time_t* start = (time_t*) start_time;  // Designate the start pointer for start_time memory
    char* honk = (char*) car_honk;         // Designate the start pointer for car_honk memory
    char* flag = (char*) flag_dir;         // Designate the start pointer for flag_dir memory
    long* total_cars = (long*) car_dir;    // Designate the start pointer for the car_dir memory

    /*
      Next, we should create space for six semaphores to help keep track for mutual exclusion
    */
    void *mutual_ex = mmap(NULL, sizeof(struct cs1550_sem) * 6, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

   struct cs1550_sem* mutex = (struct cs1550_sem*) mutual_ex;           // Keep track of the mutual exclusion
   struct cs1550_sem* northFull = (struct cs1550_sem*) mutual_ex + 1;   // Check for full spots in the north
   struct cs1550_sem* northEmpty = (struct cs1550_sem*) mutual_ex + 2;  // Check for empty spots in the north
   struct cs1550_sem* southFull = (struct cs1550_sem*) mutual_ex + 3;   // Check for full spots in the south
   struct cs1550_sem* southEmpty = (struct cs1550_sem*) mutual_ex + 4;  // Check for empty spots in the south
   struct cs1550_sem* intersect = (struct cs1550_sem*) mutual_ex + 5;   // Check for the number of cars in an interseciton

   /*
     Create space for both directions: North / South
   */
  void *northSpace = mmap(NULL, sizeof(int)*(cars + 2), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
  void *southSpace = mmap(NULL, sizeof(int)*(cars + 2), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

  /*
        Set each value for the semaphores above
  */
  mutex->value = 1;
  northFull->value = 0;
  northEmpty->value = cars;
  southFull->value = 0;
  southEmpty->value = cars;
  intersect->value = 0;

  mutex->head = NULL;           // Head of mutex set to NULL
  mutex->tail = NULL;           // Tail of mutex set to NULL

  northFull->head = NULL;       // Head of northFull set to NULL
  northFull->tail = NULL;       // Tail of northFull set to NULL

  southFull->head = NULL;       // Head of southFull set to NULL
  southFull->tail = NULL;       // Tail of southFull set to NULL

  northEmpty->head = NULL;      // Head of northEmpty set to NULL
  northEmpty->tail = NULL;      // Tail of northEmpty set to NULL

  southEmpty->head = NULL;      // Head of southEmpty set to NULL
  southEmpty->tail = NULL;      // Tail of southEmpty set to NULL

  intersect->head = NULL;       // Head of intersect set to NULL
  intersect->tail = NULL;       // Tail of intersect set to NULL

  /*
        Memory space is now created for the parts that we need, now is a good time
        to set the default for the honk, flag, and total_cars pointer variables
  */
  *honk = 'N';                 // This means that no one is honking
  *flag = ' ';                 // This means there is no in either direction of the flag person
  *total_cars = 0;             // This means that there is no cars on the road at this time

  /*
        Since the defaults are now set, at this point we know there is no cars yet
        Which means the flag person should start off sleeping
  */
  printf("The flagperson is now asleep");

  int* producer_north = (int*) northSpace;      // Holds the pointer for the producer in the north
  int* producer_south = (int*) southSpace;      // Holds the pointer for the producer in the south

  int* consumer_north = (int*) northSpace+1;    // Holds the pointer for the consumer in the north
  int* consumer_south = (int*) southSpace+1;    // Holds the pointer for the consumer in the south

  int* start_NBuff = (int*) northSpace+2;       // Looks for where the buffer starts in the north
  int* start_SBuff = (int*) southSpace+2;       // Looks for where the buffer starts in the south

  /*
        Set the producer and consumer directions to its default
  */
  *producer_north = 0;
  *producer_south = 0;
  *consumer_north = 0;
  *consumer_south = 0;

  /*
        Since there is no cars in either direction, the buffer in both directions
        should be set to default
  */
  for(x = 0; x < cars; x++){
      start_NBuff[x] = 0;
      start_SBuff[x] = 0;
  }

  // Now, since everything is set, start the program running time
  *start = time(NULL);

  /*
        Now start using the fork method to fork processes for either 
        the north producer or the south producer
  */
  if(fork() == 0){
     /*
        Start the fork process for the north producer
     */
     if(fork() == 0){
         while(1){
             down(northEmpty);     // Call the down method to run a syscall for the northEmpty queue
             down(mutex);          // Call the down method to run a syscall for the mutex variable

             *total_cars += 1;     // Increment the total number of cars by 1
             start_NBuff[*producer_north % cars] = *total_cars;    // Store which car number went through the intersection
             *producer_north += 1;  // Increment the spot within the buffer that is being produced

             if(intersect->value <= 0 && *honk == 'N'){
                 *honk = 'Y';       // Someone has honked
                 *flag = 'N';       // Flagperson looks north
                 printf("The flagperson is now awake\n");
                 printf("Car %d coming from %c, blew their horn at time %d.\n", *total_cars, *flag, time(NULL)-*start);
             }

             printf("Car %d coming from %c, arrived in the queue at time %d.\n", *total_cars, *flag, time(NULL)-*start);
             up(mutex);            // Call the up method to run a syscall for the mutex variable
             up(northFull);        // Call the up method to run a syscall for the northFull queue
             up(intersect);        // Call the up method to run a syscall for the intersect queue

             // If no cars are in the intersection, we can finally sleep
             if(rand() % 10 >= 8){
                 sleep(20);
             }
         }
     }

     /*
        Start the fork process for the south producer
     */
    else{
         while(1){
             down(southEmpty);     // Call the down method to run a syscall for the southEmpty queue
             down(mutex);          // Call the down method to run a syscall for the mutex variable

             *total_cars += 1;     // Increment the total number of cars by 1
             start_SBuff[*producer_south % cars] = *total_cars;    // Store which car number went through the intersection
             *producer_south += 1;  // Increment the spot within the buffer that is being produced

             if(intersect->value <= 0 && *honk == 'N'){
                 *honk = 'Y';       // Someone has honked
                 *flag = 'S';       // Flagperson looks south
                 printf("The flagperson is now awake\n");
                 printf("Car %d coming from %c, blew their horn at time %d.\n", *total_cars, *flag, time(NULL)-*start);
             }

             printf("Car %d coming from %c, arrived in the queue at time %d.\n", *total_cars, *flag, time(NULL)-*start);
             up(mutex);            // Call the up method to run a syscall for the mutex variable
             up(southFull);        // Call the up method to run a syscall for the southFull queue
             up(intersect);        // Call the up method to run a syscall for the intersect queue

             // If no cars are in the intersection, we can finally sleep
             if(rand() % 10 >= 8){
                 sleep(20);
             }
         }
    }
  }

  /*
        This should start the fork process for the consumer
  */
  if(fork() == 0){
      while(1){
          down(mutex);

          // If no cars are seen in the intersection the flag person will go back to sleep
          if(intersect->value == 0){
              printf("The flagperson is now asleep");
              *honk = 'N';
          }

          up(mutex);
          down(intersect);  // This should help with making the program sleep
          down(mutex);

          // If no cars are being added to the queue, it should exit
          if(intersect->value < 0){
              printf("Not enough cars in the intersection");
              exit(1);
          }

          if(*flag == 'N' && southFull->value >= 0){
              *flag = 'S';
          }
          else if(*flag == 'S' && northFull->value >= 0){
              *flag = 'N';
          }
          
          /*
            If the direction of the consumer is set to look to the north, check if the north
            section of the road is full. If it is, then start looking to the south
          */
          if(*flag == 'N'){
              down(northFull);
              direction = 'N';
              totalAmount = start_NBuff[(*consumer_north) % cars];
              consumer_north += 1;
              if(northFull->value == 0){
                  *flag = 'S';
              }
          }

          /*
            If the direction of the consumer is set to look to the south, check if the south
            section of the road is full. If it is, then start looking to the north
          */
          else if(*flag == 'S'){
              down(southFull);
              direction = 'S';
              totalAmount = start_SBuff[(*consumer_south) % cars];
              consumer_south += 1;
              if(southFull->value == 0){
                  *flag = 'N';
              }
          }

          up(mutex);        // Release the mutex
          sleep(2);         // Sleep since no more cars can run in the buffer

          printf("Car %d coming from the %c direction left the construction zone at time %d\n", totalAmount, direction, time(NULL) - *start);
          
          /*
            If the last direction check is either north or south, it should release the empty queue
            memory space
          */
          if(direction == 'N'){
              up(northEmpty);
          }
          else if(direction == 'S'){
              up(southEmpty);
          }
      }
  }

  // Wait until the program finally exits
  wait(&stall);
  return 0;

}
