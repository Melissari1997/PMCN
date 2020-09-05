#include <math.h>
#include <stdio.h>
#include <string.h>
#include "rvms.h"
#include <stdlib.h>

#define LOC 0.95   

                    
typedef struct{
  double mean;		/* struct per conservare i risultati ottenuti durante */
  double width;		/* l'analisi dei dati dello stato stazionario         */
}interval;

typedef struct{
  double meanResponseTime;
  double widthResponseTime; 		/* struct per conservare i risultati ottenuti durante */
  double meanRemainingJobs;		/* l'analisi dei dati dello stato transitorio         */
  double widthRemainingJobs;

}interval_transient;

typedef struct{
  double service[4];		/* struct per conservare i migliori risultati ottenuti durante */
  double utilization[4];	/* l'analisi dei dati dello stato stazionario         */	
  double interval[4];
}candidate;

typedef struct{
  double p12;
  double p22;			/* struct per conservare i migliori risultati ottenuti durante */
  double p32;			/* l'analisi dei dati dello stato transitorio         */
  interval_transient interval;

}candidateTransient;



/* ---------------
 *
 * Funzione che calcola un'intervallo di confidenza a partire dai dati scritti su un file, 
 * utilizzando l'algoritmo di Welford per calcolare la media campionaria e la deviazione standard
 * Viene utilizzata per lo studio dei dati prodotti durante l'analisi dello stato transitorio
 * 
 * ---------------
 */ 

interval_transient interval_estimation(FILE* file){
  long   n    = 0;                     /* counts data points */
  double sumResponse  = 0.0;
  double sumRemaining  = 0.0;
  double response;
  double remaining;
  double stdevResponse, stdevRemaining;
  double u, t, w;
  double diffResponse, diffRemaining;
  if(file == NULL){
	printf("Error\n");
	
  }
  
  interval_transient i = {0.0,0.0, 0.0,0.0};
  while (!feof(file)) {                                                 /* use Welford's one-pass method */
    fscanf(file,"%lf;%lf\n", &response, &remaining);                    /* to calculate the sample mean  */
    n++;                                                                /* and standard deviation        */
    diffResponse  = response - i.meanResponseTime;
    sumResponse  += diffResponse * diffResponse * (n - 1.0) / n;
    i.meanResponseTime += diffResponse / n;
   
    diffRemaining  = remaining - i.meanRemainingJobs;
    sumRemaining  += diffRemaining * diffRemaining * (n - 1.0) / n;
    i.meanRemainingJobs += diffRemaining / n;
  }
  stdevResponse  = sqrt(sumResponse / n);
  stdevRemaining  = sqrt(sumRemaining / n);

  if (n > 1) {
    u = 1.0 - 0.5 * (1.0 - LOC);             				 /* interval parameter  */
    t = idfStudent(n - 1, u);                				 /* critical value of t */
    i.widthResponseTime = t * stdevResponse / sqrt(n - 1);               /* interval half width */
    i.widthRemainingJobs = t * stdevRemaining / sqrt(n - 1); 
  }
  else
    printf("ERROR - insufficient data\n");
  return (i);
}

/* ---------------
 *
 * Funzione che ritorna l'indice della configurazione di probabilità di routing con tempo di risposta più elevato
 * tra le 10 configurazioni migliori trovate fino a quel momento. 
 * Viene utilizzata per lo studio dei dati prodotti durante l'analisi dello stato transitorio 
 * ---------------
 */ 

int getMaxTimeIndex(candidateTransient* candidate){
	int maxIndex = 0;
	for(int i = 1; i < 10; i++){
		if(candidate[i].interval.meanResponseTime > candidate[maxIndex].interval.meanResponseTime){
			maxIndex = i;
		}
		if(candidate[i].interval.meanResponseTime == candidate[maxIndex].interval.meanResponseTime){
			if(candidate[i].interval.meanRemainingJobs > candidate[maxIndex].interval.meanRemainingJobs){
					maxIndex = i;
			}
		}

	}
	return maxIndex;
  }


