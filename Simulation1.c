#include <stdio.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"		       /* variate generator */
#include <stdlib.h>
#include <string.h>
#include "TransientModel.h"

/* ---------------
 *
 * Funzione che fa partire la simulazione per lo stato transitorio, 
 * Prova ogni combinazione per p12 = [0.50;0.90]; p22 = [0.10;0.20]; p32 = [0.10; 0.20]
 * e per ognuno ritorna il tempo di risposta osservato ed il numero di job rimasti nel sistema
 * ---------------
 */ 
void SimulateFailure(double initialState[4])
{
	printf("[SIMULATION STARTED]\n");
	for(double p12 = 0.50; p12 <=0.90; p12 += 0.05){
	   for(double p22 = 0.10; p22 <=0.20; p22 += 0.02){
		for(double p32 = 0.10; p32 <=0.20; p32 += 0.02){
		char filename[256];
		sprintf(filename, "transientResult/p12-%2.2fp22-%2.2fp32-%2.2f", p12,p22,p32);
		FILE* file = fopen(filename, "w+");
		if(file == NULL){
			printf("Error with %s\n",filename);
			return;
			
		}
		long seed = 123456789;
		double* response;
		printf("\n[Iterations for p12-%2.2fp22-%2.2fp32%2.2f] \n",p12,p22,p32);
		for(int i = 0; i<100;i++){
			response = Simulate(10800.0,seed, 0, p12,(1-p12),p22, 0.33-p22, p32, 0.33-p32,initialState, response);
			fprintf(file, "%f;%f\n",response[0],response[1]);
			fflush(file);
			GetSeed(&seed);
		}
		fclose(file);
		}
	   }
         }	  
}


int main(){
	double initialCondition[4] = {0.0,3.0,2.0,3.0};	
	SimulateFailure(initialCondition);

}
