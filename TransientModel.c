#include <stdio.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"		       /* variate generator */
#include <stdlib.h>
#include "TransientModel.h"

#define START         0.0              /* initial time                   */ 

int TestNextEvent();
int TestGetTransaction();
int TestGetCumulative();
void makeTest();
int CheckRouting(double* matrix);
static double arrival = START;

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

int isEmpty(double array[4])
{
 for(int i = 0 ; i <4 ; i++){
   if(array[i] != 0.0){
	return 0;
   }
 }
 return 1;
}

   double GetService(int index)
/* --------------------------------------------
 * generate the next service time 
 * --------------------------------------------
 */ 
{
  SelectStream(index);
  if(index == 1)
  	return Exponential(1/8.0);
  if(index == 2)
	return Exponential(1/12.0);
  if(index == 3)
	return Exponential(1/6.0);
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
  double* cumulative = (double*) malloc(sizeof(double)*(16));
  int x = 0,y=1;
  for (x=0; x<4; x++){	
	*(cumulative+x*4) = *(matrix + x*4);
        for(y=1; y<3; y++){
		*(cumulative+x*4+y)  = *(cumulative+x*4+y-1) + *(matrix + x*4 + y);
  	}
        *(cumulative+x*4+3) = 1;
  }
  return cumulative;
  
}

int GetTransaction(int index, double* cumulativeMatrix)
{
  SelectStream(5);
  double x = Random();
  int nextIndex = 0;
  while(*(cumulativeMatrix+ index*4 + nextIndex) <= x)
	nextIndex++;
  return nextIndex;
}

double* Simulate(double stop, int seed, int print, double p12,double p13, double p22,double p23,double p32,double p33, double number[4], double* result)
{
  PlantSeeds(seed);
  //makeTest();
  result = (double*) malloc(sizeof(double)*(2));
  double STOP = stop ;
  double infinity  = 100.0 * STOP;
  double routing[4][4] = {{0.0,0.6,0.3,0.1},
			  {0.0,0.0,p12,p13},
			  {0.67,0.0,p22,p23},
			  {0.67,0.0,p32,p33}}; // routing matrix 



  int checkRouting = CheckRouting(routing[0]);
  if(checkRouting == 1){
        printf("Error on routing matrix\n");
	result[0] = 1000.0;
	result[1] = 1000.0;
	return result;
  }
  double* cumulativeMatrix = GetCumulative(routing[0]);
  double departed[4] = {0,0,0,0};
  struct {
    double arrival;                 /* next arrival time                   */
    double completion;              /* next completion time                */
    double current;                 /* current time                        */
    double next;                    /* next (most imminent) event time     */
    double last;                    /* last arrival time                   */
  } t = {0.0,0.0,0.0,0.0};
  double eventList[4] = {0.0,infinity,infinity,infinity}; /* event list */
  double last_departure[4] = {0.0,0.0,0.0,0.0};
  typedef struct {
   double node;                    /* time integrated number in the node  */
   double queue;                   /* time integrated number in the queue */
   double service;                 /* time integrated number in service */  
  } area;
  area* integral = (area*) malloc(sizeof(area)*4);
  for(int i = 1; i < 4; i++){
    integral[i].node = 0.0;
    integral[i].queue = 0.0;
    integral[i].service = 0.0;
  }
  t.current = START;
  eventList[0] = GetArrival();
  for(int i = 1; i < 4; i++){
	if(number[i] >0)
		eventList[i] = GetService(i);
  }
  int processed = 0;

  while ((eventList[0] < STOP)) {
    int i = NextEvent(eventList,4);  /* next event index   */
    t.next = eventList[i]; 
    for(int x = 1; x< 4; x++){
     if(number[x]>0){
	integral[x].node += (t.next - t.current) * number[x];
	integral[x].queue += (t.next - t.current) * (number[x]-1);
	integral[x].service += (t.next - t.current);
     }
    }
    t.current       = t.next;             /*advance the clock*/   
    if(i==0){
          eventList[0] = GetArrival();
	  int index = GetTransaction(0, cumulativeMatrix);
          number[index]++;
	  double service = 0.0;
	  if(number[index] == 1){
		service = GetService(index);
		eventList[index] = t.current + service;
	  }
	  if(eventList[0] > STOP){
            t.last = t.current;
	    eventList[0] = infinity;
	  }
    }
    else{
	  number[i]--;
	  departed[i]++;
	  last_departure[i] = t.current;
	  int nextIndex = GetTransaction(i, cumulativeMatrix);
          if(nextIndex>0){
	 	 number[nextIndex]++;
 		 double service = 0.0;
  		 if(number[nextIndex] == 1){
			service = GetService(nextIndex);
			eventList[nextIndex] = t.current + service;
  		 }
	  }else{
		processed++;	
	  }
	  
 	  
	  if(number[i] >0 )
		eventList[i] = t.current + GetService(i);
 	  else
		eventList[i] = infinity;
    } 
  } 
  double sum_integral = 0.0;
  double sum_remaining = 0.0;
  for(int i = 1; i < 4; i++){
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
   sum_remaining += number[i];
  
  }
  if(print == 1){
	printf("for %d jobs:\n", processed);
	printf("[TOTAL INTERARRIVAL TIME] = %6.3f \n\n\n", t.last/processed);
	printf("[Average response time]......= %6.3f\n",(sum_integral)/processed);
	printf("[Average # in the system]....= %6.3f\n",(sum_integral)/t.current);
  }
  arrival = START; // reset arrival for a next simulation
  result[0] = ((sum_integral)/processed);
  result[1] = sum_remaining;
  return result;

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
	double eventList1[4] = {0.0,0.1,0.2,0.3};
	int result = NextEvent(eventList1,4);
	if(result != 0){
		printf("Error on NextEvent function with eventList1. Expected: %d, Obtained: %d\n", 0,result);
		return 1;
	}
	double eventList2[4] = {1.0,0.1,0.2,0.3};
	result = NextEvent(eventList2,4);
	if(result != 1){
		printf("Error on NextEvent function with eventList2. Expected: %d, Obtained: %d\n", 1,result);
		return 1;
	}
	double eventList3[4] = {0.0,INFINITY, INFINITY,INFINITY};
	result = NextEvent(eventList3,4);
	if(result != 0){
		printf("Error on NextEvent function with eventList3. Expected: %d, Obtained: %d\n", 0,result);
		return 1;
	}	
	return 0;
	
}

