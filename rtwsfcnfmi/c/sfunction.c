#include "sfcn_fmi_rel_conf.h"
#include "sfcn_fmi.h"
#include "sfunction.h"

void copyPerTaskSampleHits(SimStruct* S) {
	int_T m, n;

	for (m = 1; m < S->sizes.numSampleTimes; m++) {
		for (n = 0; n < S->sizes.numSampleTimes; n++) {
			S->mdlInfo->perTaskSampleHits[n + m * (S->sizes.numSampleTimes)] = S->mdlInfo->sampleHits[n];
		}
	}
}

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

/* Common macro to allocate input port */
#define FMI_INPORT(type, value) {										            \
	int i;																			\
	type** inputPtrs;																\
	type* inputSignals;																\
	inputPtrs    = (type**) allocateMemory0(width, sizeof(type*));					\
	inputSignals = (type*) allocateMemory0(width, sizeof(type));					\
	for (i=0; i<width; i++) {														\
		inputSignals[i] = value;													\
		inputPtrs[i] = &(inputSignals[i]);											\
	}																				\
	arg1->portInfo.inputs[port].signal.ptrs = (InputPtrsType) inputPtrs;							\
}

static int_T SetInputPortDimensionInfoFcn_FMI(SimStruct *arg1, int_T port)
{
	int i;
	void** busInputPtrs;
	void*  busInputObject;
	int_T width = arg1->portInfo.inputs[port].width;

	/* Attempt allocating bus object for port */
	busInputObject = sfcn_fmi_allocateBusObject(1, port, width);
	if (busInputObject != 0) {
		busInputPtrs = (void**)allocateMemory0(width, sizeof(void*));
		busInputPtrs[0] = busInputObject;
		for (i = 1; i<width; i++) {
			busInputPtrs[i] = sfcn_fmi_allocateBusObject(1, port, width);
		}
		arg1->portInfo.inputs[port].signal.ptrs = (InputPtrsType)busInputPtrs;
		return 1;
	}

	/* Allocate port signal vectors */
	switch (arg1->portInfo.inputs[port].dataTypeId) {
	case SS_DOUBLE:   /* real_T    */
	{
		FMI_INPORT(real_T, 0.0);
	}
	break;
	case SS_SINGLE:   /* real32_T  */
		FMI_INPORT(real32_T, 0.0);
		break;
	case SS_INTEGER:  /* int_T */
	{
		FMI_INPORT(int_T, 0);
	}
	break;
	case SS_INT8:     /* int8_T    */
	{
		FMI_INPORT(int8_T, 0);
	}
	break;
	case SS_UINT8:    /* uint8_T   */
	{
		FMI_INPORT(uint8_T, 0);
	}
	break;
	case SS_INT16:    /* int16_T   */
	{
		FMI_INPORT(int16_T, 0);
	}
	break;
	case SS_UINT16:   /* uint16_T  */
	{
		FMI_INPORT(uint16_T, 0);
	}
	break;
	case SS_INT32:    /* int32_T   */
	{
		FMI_INPORT(int32_T, 0);
	}
	break;
	case SS_UINT32:   /* uint32_T  */
	{
		FMI_INPORT(uint32_T, 0);
	}
	break;
	case SS_BOOLEAN:  /* boolean_T */
	{
		FMI_INPORT(boolean_T, false);
	}
	break;
	default:		/* default real_T */
	{
		FMI_INPORT(real_T, 0.0);
	}
	break;
	}

	return 1;
};

/* SimStruct callback functions to setup dimensions and allocate ports */

