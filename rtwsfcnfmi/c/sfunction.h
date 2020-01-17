#pragma once

#if defined(_MSC_VER)
#include "windows.h" /* for HINSTANCE */
#endif

#include "simstruc.h"

#define SFCN_FMI_EPS 2e-13	/* Not supported with discrete sample times smaller than this */

/* Model status */
typedef enum {
	modelInstantiated,
	modelInitializationMode,
	modelEventMode,
	modelContinuousTimeMode,
	modelTerminated
} ModelStatus;

/* Model data structure */
typedef struct {
	void *userData;
	const char* instanceName;
	int loggingOn;
	SimStruct* S;
	real_T* dX;
	real_T* oldZC;
	int_T* numSampleHits;
	int_T fixed_in_minor_step_offset_tid;
	real_T nextHit_tid0;
	void** inputs;
	void** outputs;
	void** parameters;
	void** blockoutputs;
	void** dwork;
	real_T lastGetTime;
	int shouldRecompute;
	int isCoSim;
	int isDiscrete;
	int hasEnteredContMode;
	real_T time;
	real_T nbrSolverSteps;
	ModelStatus status;
#if defined(_MSC_VER)
	HINSTANCE* mexHandles;
#else
	void** mexHandles;
#endif
	real_T* inputDerivatives;
	real_T derivativeTime;
} Model;

/* Function to copy per-task sample hits */
void copyPerTaskSampleHits(SimStruct* S);

SimStruct *CreateSimStructForFMI(const char* instanceName);

typedef void(*FreeMemoryCallback)(void*);

void FreeSimStruct(SimStruct *S, FreeMemoryCallback freeMemory);
void resetSimStructVectors(SimStruct *S);
void allocateSimStructVectors(Model* m);
void setSampleStartValues(Model* m);

/* ODE solver functions */
extern void rt_CreateIntegrationData(SimStruct *S);
extern void rt_DestroyIntegrationData(SimStruct *S);
extern void rt_UpdateContinuousStates(SimStruct *S);
