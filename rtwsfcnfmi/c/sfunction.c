#include "sfcn_fmi_rel_conf.h"
#include "sfcn_fmi.h"
#include "sfunction.h"


extern void* allocateMemory0(size_t nobj, size_t size);

static int_T RegNumInputPortsCB_FMI(void *Sptr, int_T nInputPorts)
{
	SimStruct *S = (SimStruct *)Sptr;

	if (nInputPorts < 0) {
		return(0);
	}

	_ssSetNumInputPorts(S, nInputPorts);
	_ssSetSfcnUsesNumPorts(S, 1);

	if (nInputPorts > 0) {
		ssSetPortInfoForInputs(S,
			(struct _ssPortInputs*) allocateMemory0((size_t)nInputPorts,
				sizeof(struct _ssPortInputs)));
	}

	return(1);
}

static int_T RegNumOutputPortsCB_FMI(void *Sptr, int_T nOutputPorts)
{
	SimStruct *S = (SimStruct *)Sptr;

	if (nOutputPorts < 0) {
		return(0);
	}

	_ssSetNumOutputPorts(S, nOutputPorts);
	_ssSetSfcnUsesNumPorts(S, 1);

	if (nOutputPorts > 0) {
		ssSetPortInfoForOutputs(S,
			(struct _ssPortOutputs*) allocateMemory0((size_t)nOutputPorts,
				sizeof(struct _ssPortOutputs)));
	}

	return(1);
}

/* Setup of port dimensions when compiled as MATLAB_MEX_FILE (including simulink.c) */
static int_T SetInputPortWidth_FMI(SimStruct *arg1, int_T port, const DimsInfo_T *dimsInfo)
{
	arg1->portInfo.inputs[port].width = dimsInfo->width;
	return 1;
}

static int_T SetOutputPortWidth_FMI(SimStruct *arg1, int_T port, const DimsInfo_T *dimsInfo)
{
	arg1->portInfo.outputs[port].width = dimsInfo->width;
	return 1;
}

static DTypeId registerDataTypeFcn_FMI(void * arg1, const char_T * dataTypeName)
{
	int_T i;
	SimStruct* S = (SimStruct*)arg1;

	for (i = 0; i<S->sizes.numDWork; i++) {
		if (((int_T*)(S->mdlInfo->dataTypeAccess->dataTypeTable))[i] == 0) {
			/* Found next free data type id */
			break;
		}
	}
	return i + 15; /* Offset from Simulink built-in data type ids */
}

static int_T setDataTypeSizeFcn_FMI(void * arg1, DTypeId id, int_T size)
{
	SimStruct* S = (SimStruct*)arg1;

	((int_T*)(S->mdlInfo->dataTypeAccess->dataTypeTable))[id - 15] = size;
	return 1;
}

static int_T setNumDWork_FMI(SimStruct* S, int_T numDWork)
{
	S->work.dWork.sfcn = (struct _ssDWorkRecord*) allocateMemory0((size_t)numDWork, sizeof(struct _ssDWorkRecord));
	S->sizes.numDWork = numDWork;

	if (S->mdlInfo != NULL) {
		S->mdlInfo->dataTypeAccess = (slDataTypeAccess*)allocateMemory0(1, sizeof(slDataTypeAccess));
		S->mdlInfo->dataTypeAccess->dataTypeTable = (int_T*)allocateMemory0((size_t)numDWork, sizeof(int_T));
	}
	return 1;
}

SimStruct *CreateSimStructForFMI(const char* instanceName)
{
	SimStruct *S = (SimStruct*)allocateMemory0(1, sizeof(SimStruct));
	if (S == NULL) {
		return NULL;
	}
	S->mdlInfo = (struct _ssMdlInfo*)allocateMemory0(1, sizeof(struct _ssMdlInfo));
	if (S->mdlInfo == NULL) {
		return NULL;
	}

	_ssSetRootSS(S, S);
	_ssSetSimMode(S, SS_SIMMODE_SIZES_CALL_ONLY);
	_ssSetSFcnParamsCount(S, 0);

	_ssSetPath(S, SFCN_FMI_MODEL_IDENTIFIER);
	_ssSetModelName(S, instanceName);

	ssSetRegNumInputPortsFcn(S, RegNumInputPortsCB_FMI);
	ssSetRegNumInputPortsFcnArg(S, (void *)S);
	ssSetRegNumOutputPortsFcn(S, RegNumOutputPortsCB_FMI);
	ssSetRegNumOutputPortsFcnArg(S, (void *)S);
	ssSetRegInputPortDimensionInfoFcn(S, SetInputPortWidth_FMI);
	ssSetRegOutputPortDimensionInfoFcn(S, SetOutputPortWidth_FMI);
	/* Support for custom data types */
	S->regDataType.arg1 = S;
	S->regDataType.registerFcn = registerDataTypeFcn_FMI;
	S->regDataType.setSizeFcn = setDataTypeSizeFcn_FMI;
	/* The following SimStruct initialization is required for use with RTW-generated S-functions */
	S->mdlInfo->simMode = SS_SIMMODE_NORMAL;
	S->mdlInfo->variableStepSolver = SFCN_FMI_IS_VARIABLE_STEP_SOLVER;
	S->mdlInfo->fixedStepSize = SFCN_FMI_FIXED_STEP_SIZE;
	S->mdlInfo->stepSize = SFCN_FMI_FIXED_STEP_SIZE;						/* Step size used by ODE solver */
	S->mdlInfo->solverMode = (SFCN_FMI_IS_MT == 1) ? SOLVER_MODE_MULTITASKING : SOLVER_MODE_SINGLETASKING;
	S->mdlInfo->solverExtrapolationOrder = SFCN_FMI_EXTRAPOLATION_ORDER;	/* Extrapolation order for ode14x */
	S->mdlInfo->solverNumberNewtonIterations = SFCN_FMI_NEWTON_ITER;		/* Number of iterations for ode14x */
	S->mdlInfo->simTimeStep = MAJOR_TIME_STEP; /* Make ssIsMajorTimeStep return true during initialization */
	S->sfcnParams.dlgNum = 0;  /* No dialog parameters, check performed in mdlInitializeSizes */
	S->errorStatus.str = NULL; /* No error */
	S->blkInfo.block = NULL;   /* Accessed by ssSetOutputPortBusMode in mdlInitializeSizes */
	S->regDataType.setNumDWorkFcn = setNumDWork_FMI;

#if defined(MATLAB_R2011a_) || defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
	S->states.statesInfo2 = (struct _ssStatesInfo2 *) allocateMemory0(1, sizeof(struct _ssStatesInfo2));
#if defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
	S->states.statesInfo2->periodicStatesInfo = (ssPeriodicStatesInfo *)allocateMemory0(1, sizeof(ssPeriodicStatesInfo));
#endif
#endif

	return(S);
}