static int_T SetOutputPortDimensionInfoFcn_FMI(SimStruct *arg1, int_T port)
{
	void*  busOutputVector;
	int_T width = arg1->portInfo.outputs[port].width;

	/* Attempt allocating bus vector for port */
	busOutputVector = sfcn_fmi_allocateBusObject(0, port, width);
	if (busOutputVector != 0) {
		arg1->portInfo.outputs[port].signalVect = busOutputVector;
		return 1;
	}

	/* Allocate port signal vector */
	switch (arg1->portInfo.outputs[port].dataTypeId) {
	case SS_DOUBLE:   /* real_T    */
		arg1->portInfo.outputs[port].signalVect = (real_T*)allocateMemory0(width, sizeof(real_T));
		break;
	case SS_SINGLE:   /* real32_T  */
		arg1->portInfo.outputs[port].signalVect = (real32_T*)allocateMemory0(width, sizeof(real32_T));
		break;
	case SS_INTEGER:  /* int_T */
		arg1->portInfo.outputs[port].signalVect = (int_T*)allocateMemory0(width, sizeof(int_T));
		break;
	case SS_INT8:     /* int8_T    */
		arg1->portInfo.outputs[port].signalVect = (int8_T*)allocateMemory0(width, sizeof(int8_T));
		break;
	case SS_UINT8:    /* uint8_T   */
		arg1->portInfo.outputs[port].signalVect = (uint8_T*)allocateMemory0(width, sizeof(uint8_T));
		break;
	case SS_INT16:    /* int16_T   */
		arg1->portInfo.outputs[port].signalVect = (int16_T*)allocateMemory0(width, sizeof(int16_T));
		break;
	case SS_UINT16:   /* uint16_T  */
		arg1->portInfo.outputs[port].signalVect = (uint16_T*)allocateMemory0(width, sizeof(uint16_T));
		break;
	case SS_INT32:    /* int32_T   */
		arg1->portInfo.outputs[port].signalVect = (int32_T*)allocateMemory0(width, sizeof(int32_T));
		break;
	case SS_UINT32:   /* uint32_T  */
		arg1->portInfo.outputs[port].signalVect = (uint32_T*)allocateMemory0(width, sizeof(uint32_T));
		break;
	case SS_BOOLEAN:  /* boolean_T */
		arg1->portInfo.outputs[port].signalVect = (boolean_T*)allocateMemory0(width, sizeof(boolean_T));
		break;
	default:
		arg1->portInfo.outputs[port].signalVect = (real_T*)allocateMemory0(width, sizeof(real_T));
		break;
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

/* Macro to free allocated memory */
#define sfcn_fmi_FREE(ptr, freeFcn)                   \
	if((ptr) != NULL) {\
	   freeFcn((void *)(ptr));\
       (ptr) = NULL;\
    }

void FreeSimStruct(SimStruct *S, FreeMemoryCallback freeMemory) {
	int_T port, i;
	void** inputPtrs;
	void* inputSignals;

	if (S != NULL) {

		for (port = 0; port<S->sizes.in.numInputPorts; port++) {
			inputPtrs = (void**)S->portInfo.inputs[port].signal.ptrs;
			if (inputPtrs != NULL) {
				inputSignals = inputPtrs[0];
				sfcn_fmi_FREE(inputSignals, freeMemory);
				freeMemory(inputPtrs);
				inputPtrs = NULL;
			}
		}
		sfcn_fmi_FREE(S->portInfo.inputs, freeMemory);

		for (port = 0; port<S->sizes.out.numOutputPorts; port++) {
			sfcn_fmi_FREE(S->portInfo.outputs[port].signalVect, freeMemory);
		}
		sfcn_fmi_FREE(S->portInfo.outputs, freeMemory);

		sfcn_fmi_FREE(S->states.contStates, freeMemory);
		/* S->states.dX changed and deallocated by rt_DestroyIntegrationData */
		sfcn_fmi_FREE(S->states.contStateDisabled, freeMemory);
		sfcn_fmi_FREE(S->states.discStates, freeMemory);
		sfcn_fmi_FREE(S->stInfo.sampleTimes, freeMemory);
		sfcn_fmi_FREE(S->stInfo.offsetTimes, freeMemory);
		sfcn_fmi_FREE(S->stInfo.sampleTimeTaskIDs, freeMemory);
		sfcn_fmi_FREE(S->work.modeVector, freeMemory);
		sfcn_fmi_FREE(S->work.iWork, freeMemory);
		sfcn_fmi_FREE(S->work.pWork, freeMemory);
		sfcn_fmi_FREE(S->work.rWork, freeMemory);
		for (i = 0; i<S->sizes.numDWork; i++) {
			sfcn_fmi_FREE(S->work.dWork.sfcn[i].array, freeMemory);
		}
		sfcn_fmi_FREE(S->work.dWork.sfcn, freeMemory);
		sfcn_fmi_mxGlobalTunable_(S, 0, 0);
		sfcn_fmi_FREE(S->sfcnParams.dlgParams, freeMemory);

#if defined(MATLAB_R2011a_) || defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
		sfcn_fmi_FREE(S->states.statesInfo2->absTol, freeMemory);
		sfcn_fmi_FREE(S->states.statesInfo2->absTolControl, freeMemory);
#if defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
		sfcn_fmi_FREE(S->states.statesInfo2->periodicStatesInfo, freeMemory);
#endif
		sfcn_fmi_FREE(S->states.statesInfo2, freeMemory);
#endif

		if (S->mdlInfo != NULL) {
			if (S->mdlInfo->dataTypeAccess != NULL) {
				sfcn_fmi_FREE(S->mdlInfo->dataTypeAccess->dataTypeTable, freeMemory);
				freeMemory(S->mdlInfo->dataTypeAccess);
				S->mdlInfo->dataTypeAccess = NULL;
			}
			sfcn_fmi_FREE(S->mdlInfo->solverInfo->zcSignalVector, freeMemory);
			sfcn_fmi_FREE(S->mdlInfo->sampleHits, freeMemory);
			sfcn_fmi_FREE(S->mdlInfo->t, freeMemory);
			rt_DestroyIntegrationData(S); /* Clear solver data */
			freeMemory(S->mdlInfo);
			S->mdlInfo = NULL;
		}

		freeMemory(S);
		S = NULL;
	}
}

void resetSimStructVectors(SimStruct *S) {
	int_T i;

	memset(S->states.contStates, 0, (S->sizes.numContStates + 1) * sizeof(real_T));
	memset(S->states.dX, 0, (S->sizes.numContStates + 1) * sizeof(real_T));
	memset(S->states.contStateDisabled, 0, (S->sizes.numContStates + 1) * sizeof(boolean_T));
	memset(S->states.discStates, 0, (S->sizes.numDiscStates + 1) * sizeof(real_T));
	memset(S->stInfo.sampleTimes, 0, (S->sizes.numSampleTimes + 1) * sizeof(time_T));
	memset(S->stInfo.offsetTimes, 0, (S->sizes.numSampleTimes + 1) * sizeof(time_T));
	memset(S->stInfo.sampleTimeTaskIDs, 0, (S->sizes.numSampleTimes + 1) * sizeof(int_T));
	memset(S->mdlInfo->sampleHits, 0, (S->sizes.numSampleTimes*S->sizes.numSampleTimes + 1) * sizeof(int_T));
	memset(S->mdlInfo->t, 0, (S->sizes.numSampleTimes + 1) * sizeof(time_T));
	memset(S->work.modeVector, 0, (S->sizes.numModes + 1) * sizeof(int_T));
	memset(S->work.iWork, 0, (S->sizes.numIWork + 1) * sizeof(int_T));
	memset(S->work.pWork, 0, (S->sizes.numPWork + 1) * sizeof(void*));
	memset(S->work.rWork, 0, (S->sizes.numRWork + 1) * sizeof(real_T));
	memset(S->mdlInfo->solverInfo->zcSignalVector, 0, (SFCN_FMI_ZC_LENGTH + 1) * sizeof(real_T));
	for (i = 0; i<S->sizes.numDWork; i++) {
		switch (S->work.dWork.sfcn[i].dataTypeId) {
		case SS_DOUBLE:   /* real_T    */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(real_T));
			break;
		case SS_SINGLE:   /* real32_T  */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(real32_T));
			break;
		case SS_INTEGER:  /* int_T */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(int_T));
			break;
		case SS_INT8:     /* int8_T    */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(int8_T));
			break;
		case SS_UINT8:    /* uint8_T   */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(uint8_T));
			break;
		case SS_INT16:    /* int16_T   */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(int16_T));
			break;
		case SS_UINT16:   /* uint16_T  */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(uint16_T));
			break;
		case SS_INT32:    /* int32_T   */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(int32_T));
			break;
		case SS_UINT32:   /* uint32_T  */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(uint32_T));
			break;
		case SS_BOOLEAN:  /* boolean_T */
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(boolean_T));
			break;
		default:
			memset(S->work.dWork.sfcn[i].array, 0, (S->work.dWork.sfcn[i].width) * sizeof(real_T));
			break;
		}
	}
}

