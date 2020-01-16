#pragma once

#include "simstruc.h"

SimStruct *CreateSimStructForFMI(const char* instanceName);

typedef void(*FreeMemoryCallback)(void*);

void FreeSimStruct(SimStruct *S, FreeMemoryCallback freeMemory);

void resetSimStructVectors(SimStruct *S);
