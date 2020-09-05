#include <stdio.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"		       /* variate generator */
#include <stdlib.h>
#include "steadyStateModel.h"


#define START         0.0              /* initial time                   */ 
int TestNextEvent();
int TestGetTransaction();
int TestGetCumulative();
void makeTest();
int CheckRouting(double* matrix);

static double arrival = START;
typedef struct {
   double node;                    /* time integrated number in the node  */
   double queue;                   /* time integrated number in the queue */
   double service;                 /* time integrated number in service */  
  } area;


   double GetArrival()
/* ---------------------------------------------
 * generate the next arrival time, with rate 1/10
 * ---------------------------------------------
 */ 
{
  SelectStream(0); 
  arrival += Exponential(1/10.0);
  return (arrival);
} 

int isEmpty(double array[5])
{
 for(int i = 0 ; i <5 ; i++){
   if(array[i] != 0.0){
	return 0;
   }
 }
 return 1;
}

   double GetService(int index, double service)
/* --------------------------------------------
 * generate the next service time 
 * --------------------------------------------
 */ 
{
  SelectStream(index);
  return Exponential(1/service);
}  

   int NextEvent(double* eventList, int size)
{
   int i, location = 0;
   double minimum = eventList[0];
   for( i=1; i< size; i++){
	if(eventList[i] < minimum){
		minimum = eventList[i];
		location = i;
	}
   }
   
   return location;
}
  double* GetCumulative(double* matrix)
{
  double* cumulative = (double*) malloc(sizeof(double)*(25));
  int x = 0,y=1;
  for (x=0; x<5; x++){	
	*(cumulative+x*5) = *(matrix + x*5);
        for(y=1; y<4; y++){
		*(cumulative+x*5+y)  = *(cumulative+x*5+y-1) + *(matrix + x*5 + y);
  	}
        *(cumulative+x*5+4) = 1;
  }
  return cumulative;
  
}


int GetTransaction(int index, double* cumulativeMatrix)
{
  SelectStream(5);
  double x = Random();
  int nextIndex = 0;
  while(*(cumulativeMatrix+ index*5 + nextIndex) <= x)
	nextIndex++;
  return nextIndex;
}

/*
 * ---------------
 * La funzione RunSimulation simula il comportamento del sistema e scrive su un file tutte le
 * utilizzazioni osservate
 * ---------------
 */ 