void allocateSimStructVectors(Model* m) {
	int_T i;
	SimStruct* S = m->S;

	S->states.contStates = (real_T*)allocateMemory0(S->sizes.numContStates + 1, sizeof(real_T));
	S->states.dX = (real_T*)allocateMemory0(S->sizes.numContStates + 1, sizeof(real_T));
	/* Store pointer, since it will be changed to point to ODE integration data */
	m->dX = S->states.dX;
	S->states.contStateDisabled = (boolean_T*)allocateMemory0(S->sizes.numContStates + 1, sizeof(boolean_T));
	S->states.discStates = (real_T*)allocateMemory0(S->sizes.numDiscStates + 1, sizeof(real_T));
#if defined(MATLAB_R2011a_) || defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
	S->states.statesInfo2->absTol = (real_T*)allocateMemory0(S->sizes.numContStates + 1, sizeof(real_T));
	S->states.statesInfo2->absTolControl = (uint8_T*)allocateMemory0(S->sizes.numContStates + 1, sizeof(uint8_T));
#endif
	S->stInfo.sampleTimes = (time_T*)allocateMemory0(S->sizes.numSampleTimes + 1, sizeof(time_T));
	S->stInfo.offsetTimes = (time_T*)allocateMemory0(S->sizes.numSampleTimes + 1, sizeof(time_T));
	S->stInfo.sampleTimeTaskIDs = (int_T*)allocateMemory0(S->sizes.numSampleTimes + 1, sizeof(int_T));
	/* Allocating per-task sample hit matrix */
	S->mdlInfo->sampleHits = (int_T*)allocateMemory0(S->sizes.numSampleTimes*S->sizes.numSampleTimes + 1, sizeof(int_T));
	S->mdlInfo->perTaskSampleHits = S->mdlInfo->sampleHits;
	S->mdlInfo->t = (time_T*)allocateMemory0(S->sizes.numSampleTimes + 1, sizeof(time_T));
	S->work.modeVector = (int_T*)allocateMemory0(S->sizes.numModes + 1, sizeof(int_T));
	S->work.iWork = (int_T*)allocateMemory0(S->sizes.numIWork + 1, sizeof(int_T));
	S->work.pWork = (void**)allocateMemory0(S->sizes.numPWork + 1, sizeof(void*));
	S->work.rWork = (real_T*)allocateMemory0(S->sizes.numRWork + 1, sizeof(real_T));
	for (i = 0; i<S->sizes.in.numInputPorts; i++) {
		SetInputPortDimensionInfoFcn_FMI(S, i);
	}
	for (i = 0; i<S->sizes.out.numOutputPorts; i++) {
		SetOutputPortDimensionInfoFcn_FMI(S, i);
	}
	for (i = 0; i<S->sizes.numDWork; i++) {
		switch (S->work.dWork.sfcn[i].dataTypeId) {
		case SS_DOUBLE:   /* real_T    */
			S->work.dWork.sfcn[i].array = (real_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(real_T));
			break;
		case SS_SINGLE:   /* real32_T  */
			S->work.dWork.sfcn[i].array = (real32_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(real32_T));
			break;
		case SS_INTEGER:  /* int_T */
			S->work.dWork.sfcn[i].array = (int_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(int_T));
			break;
		case SS_INT8:     /* int8_T    */
			S->work.dWork.sfcn[i].array = (int8_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(int8_T));
			break;
		case SS_UINT8:    /* uint8_T   */
			S->work.dWork.sfcn[i].array = (uint8_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(uint8_T));
			break;
		case SS_INT16:    /* int16_T   */
			S->work.dWork.sfcn[i].array = (int16_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(int16_T));
			break;
		case SS_UINT16:   /* uint16_T  */
			S->work.dWork.sfcn[i].array = (uint16_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(uint16_T));
			break;
		case SS_INT32:    /* int32_T   */
			S->work.dWork.sfcn[i].array = (int32_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(int32_T));
			break;
		case SS_UINT32:   /* uint32_T  */
			S->work.dWork.sfcn[i].array = (uint32_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(uint32_T));
			break;
		case SS_BOOLEAN:  /* boolean_T */
			S->work.dWork.sfcn[i].array = (boolean_T*)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(boolean_T));
			break;
		case SS_POINTER:  /* void* */
			S->work.dWork.sfcn[i].array = (void**)allocateMemory0(S->work.dWork.sfcn[i].width, sizeof(void*));
			break;
		default:  /* Custom data type registered */
			S->work.dWork.sfcn[i].array = (void*)allocateMemory0(S->work.dWork.sfcn[i].width, ((int_T*)(S->mdlInfo->dataTypeAccess->dataTypeTable))[S->work.dWork.sfcn[i].dataTypeId - 15]);
			break;
		}
	}
}

