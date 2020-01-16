#pragma once

#if defined(_MSC_VER)
#include "windows.h" /* for HINSTANCE */
#endif

#include "simstruc.h"

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
	void* functions;  // TODO: move to userData
	real_T lastGetTime;
	int shouldRecompute;
	int isCoSim;
	int isDiscrete;
	int hasEnteredContMode;
	real_T time;
	real_T nbrSolverSteps;
	void* eventInfo;  // TODO: move to userData
	ModelStatus status;
#if defined(_MSC_VER)
	HINSTANCE* mexHandles;
#else
	void** mexHandles;
#endif
	real_T* inputDerivatives;
	real_T derivativeTime;
} Model;

SimStruct *CreateSimStructForFMI(const char* instanceName);

typedef void(*FreeMemoryCallback)(void*);

void FreeSimStruct(SimStruct *S, FreeMemoryCallback freeMemory);

void resetSimStructVectors(SimStruct *S);

void allocateSimStructVectors(Model* m);

/* ------------------ ODE solver functions ------------------- */
extern void rt_CreateIntegrationData(SimStruct *S);
extern void rt_DestroyIntegrationData(SimStruct *S);
extern void rt_UpdateContinuousStates(SimStruct *S);