void RunSimulation(int seed, int print, double service1, double service2, double service3, double service4, int numJobs)
{
  PlantSeeds(seed); 
  //makeTests();
  char filename_service1[256];
  char filename_service2[256];
  char filename_service3[256];
  char filename_service4[256];
  snprintf(filename_service1,sizeof(filename_service1), "steadyStateResult/Service1/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
  snprintf(filename_service2,sizeof(filename_service2), "steadyStateResult/Service2/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
  snprintf(filename_service3,sizeof(filename_service3), "steadyStateResult/Service3/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
  snprintf(filename_service4,sizeof(filename_service4), "steadyStateResult/Service4/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
  FILE* file_service1 = fopen(filename_service1, "w+");
  FILE* file_service2 = fopen(filename_service2, "w+");
  FILE* file_service3 = fopen(filename_service3, "w+");
  FILE* file_service4 = fopen(filename_service4, "w+");

  double services[4] = {service1,service2,service3,service4};

  double routing[5][5] = {{0.0,0.6,0.3,0.1,0.0},
			  {0.0,0.0,0.67,0.33,0.0},
			  {0.67,0.0,0.165,0.0,0.165},
			  {0.67,0.0,0.0,0.165,0.165},
			  {0.4 , 0.0 , 0.3   ,0.3   ,0.0}}; // routing matrix 

  int checkRouting = CheckRouting(routing[0]);
  if(checkRouting == 1){
        printf("Error on routing matrix\n");
	return;
  }
  double* cumulativeMatrix = GetCumulative(routing[0]);
  double departed[5] = {0,0,0,0,0};
  struct {
    double arrival;                 /* next arrival time                   */
    double completion;              /* next completion time                */
    double current;                 /* current time                        */
    double next;                    /* next (most imminent) event time     */
    double last;                    /* last arrival time                   */
  } t = {0.0,0.0,0.0,0.0,0.0};
  double eventList[5] = {0.0,INFINITY,INFINITY,INFINITY, INFINITY}; /* event list */
  double last_departure[5] = {0.0,0.0,0.0,0.0,0.0};
  area* integral = (area*) malloc(sizeof(area)*5);
  for(int i = 1; i < 5; i++){
    integral[i].node = 0.0;
    integral[i].queue = 0.0;
    integral[i].service = 0.0;
  }
  double number[5] = {0,0,0,0,0};                  /* number in the node  */
  for(int i = 1; i < 5; i++){
	if(number[i] >0)
		eventList[i] = GetService(i,services[i-1]);
  }
  t.current = START;
  eventList[0] = GetArrival();
  int processed = 0;
  int arrived = 0;

  while (processed < numJobs) {
    int i = NextEvent(eventList,5);  /* next event index   */
    t.next = eventList[i]; 
    for(int x = 1; x< 5; x++){
     if(number[x]>0){
	integral[x].node += (t.next - t.current) * number[x];
	integral[x].queue += (t.next - t.current) * (number[x]-1);
	integral[x].service += (t.next - t.current);
     }
    }
    t.current       = t.next;             /*advance the clock*/  
    if(i==0){
          eventList[0] = GetArrival();
	  arrived++;
	  int index = GetTransaction(0, cumulativeMatrix);
          number[index]++;
	  double service = 0.0;
	  if(number[index] == 1){
		service = GetService(index,services[index-1]);
		eventList[index] = t.current + service;
	  }
    }
    else{
	  number[i]--;
	  departed[i]++;
	  last_departure[i] = t.current;
	  int nextIndex = GetTransaction(i, cumulativeMatrix);
 
          if(nextIndex>0){
	 	 number[nextIndex]++;
  		 if(number[nextIndex] == 1){
			double service = GetService(nextIndex,services[nextIndex-1]);
			eventList[nextIndex] = t.current + service;
	
  		 }
	  }else{
		processed++;
	        fprintf(file_service1,"%f\n", integral[1].service / t.current);
  	        fflush(file_service1);
	        fprintf(file_service2,"%f\n", integral[2].service / t.current);
  	        fflush(file_service2);
	        fprintf(file_service3,"%f\n", integral[3].service / t.current);
  	        fflush(file_service3);
	        fprintf(file_service4,"%f\n", integral[4].service / t.current);
  	        fflush(file_service4);
	  }
	  
 	  
	  if(number[i] >0 )
		eventList[i] = t.current + GetService(i, services[i-1]);
 	  else
		eventList[i] = INFINITY;
    } 

  } 
  double sum_integral = 0.0;
  for(int i = 1; i < 5; i++){
   if(print == 1){
	  printf("[NODE %d]:\n",i);
	  printf("   average interarrival time = %6.3f\n", t.last / departed[i]);
	  printf("   average wait ............ = %6.3f\n", integral[i].node / departed[i]);
	  printf("   average # in the node ... = %6.3f\n", integral[i].node / t.current);
	  printf("   average time in queue ... = %6.3f\n", integral[i].queue / departed[i]);
	  printf("   average # in the queue .. = %6.3f\n", integral[i].queue / t.current);
	  printf("   average service time .... = %6.3f\n", integral[i].service / departed[i]);
	  printf("   utilization ............. = %6.3f\n", integral[i].service / t.current);
	  printf("   job remained in the node. = %6.3f\n", number[i]);
	  printf("\n\n\n");
   }
   sum_integral += integral[i].node;
  
  }
  if(print == 1){
	printf("for %d jobs:\n", processed);
	printf("[TOTAL INTERARRIVAL TIME] = %6.3f \n\n\n", t.last/processed);
	printf("[Average response time]......= %6.3f\n",(sum_integral)/processed);
	printf("[Average # in the system]....= %6.3f\n",(sum_integral)/t.current);
  }
  fclose(file_service1);
  fclose(file_service2);
  fclose(file_service3);
  fclose(file_service4);
  arrival = START; // reset arrival for a next simulation
}


/* --------------------------------------------
 * 
 *        		 TESTS
 * 
 * --------------------------------------------
 */ 

void makeTest(){
	int resultNextEvent = TestNextEvent(); 
	if(resultNextEvent == 1){
		exit(0);
	}
	int resultGetTransaction = TestGetTransaction(); 
	if(resultGetTransaction == 1){
		exit(0);
	}
	int resultGetCumulative = TestGetCumulative(); 
	if(resultGetCumulative == 1){
		
		exit(0);
	}
}


int TestNextEvent(){
	double eventList1[5] = {0.0,0.1,0.2,0.3,0.4};
	int result = NextEvent(eventList1,5);
	if(result != 0){
		printf("Error on NextEvent function with eventList1. Expected: %d, Obtained: %d\n", 0,result);
		return 1;
	}
	double eventList2[5] = {1.0,0.1,0.2,0.3,0.4};
	result = NextEvent(eventList2,5);
	if(result != 1){
		printf("Error on NextEvent function with eventList2. Expected: %d, Obtained: %d\n", 1,result);
		return 1;
	}
	double eventList3[5] = {0.0,INFINITY, INFINITY,INFINITY,INFINITY};
	result = NextEvent(eventList3,5);
	if(result != 0){
		printf("Error on NextEvent function with eventList3. Expected: %d, Obtained: %d\n", 0,result);
		return 1;
	}	
	return 0;
	
}

int TestGetTransaction(){
	double testMatrix[5][5] = {{0.0,0.0,1.0,0.0,0.0},
			  {1.0,0.0,0.0,0.0,0.0},
			  {0.0,0.0,0.0,1.0,0.0},
			  {0.0,1.0,0.0,0.0,0.0},
			  {0.67,0.0,0.0,0.0,0.33}};
	int result = GetTransaction(0,testMatrix[0]);
	if(result != 2){
		printf("Error on GetTransaction function with index 0. Expected: %d, Obtained: %d\n", 2,result);
		return 1;
	}
	result = GetTransaction(1,testMatrix[0]);
	if(result != 0){
		printf("Error on GetTransaction function with index 1. Expected: %d, Obtained: %d\n", 0,result);
		return 1;
	}
	result = GetTransaction(2,testMatrix[0]);
	if(result != 3){
		printf("Error on GetTransaction function with index 2. Expected: %d, Obtained: %d\n", 3,result);
		return 1;
	}
	result = GetTransaction(3,testMatrix[0]);
	if(result != 1){
		printf("Error on GetTransaction function with index 3. Expected: %d, Obtained: %d\n", 1,result);
		return 1;
	}
	result = GetTransaction(4,testMatrix[0]);
	if(result != 0 && result != 4){
		printf("Error on GetTransaction function with index 4. Expected: %d or %d, Obtained: %d\n", 0,4,result);
		return 1;
	}	
	return 0;


}

int TestGetCumulative(){
	double routing[5][5] = {{0.0,0.6,0.3,0.1,0.0},
			  {0.0,0.0,0.67,0.33,0.0},
			  {0.67,0.0,0.0,0.0,0.33},
			  {0.67,0.0,0.0,0.0,0.33},
			  {0.0,0.0,0.2,0.8,0.0}}; 	
	double expectedMatrix[5][5] = {{0.0,0.6,0.9,1.0,1.0},
			  {0.0,0.0,0.67,1.0,1.0},
			  {0.67,0.67,0.67,0.67,1.0},
			  {0.67,0.67,0.67,0.67,1.0},
			  {0.0,0.0,0.2,1.0,1.0}};
	double* cumulativeMatrix = GetCumulative(routing[0]);
	for(int i = 0; i <5;i++){
		for(int j = 0;  j< 5; j++){
			if(fabs(*(cumulativeMatrix+i*5+j) - expectedMatrix[i][j]) > 0.0000001){
				printf("Error with TestCumulative. Index [%d, %d]. Expected %f, got %f\n", i, j ,*(cumulativeMatrix+i*5+j),expectedMatrix[i][j]);
				return 1;
			}
				
		}

	}
	return 0;


}

int CheckRouting(double* matrix){
  int x,y;
  double sum = 0;
  for (x=0; x<5; x++){	
        for(y=0; y<5; y++){
		sum += *(matrix+x*5+y);
  	}
        if(fabs((sum - 1.0)) > 0.0000000001){
	  return 1;
	}
        sum = 0;
  }
  return 0;

}