void setSampleStartValues(Model* m)
{
	int i;

	m->fixed_in_minor_step_offset_tid = -1;
	for (i = 0; i<m->S->sizes.numSampleTimes; i++) {
		m->S->stInfo.sampleTimeTaskIDs[i] = i; /* Simple mapping, only considering root SimStruct for the moment */
		m->S->mdlInfo->t[i] = m->S->stInfo.offsetTimes[i];
		if (m->S->stInfo.sampleTimes[i] < SFCN_FMI_EPS) {
			if (m->S->stInfo.offsetTimes[i] < FIXED_IN_MINOR_STEP_OFFSET - SFCN_FMI_EPS) {
				m->S->mdlInfo->sampleHits[i] = 1;		/* Continuous sample time */
			}
			else {
				m->fixed_in_minor_step_offset_tid = i;	/* FIXED_IN_MINOR_STEP_OFFSET */
			}
		}
		else {
			if (i == 0) {
				m->isDiscrete = 1;  /* Purely discrete */
			}
		}
	}
	if (SFCN_FMI_LOAD_MEX) {
		copyPerTaskSampleHits(m->S);
	}
}

/* Setup of port dimensions in stand-alone mode (not including simulink.c) */

int_T _ssSetInputPortMatrixDimensions_FMI(SimStruct *S, int_T port, int_T m, int_T n)
{
	S->portInfo.inputs[port].width = ((m == DYNAMICALLY_SIZED) || (n == DYNAMICALLY_SIZED)) ? DYNAMICALLY_SIZED : (m * n);

	return(1);
}

int_T _ssSetOutputPortMatrixDimensions_FMI(SimStruct *S, int_T port, int_T m, int_T n)
{
	S->portInfo.outputs[port].width = ((m == DYNAMICALLY_SIZED) || (n == DYNAMICALLY_SIZED)) ? DYNAMICALLY_SIZED : (m * n);

	return(1);
}

int_T _ssSetInputPortVectorDimension_FMI(SimStruct *S, int_T port, int_T m)
{
	S->portInfo.inputs[port].width = m;

	return(1);
}

int_T _ssSetOutputPortVectorDimension_FMI(SimStruct *S, int_T port, int_T m)
{
	S->portInfo.outputs[port].width = m;

	return(1);
}