int TestGetTransaction(){
	double testMatrix[4][4] = {{0.0,0.0,1.0,0.0},
			  {1.0,0.0,0.0,0.0},
			  {0.0,0.0,0.0,1.0},
			  {0.0,1.0,0.0,0.0},};
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
	return 0;


}

int TestGetCumulative(){
	double routing[4][4] = 
			 {{0.0,0.6,0.3,0.1},
			  {0.0,0.0,0.7,0.3},
			  {0.67,0.0,0.0,0.33},
			  {0.67,0.0,0.0,0.33},}; 
	
	double expectedMatrix[4][4] =
			 {{0.0,0.6,0.9,1.0},
			  {0.0,0.0,0.7,1.0},
			  {0.67,0.67,0.67,1.0},
			  {0.67,0.67,0.67,1.0},};
	double* cumulativeMatrix = GetCumulative(routing[0]);
	for(int i = 0; i <4;i++){
		for(int j = 0;  j< 4; j++){
			if(fabs(*(cumulativeMatrix+i*4+j) - expectedMatrix[i][j]) > 0.0000001){
				printf("Error with TestCumulative. Index [%d, %d]. Expected %f, got %f\n", i, j ,*(cumulativeMatrix+i*4+j),expectedMatrix[i][j]);
				return 1;
			}
				
		}

	}
	return 0;


}

//controlla che la matrice di routing sia ben composta

int CheckRouting(double* matrix){
  int x,y;
  double sum = 0;
  for (x=0; x<4; x++){	
        for(y=0; y<4; y++){
		sum += *(matrix+x*4+y);
  	}
        if(fabs((sum - 1.0)) > 0.0000000001){
	  return 1;
	}
        sum = 0;
  }
  return 0;

}