/* ---------------
 *
 * Funzione che analizza i dati prodotti durante la simulazione dello stato transitorio  
 * Calcola un intervallo di confidenza per ogni configurazione analizzata, mantenendo in memoria le migliori 10 che verrano poi scritte su un file
 * ---------------
 */ 
  void TransientResult(){
	char filename[256];
	FILE* file;
	FILE* resultFile = fopen("ResultTransient", "w+");
	candidateTransient candidate[10];

	for(int i = 0; i< 10; i++){
		candidate[i].interval.meanResponseTime = 10000000;
		candidate[i].interval.widthResponseTime = 10000000;			//inizializzazione 
		candidate[i].interval.meanRemainingJobs = 100000000;
		candidate[i].interval.widthRemainingJobs = 100000000;

	}

	for(double p12 = 0.50; p12 <= 0.90; p12 += 0.05){
	   for(double p22 = 0.10; p22 <0.20; p22 += 0.02){
		for(double p32 = 0.10; p32 <0.20; p32 += 0.02){
			snprintf(filename,sizeof(filename), "transientResult/p12-%2.2fp22-%2.2fp32-%2.2f", p12,p22,p32);
			file = fopen(filename,"r");
			interval_transient result = interval_estimation(file);
			int maxIndex = getMaxTimeIndex(candidate);
			if(candidate[maxIndex].interval.meanResponseTime > result.meanResponseTime){				/* Controlla se la configurazione appena analizzata può rientrare nella  */ 																        /*   lista delle migliori 10                                             */
				candidate[maxIndex].p12 = p12;
				candidate[maxIndex].p22 = p22;															
				candidate[maxIndex].p32 = p32;									
				candidate[maxIndex].interval.meanResponseTime = result.meanResponseTime;
				candidate[maxIndex].interval.widthResponseTime = result.widthResponseTime;
				candidate[maxIndex].interval.meanRemainingJobs = result.meanRemainingJobs;
				candidate[maxIndex].interval.widthRemainingJobs = result.widthRemainingJobs;
			}
			
		}
	   }

        }
	for(int i = 0; i<10; i++){
		printf("Config number: %d\n",i);	
		printf("p12: %2.2f\n",candidate[i].p12);
		printf("p22: %2.2f\n",candidate[i].p22);
		printf("p32: %2.2f\n",candidate[i].p32);
		printf("Mean response time: %2.3f +/- %2.3f\n", candidate[i].interval.meanResponseTime, candidate[i].interval.widthResponseTime);
		printf("Mean remaining jobs: %2.3f +/- %2.3f\n", candidate[i].interval.meanRemainingJobs, candidate[i].interval.widthRemainingJobs);
		printf("\n\n\n");


		fprintf(resultFile, "Config number: %d\n",i + 1);	
		fprintf(resultFile, "p12: %2.2f\n",candidate[i].p12);
		fprintf(resultFile, "p22: %2.2f\n",candidate[i].p22);
		fprintf(resultFile, "p32: %2.2f\n",candidate[i].p32);
		fprintf(resultFile, "Mean response time: %2.3f +/- %2.3f\n", candidate[i].interval.meanResponseTime, candidate[i].interval.widthResponseTime);
		fprintf(resultFile, "Mean remaining jobs: %2.3f +/- %2.3f\n", candidate[i].interval.meanRemainingJobs, candidate[i].interval.widthRemainingJobs);
		

	}

}





/* ---------------
 *
 * Funzione che calcola un'intervallo di confidenza tramite batch means a partire dai dati scritti su un file, 
 * utilizzando l'algoritmo di Welford per calcolare la media campionaria e la deviazione standard.
 * Viene utilizzata per lo studio dei dati prodotti durante l'analisi dello stato stazionario
 * 
 * ---------------
 */

