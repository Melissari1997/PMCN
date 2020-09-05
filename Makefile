simulation1: Simulation1.c  TransientModel.c rvms.c rvgs.c rngs.c
		gcc -o Simulation1 Simulation1.c  TransientModel.c rvms.c rvgs.c rngs.c -lm

simulation2: Simulation2.c  SteadyStateModel.c rvms.c rvgs.c rngs.c
		gcc -o Simulation2 Simulation2.c  SteadyStateModel.c rvms.c rvgs.c rngs.c -lm

estimate: intEstimate.c rvms.c rvgs.c rngs.c 
	gcc -o Estimate intEstimate.c  rvms.c rvgs.c rngs.c -lm

model: model.c rvms.c rvgs.c rngs.c 
	gcc -o Model model.c  rvms.c rvgs.c rngs.c -lm
