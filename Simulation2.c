#include <stdio.h>
#include <math.h>
#include "rngs.h"                      /* the multi-stream generator */
#include "rvgs.h"		       /* variate generator */
#include <stdlib.h>
#include <string.h>
#include "steadyStateModel.h"



/* ---------------
 *
 * Funzione che fa partire la simulazione per lo stato stazionario, 
 * Prova ogni combinazione per i tassi di servizio:
 * Server 1: [7.0; 10.0]
 * Server 2: [9.0; 15.0]
 * Server 3: [6.0; 8.0 ]
 * Server 4: [6.0; 8.0 ]
 * La funzione RunSimulation simula il comportamento del sistema e scrive su un file tutte le
 * utilizzazioni osservate
 * ---------------
 */ 
void ChooseUtilization(int numJobs, long seed){
	printf("[SIMULATION STARTED]\n");
	for(double service1 = 10.0; service1 <=10.0; service1++){
	   printf("Running configuration:\n");
	   printf("- service 1: %f\n", service1);
           for(double service2 = 15.0; service2 <=15.0; service2++){
		for(double service3 = 6.0; service3 <= 8.0; service3 = service3 + 0.5){
			for(double service4 = 2.0; service4 <= 4.0; service4 = service4 + 0.5){
				RunSimulation(seed, 0, service1,service2,service3,service4, numJobs);
			}
	        }			
           }	
	}
	


}


int main(){
	ChooseUtilization(2592000,123456789); 
}