interval batchMeansIntervalEstimation(FILE* file, int numBatch, int batchSize)
{
  long   n    = 0;                     /* counts data points */
  double sum  = 0.0;
  double mean = 0.0;
  double data;
  double stdev;
  double u, t, w;
  double diff;
  char filename[256];
  candidate results[5];
  snprintf(filename,sizeof(filename), "batchMeans/numBatch_%2.2d_batchSize_%2.2d", numBatch,batchSize);
  FILE* batchMeansFile = fopen(filename, "w+");
  if(file == NULL){
	printf("Error\n");
	
  }
  interval* confInt = (interval*)malloc(sizeof(interval)*numBatch);
  for(int i = 0; i < numBatch; i++){
	confInt[i].mean = 0;
	confInt[i].width = 0;
  }
  int index = 0;
  int tot = 0;
  while (!feof(file)) {                        /* use Welford's one-pass method */
    fscanf(file,"%lf\n", &data);               /* to calculate the sample mean  */
    n++;                                       /* and standard deviation        */
    tot++;
    diff  = data - confInt[index].mean;
    sum  += diff * diff * (n - 1.0) / n;
    confInt[index].mean += diff / n;
    if(n%batchSize == 0){
       index++;
       diff = 0.0;
       sum = 0.0;
       n = 0.0;
       
    }  
  }
 
  diff = 0.0;
  sum = 0.0;
  n = 0.0;
  interval i = {0.0,0.0};
  for(int j = 0; j < numBatch ; j++){
    n++;
    diff  = confInt[j].mean -i.mean;
    sum  += diff * diff * (n - 1.0) / n;
    i.mean += diff / n;
  }
  stdev  = sqrt(sum / numBatch);

  if (numBatch > 1) {
    u = 1.0 - 0.5 * (1.0 - LOC);                     /* interval parameter  */
    t = idfStudent(numBatch - 1, u);                 /* critical value of t */
    i.width = t * stdev / sqrt(numBatch - 1);        /* interval half width */
  }
  else
    printf("ERROR - insufficient data: %ld\n", n);
  return (i);
}


/* ---------------
 *
 * Funzione che ritorna l'indice della configurazione che ha prodottu un'utilizzazione più elevata
 * tra le 5 configurazioni migliori trovate fino a quel momento. 
 * Viene utilizzata per lo studio dei dati prodotti durante l'analisi dello stato stazionario 
 * ---------------
 */ 
int GetMaxIndex(candidate* results){
	double maxTotalCost = results[0].service[0] + results[0].service[0] + results[0].service[2] + results[0].service[3];
	int maxIndex = 0;
	for(int j = 1; j < 5 ; j++){
		double totalCost = results[j].service[0] + results[j].service[1] + results[j].service[2] + results[j].service[3];
		if(totalCost > maxTotalCost){
			maxTotalCost = totalCost;
			maxIndex = j;
		}
	}
	return maxIndex;

}


