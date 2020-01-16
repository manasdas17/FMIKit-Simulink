#pragma once

#include "simstruc.h"

SimStruct *CreateSimStructForFMI(const char* instanceName);

typedef void(*FreeMemoryCallback)(void*);

void FreeSimStruct(SimStruct *S, FreeMemoryCallback freeMemory);

void resetSimStructVectors(SimStruct *S);

/* ------------------ ODE solver functions ------------------- */
extern void rt_CreateIntegrationData(SimStruct *S);
extern void rt_DestroyIntegrationData(SimStruct *S);
extern void rt_UpdateContinuousStates(SimStruct *S);