/* ---------------
 *
 * Funzione che analizza i dati prodotti durante la simulazione dello stato stazionario 
 * Calcola un intervallo di confidenza per ogni configurazione analizzata, mantenendo in memoria le migliori 5 che verrano poi scritte su un file
 * ---------------
 */ 

  void SteadyStateResult()
{
  char filename_service1[256];
  char filename_service2[256];
  char filename_service3[256];
  char filename_service4[256];
  candidate results[5];
  FILE* resultFile = fopen("ResultSteadyState", "w+");
  for(int i = 0; i<5; i++){
	for(int j = 0; j<4; j++){
		results[i].service[j] = 50.0;
		results[i].utilization[j] = 0;
		results[i].interval[j] = 0;	
	}
  }
  for(double service1 = 8.0; service1 <=10.0; service1 ++){
           for(double service2 = 11.0; service2 <=15.0; service2 ++){
		for(double service3 = 6.0; service3 <= 8.0; service3 = service3 + 0.5){
			for(double service4 = 2.0; service4 <= 4.0; service4 = service4 + 0.5){
				printf("--------------\n\n\n\n");
				snprintf(filename_service1,sizeof(filename_service1), "SteadyResult/Service1/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
				FILE* service1File = fopen(filename_service1, "r");
				if(service1File == NULL)
					printf("Error 1 %s \n", filename_service1);
				interval resultService1 = batchMeansIntervalEstimation(service1File,64,40500); 
				

				snprintf(filename_service2,sizeof(filename_service2), "SteadyResult/Service2/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
				FILE* service2File = fopen(filename_service2, "r");
				if(service2File == NULL)
					printf("Error 2 %s \n", filename_service2);
				interval resultService2 = batchMeansIntervalEstimation(service2File,64,40500); 
				
				


				snprintf(filename_service3,sizeof(filename_service3), "SteadyResult/Service3/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
				FILE* service3File = fopen(filename_service3, "r");
				if(service3File == NULL)
					printf("Error 3 %s \n", filename_service3);
				interval resultService3 = batchMeansIntervalEstimation(service3File,64,40500); 
				


				snprintf(filename_service4,sizeof(filename_service4), "SteadyResult/Service4/service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f", service1,service2,service3,service4);
				FILE* service4File = fopen(filename_service4, "r");
				if(service4File == NULL)
					printf("Error 4 %s  \n", filename_service4);
				interval resultService4 = batchMeansIntervalEstimation(service4File,64,40500); 
				fclose(service1File);
				fclose(service2File);
				fclose(service3File);
				fclose(service4File);
				printf("Server 1 result for service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f: [%f, %f] \n", service1,service2,service3,service4, resultService1.mean -resultService1.width, resultService1.mean +resultService1.width);
					printf("Server 2 result for service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f: [%f, %f] \n", service1,service2,service3,service4, resultService2.mean -resultService2.width, resultService2.mean +resultService2.width);

				        printf("Server 3 result for service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f: [%f, %f]\n", service1,service2,service3,service4, resultService3.mean -resultService3.width, resultService3.mean +resultService3.width);

					printf("Server 4 result for service1-%2.2fservice2-%2.2f-service3-%2.2fservice4-%2.2f: [%f, %f] \n", service1,service2,service3,service4, resultService4.mean -resultService4.width, resultService4.mean +resultService4.width);
					printf("\n\n");

				if( (resultService1.mean + resultService1.width >= 0.65 && resultService1.mean - resultService1.width <=0.80) &&
				    (resultService2.mean + resultService2.width >= 0.65 && resultService2.mean - resultService2.width <=0.80) &&
				    (resultService3.mean + resultService3.width >= 0.65 && resultService3.mean - resultService3.width <=0.80) &&
				    (resultService4.mean + resultService4.width >= 0.65 && resultService4.mean - resultService4.width <=0.80)){ /* Controlla se la configurazione appena analizzata */
																	        /* può rientrare nella lista delle migliori 10      */	
					int maxIndex = GetMaxIndex(results);
					double totalCost = results[maxIndex].service[0] + results[maxIndex].service[1] + results[maxIndex].service[2] + results[maxIndex].service[3]; 
					double totalCostToCompare = service1+service2+service3+service4;
					if(totalCostToCompare < totalCost){
						results[maxIndex].service[0] = service1;
						results[maxIndex].service[1] = service2;
						results[maxIndex].service[2] = service3;
						results[maxIndex].service[3] = service4;

						results[maxIndex].utilization[0] = resultService1.mean;
						results[maxIndex].utilization[1] = resultService2.mean;
						results[maxIndex].utilization[2] = resultService3.mean;
						results[maxIndex].utilization[3] = resultService4.mean;

						results[maxIndex].interval[0] = resultService1.width;
						results[maxIndex].interval[1] = resultService2.width;
						results[maxIndex].interval[2] = resultService3.width;
						results[maxIndex].interval[3] = resultService4.width;
					}
				}


			}
	        }			
           }	
	}
  

	for(int i = 0; i < 5 ; i++){
		printf("--------CONFIGURATION %d-------\n", i+1);
		fprintf(resultFile, "Configurazione %d\n",i+1);
		for(int j = 0; j < 4 ; j++){
			printf("------Server %d---------\n", j+1);
			printf("%2.2f || ",results[i].service[j]);
			printf("%2.2f || ",results[i].utilization[j]);
			printf("%f\n",results[i].interval[j]);
			fprintf(resultFile, "Tasso di servizio %2.2f. Utilizzazione %f +/- %f\n",results[i].service[j],results[i].utilization[j],results[i].interval[j]);
			
		}
		printf("---------------\n");
		


	}
	


}

  int main(void)
{
	TransientResult();   
	SteadyStateResult();
   	return 0;
}
