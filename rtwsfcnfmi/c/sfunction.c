#include "sfcn_fmi_rel_conf.h"
#include "sfcn_fmi.h"
#include "sfunction.h"

Model* currentModel = NULL;

const char* _SFCN_FMI_MATLAB_BIN = NULL;

static int LoadMEXAndDependencies(Model *model);

void copyPerTaskSampleHits(SimStruct* S) {
	int_T m, n;

	for (m = 1; m < S->sizes.numSampleTimes; m++) {
		for (n = 0; n < S->sizes.numSampleTimes; n++) {
			S->mdlInfo->perTaskSampleHits[n + m * (S->sizes.numSampleTimes)] = S->mdlInfo->sampleHits[n];
		}
	}
}

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
			(struct _ssPortInputs*) calloc((size_t)nInputPorts,
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
			(struct _ssPortOutputs*) calloc((size_t)nOutputPorts,
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
	S->work.dWork.sfcn = (struct _ssDWorkRecord*) calloc((size_t)numDWork, sizeof(struct _ssDWorkRecord));
	S->sizes.numDWork = numDWork;

	if (S->mdlInfo != NULL) {
		S->mdlInfo->dataTypeAccess = (slDataTypeAccess*)calloc(1, sizeof(slDataTypeAccess));
		S->mdlInfo->dataTypeAccess->dataTypeTable = (int_T*)calloc((size_t)numDWork, sizeof(int_T));
	}
	return 1;
}

/* Common macro to allocate input port */
#define FMI_INPORT(type, value) {										            \
	int i;																			\
	type** inputPtrs;																\
	type* inputSignals;																\
	inputPtrs    = (type**) calloc(width, sizeof(type*));					\
	inputSignals = (type*) calloc(width, sizeof(type));					\
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
		busInputPtrs = (void**)calloc(width, sizeof(void*));
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
		arg1->portInfo.outputs[port].signalVect = (real_T*)calloc(width, sizeof(real_T));
		break;
	case SS_SINGLE:   /* real32_T  */
		arg1->portInfo.outputs[port].signalVect = (real32_T*)calloc(width, sizeof(real32_T));
		break;
	case SS_INTEGER:  /* int_T */
		arg1->portInfo.outputs[port].signalVect = (int_T*)calloc(width, sizeof(int_T));
		break;
	case SS_INT8:     /* int8_T    */
		arg1->portInfo.outputs[port].signalVect = (int8_T*)calloc(width, sizeof(int8_T));
		break;
	case SS_UINT8:    /* uint8_T   */
		arg1->portInfo.outputs[port].signalVect = (uint8_T*)calloc(width, sizeof(uint8_T));
		break;
	case SS_INT16:    /* int16_T   */
		arg1->portInfo.outputs[port].signalVect = (int16_T*)calloc(width, sizeof(int16_T));
		break;
	case SS_UINT16:   /* uint16_T  */
		arg1->portInfo.outputs[port].signalVect = (uint16_T*)calloc(width, sizeof(uint16_T));
		break;
	case SS_INT32:    /* int32_T   */
		arg1->portInfo.outputs[port].signalVect = (int32_T*)calloc(width, sizeof(int32_T));
		break;
	case SS_UINT32:   /* uint32_T  */
		arg1->portInfo.outputs[port].signalVect = (uint32_T*)calloc(width, sizeof(uint32_T));
		break;
	case SS_BOOLEAN:  /* boolean_T */
		arg1->portInfo.outputs[port].signalVect = (boolean_T*)calloc(width, sizeof(boolean_T));
		break;
	default:
		arg1->portInfo.outputs[port].signalVect = (real_T*)calloc(width, sizeof(real_T));
		break;
	}

	return 1;
}

Model *InstantiateModel(const char* instanceName, logMessageCallback logMessage, void *userData) {

	Model* model = (Model*)calloc(1, sizeof(Model));

	if (model == NULL) {
		goto fail;
	}

	model->logMessage = logMessage;
	model->userData = userData;

	/* The following arguments are ignored: fmuResourceLocation, visible */

	model->instanceName = strdup(instanceName);

	if (model->instanceName == NULL) {
		goto fail;
	}

	model->loggingOn = 0;
	model->shouldRecompute = 0;
	model->time = 0.0;
	model->nbrSolverSteps = 0.0;
	model->isDiscrete = 0;

	if (SFCN_FMI_LOAD_MEX) {
#if defined(_MSC_VER)
		model->mexHandles = (HINSTANCE*)calloc(SFCN_FMI_NBR_MEX + 1, sizeof(HINSTANCE));
#else
		model->mexHandles = (void**)calloc(SFCN_FMI_NBR_MEX + 1, sizeof(void*));
#endif
		/* Handle loading of MATLAB binaries and binary MEX S-functions */
		if (!LoadMEXAndDependencies(model)) {
			goto fail;
		}
#if defined(_MSC_VER)
		SetDllDirectory(0);
#endif
	}
	if (SFCN_FMI_NBR_MEX > 0) {
		currentModel = model;
	}

	model->S = CreateSimStructForFMI(model->instanceName);
	if (model->S == NULL) {
		goto fail;
	}

	/* Register model callback functions in Simstruct */
	sfcn_fmi_registerMdlCallbacks_(model->S);

	/* Initialize sizes and create vectors */
	sfcnInitializeSizes(model->S);
	allocateSimStructVectors(model);

	/* Create solver data and ZC vector */
	rt_CreateIntegrationData(model->S);
	model->S->mdlInfo->solverInfo->zcSignalVector = (real_T*)calloc(SFCN_FMI_ZC_LENGTH + 1, sizeof(real_T));
	model->S->states.nonsampledZCs = model->S->mdlInfo->solverInfo->zcSignalVector;
	/* Register model callback for ODE solver */
	sfcn_fmi_registerRTModelCallbacks_(model->S);

	/* Initialize sample times and sample flags */
	sfcnInitializeSampleTimes(model->S);
	setSampleStartValues(model);

	/* non-finites */
	rt_InitInfAndNaN(sizeof(real_T));
	/* Create and initialize global tunable parameters */
	sfcn_fmi_mxGlobalTunable_(model->S, 1, 0);
	/* Call mdlStart */
	if (ssGetmdlStart(model->S) != NULL) {
		sfcnStart(model->S);
	}

	/* Allocate model vectors */
	model->oldZC = (real_T*)calloc(SFCN_FMI_ZC_LENGTH + 1, sizeof(real_T));
	model->numSampleHits = (int_T*)calloc(model->S->sizes.numSampleTimes + 1, sizeof(int_T));
	model->inputs = (void**)calloc(SFCN_FMI_NBR_INPUTS + 1, sizeof(void*));
	model->outputs = (void**)calloc(SFCN_FMI_NBR_OUTPUTS + 1, sizeof(void*));
	model->parameters = (void**)calloc(SFCN_FMI_NBR_PARAMS + 1, sizeof(void*));
	model->blockoutputs = (void**)calloc(SFCN_FMI_NBR_BLOCKIO + 1, sizeof(void*));
	model->dwork = (void**)calloc(SFCN_FMI_NBR_DWORK + 1, sizeof(void*));
	model->inputDerivatives = (real_T*)calloc(SFCN_FMI_NBR_INPUTS + 1, sizeof(real_T));

	/* Assign variable pointers for use in fmiSetReal and fmiGetReal */
	sfcn_fmi_assignInputs_(model->S, model->inputs);
	sfcn_fmi_assignOutputs_(model->S, model->outputs);
	sfcn_fmi_assignParameters_(model->S, model->parameters);	/* Also allocates parameter struct for model instance and stores in UserData */
	sfcn_fmi_assignBlockOutputs_(model->S, model->blockoutputs);
	sfcn_fmi_assignDWork_(model->S, model->dwork);

	/* Check Simstruct error status and stop requested */
	if ((ssGetErrorStatus(model->S) != NULL) || (ssGetStopRequested(model->S) != 0)) {
		goto fail;
	}

	model->status = modelInstantiated;

	logMessage(model, 0, "Instantiation succeeded");
	return model;

fail:
	if (model != NULL) {
		const char * iName = instanceName != NULL ? instanceName : "";
		if (model->S != NULL) {
			if ((ssGetErrorStatus(model->S) != NULL) || (ssGetStopRequested(model->S) != 0)) {
				if (ssGetStopRequested(model->S) != 0) {
					logMessage(model, 3, "Stop requested by S-function!");
				}
				if (ssGetErrorStatus(model->S) != NULL) {
					logMessage(model, 3,"Error reported by S-function: %s", ssGetErrorStatus(model->S));
				}
				FreeModel(model);
				return NULL;
			}
		}
		logMessage(model, 4, "Instantiation failed due to problems with memory allocation or dynamic loading.");
		FreeModel(model);
	}
	return NULL;
}


SimStruct *CreateSimStructForFMI(const char* instanceName)
{
	SimStruct *S = (SimStruct*)calloc(1, sizeof(SimStruct));
	if (S == NULL) {
		return NULL;
	}
	S->mdlInfo = (struct _ssMdlInfo*)calloc(1, sizeof(struct _ssMdlInfo));
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
	S->states.statesInfo2 = (struct _ssStatesInfo2 *) calloc(1, sizeof(struct _ssStatesInfo2));
#if defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
	S->states.statesInfo2->periodicStatesInfo = (ssPeriodicStatesInfo *)calloc(1, sizeof(ssPeriodicStatesInfo));
#endif
#endif

	return(S);
}

void FreeSimStruct(SimStruct *S) {
	int_T port, i;
	void** inputPtrs;
	void* inputSignals;

	if (S != NULL) {

		for (port = 0; port<S->sizes.in.numInputPorts; port++) {
			inputPtrs = (void**)S->portInfo.inputs[port].signal.ptrs;
			if (inputPtrs != NULL) {
				inputSignals = inputPtrs[0];
				free(inputSignals);
				free(inputPtrs);
				inputPtrs = NULL;
			}
		}
		free(S->portInfo.inputs);

		for (port = 0; port<S->sizes.out.numOutputPorts; port++) {
			free(S->portInfo.outputs[port].signalVect);
		}
		free(S->portInfo.outputs);

		free(S->states.contStates);
		/* S->states.dX changed and deallocated by rt_DestroyIntegrationData */
		free(S->states.contStateDisabled);
		free(S->states.discStates);
		free(S->stInfo.sampleTimes);
		free(S->stInfo.offsetTimes);
		free(S->stInfo.sampleTimeTaskIDs);
		free(S->work.modeVector);
		free(S->work.iWork);
		free(S->work.pWork);
		free(S->work.rWork);
		for (i = 0; i<S->sizes.numDWork; i++) {
			free(S->work.dWork.sfcn[i].array);
		}
		free(S->work.dWork.sfcn);
		sfcn_fmi_mxGlobalTunable_(S, 0, 0);
		free(S->sfcnParams.dlgParams);

#if defined(MATLAB_R2011a_) || defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
		free(S->states.statesInfo2->absTol);
		free(S->states.statesInfo2->absTolControl);
#if defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
		free(S->states.statesInfo2->periodicStatesInfo);
#endif
		free(S->states.statesInfo2);
#endif

		if (S->mdlInfo != NULL) {
			if (S->mdlInfo->dataTypeAccess != NULL) {
				free(S->mdlInfo->dataTypeAccess->dataTypeTable);
				free(S->mdlInfo->dataTypeAccess);
				S->mdlInfo->dataTypeAccess = NULL;
			}
			free(S->mdlInfo->solverInfo->zcSignalVector);
			free(S->mdlInfo->sampleHits);
			free(S->mdlInfo->t);
			rt_DestroyIntegrationData(S); /* Clear solver data */
			free(S->mdlInfo);
			S->mdlInfo = NULL;
		}

		free(S);
		S = NULL;
	}
}

void FreeModel(Model* model) {
    
    void* paramP;
    int i;

    if (model == NULL) {
        return;
    }

    assert(model->instanceName != NULL);

//    logger(model, model->instanceName, fmi2OK, "", "Freeing instance\n");

    if (model->S != NULL) {
        if (ssGetUserData(model->S) != NULL ) {
            if (SFCN_FMI_NBR_PARAMS > 0) {
                /* Free dynamically allocated parameters for this instance */
                paramP = sfcn_fmi_getParametersP_(model->S);
                free(paramP);
            }
        }
        /* Call mdlTerminate here, since that clears S-function Userdata */
        sfcnTerminate(model->S);
    }

//    UserData *userData = (UserData*)model->userData;

    if (SFCN_FMI_LOAD_MEX) {
        for (i=0; i<SFCN_FMI_NBR_MEX; i++) {
#if defined(_MSC_VER)
            FreeLibrary(model->mexHandles[i]);
#else
            dlclose(model->mexHandles[i]);
#endif
        }
#if defined(_MSC_VER)
        SetDllDirectory(0);
#endif
        free((void *)_SFCN_FMI_MATLAB_BIN);
    }

    FreeSimStruct(model->S);
    free((void *)model->instanceName);
    free(model->dX);
    free(model->oldZC);
    free(model->numSampleHits);
    free(model->inputs);
    free(model->outputs);
    free(model->parameters);
    free(model->blockoutputs);
    free(model->dwork);
    free(model->mexHandles);
    free(model->inputDerivatives);
    free(model);
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

	S->states.contStates = (real_T*)calloc(S->sizes.numContStates + 1, sizeof(real_T));
	S->states.dX = (real_T*)calloc(S->sizes.numContStates + 1, sizeof(real_T));
	/* Store pointer, since it will be changed to point to ODE integration data */
	m->dX = S->states.dX;
	S->states.contStateDisabled = (boolean_T*)calloc(S->sizes.numContStates + 1, sizeof(boolean_T));
	S->states.discStates = (real_T*)calloc(S->sizes.numDiscStates + 1, sizeof(real_T));
#if defined(MATLAB_R2011a_) || defined(MATLAB_R2015a_) || defined(MATLAB_R2017b_)
	S->states.statesInfo2->absTol = (real_T*)calloc(S->sizes.numContStates + 1, sizeof(real_T));
	S->states.statesInfo2->absTolControl = (uint8_T*)calloc(S->sizes.numContStates + 1, sizeof(uint8_T));
#endif
	S->stInfo.sampleTimes = (time_T*)calloc(S->sizes.numSampleTimes + 1, sizeof(time_T));
	S->stInfo.offsetTimes = (time_T*)calloc(S->sizes.numSampleTimes + 1, sizeof(time_T));
	S->stInfo.sampleTimeTaskIDs = (int_T*)calloc(S->sizes.numSampleTimes + 1, sizeof(int_T));
	/* Allocating per-task sample hit matrix */
	S->mdlInfo->sampleHits = (int_T*)calloc(S->sizes.numSampleTimes*S->sizes.numSampleTimes + 1, sizeof(int_T));
	S->mdlInfo->perTaskSampleHits = S->mdlInfo->sampleHits;
	S->mdlInfo->t = (time_T*)calloc(S->sizes.numSampleTimes + 1, sizeof(time_T));
	S->work.modeVector = (int_T*)calloc(S->sizes.numModes + 1, sizeof(int_T));
	S->work.iWork = (int_T*)calloc(S->sizes.numIWork + 1, sizeof(int_T));
	S->work.pWork = (void**)calloc(S->sizes.numPWork + 1, sizeof(void*));
	S->work.rWork = (real_T*)calloc(S->sizes.numRWork + 1, sizeof(real_T));
	for (i = 0; i<S->sizes.in.numInputPorts; i++) {
		SetInputPortDimensionInfoFcn_FMI(S, i);
	}
	for (i = 0; i<S->sizes.out.numOutputPorts; i++) {
		SetOutputPortDimensionInfoFcn_FMI(S, i);
	}
	for (i = 0; i<S->sizes.numDWork; i++) {
		switch (S->work.dWork.sfcn[i].dataTypeId) {
		case SS_DOUBLE:   /* real_T    */
			S->work.dWork.sfcn[i].array = (real_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(real_T));
			break;
		case SS_SINGLE:   /* real32_T  */
			S->work.dWork.sfcn[i].array = (real32_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(real32_T));
			break;
		case SS_INTEGER:  /* int_T */
			S->work.dWork.sfcn[i].array = (int_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(int_T));
			break;
		case SS_INT8:     /* int8_T    */
			S->work.dWork.sfcn[i].array = (int8_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(int8_T));
			break;
		case SS_UINT8:    /* uint8_T   */
			S->work.dWork.sfcn[i].array = (uint8_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(uint8_T));
			break;
		case SS_INT16:    /* int16_T   */
			S->work.dWork.sfcn[i].array = (int16_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(int16_T));
			break;
		case SS_UINT16:   /* uint16_T  */
			S->work.dWork.sfcn[i].array = (uint16_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(uint16_T));
			break;
		case SS_INT32:    /* int32_T   */
			S->work.dWork.sfcn[i].array = (int32_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(int32_T));
			break;
		case SS_UINT32:   /* uint32_T  */
			S->work.dWork.sfcn[i].array = (uint32_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(uint32_T));
			break;
		case SS_BOOLEAN:  /* boolean_T */
			S->work.dWork.sfcn[i].array = (boolean_T*)calloc(S->work.dWork.sfcn[i].width, sizeof(boolean_T));
			break;
		case SS_POINTER:  /* void* */
			S->work.dWork.sfcn[i].array = (void**)calloc(S->work.dWork.sfcn[i].width, sizeof(void*));
			break;
		default:  /* Custom data type registered */
			S->work.dWork.sfcn[i].array = (void*)calloc(S->work.dWork.sfcn[i].width, ((int_T*)(S->mdlInfo->dataTypeAccess->dataTypeTable))[S->work.dWork.sfcn[i].dataTypeId - 15]);
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

#ifdef __APPLE__

static int LoadMEXAndDependencies(Model *model) {
    return 0;  // not implemented
}

#else

/* Dynamic loading of MATLAB MEX files for S-function blocks */
static int LoadMEXAndDependencies(Model *model)
{
#if defined(_MSC_VER)
    HINSTANCE hInst;
    HMODULE hMySelf=0;
#else
    Dl_info dli;
    struct stat sb;
    void* hInst = NULL;
#endif
    int i;
    char fmuPath[1024];
    char*last;
    char*mexDir;
    char mexFile[1024];

#if defined(_MSC_VER)
    /* Setting MATLAB bin directory and loading MATLAB dependencies.
        Not done on Linux, there the MATLAB bin needs to be set with LD_LIBRARY_PATH */
    if ( (getenv("SFCN_FMI_MATLAB_BIN") != NULL) && (_SFCN_FMI_MATLAB_BIN == NULL) ) {
        UserData *userData = (UserData *)model->userData;
        _SFCN_FMI_MATLAB_BIN = strDup(&(userData->functions), getenv("SFCN_FMI_MATLAB_BIN"));
    }
    if (_SFCN_FMI_MATLAB_BIN == NULL) {
        SetDllDirectory(SFCN_FMI_MATLAB_BIN);
        logger(model, model->instanceName, fmi2OK, "", "Setting DLL directory for MATLAB dependencies: %s", SFCN_FMI_MATLAB_BIN);
        logger(model, model->instanceName, fmi2OK, "", "The environment variable SFCN_FMI_MATLAB_BIN can be used to override this path.");
    } else {
        SetDllDirectory(_SFCN_FMI_MATLAB_BIN);
        logger(model, model->instanceName, fmi2OK, "", "Setting DLL directory for MATLAB dependencies: %s", _SFCN_FMI_MATLAB_BIN);
        logger(model, model->instanceName, fmi2OK, "", "Environment variable SFCN_FMI_MATLAB_BIN was used to override default path.");
    }
    logger(model, model->instanceName, fmi2OK, "", "Loading from MATLAB bin...");
    if (hInst=LoadLibraryA("libmx.dll")) {
        logger(model, model->instanceName, fmi2OK, "", "...libmx.dll");
    } else  {
        logger(model, model->instanceName, fmi2Error, "", "Failed to load binary libmx.dll\n");
        return 0;
    }
    if (hInst=LoadLibraryA("libmex.dll")) {
        logger(model, model->instanceName, fmi2OK, "", "...libmex.dll");
    } else  {
        logger(model, model->instanceName, fmi2Error, "", "Failed to load binary libmex.dll\n");
        return 0;
    }
    if (hInst=LoadLibraryA("libmat.dll")) {
        logger(model, model->instanceName, fmi2OK, "", "...libmat.dll");
    } else  {
        logger(model, model->instanceName, fmi2Error, "", "Failed to load binary libmat.dll\n");
        return 0;
    }
    if (hInst=LoadLibraryA("libfixedpoint.dll")) {
        logger(model, model->instanceName, fmi2OK, "", "...libfixedpoint.dll");
    } else  {
        logger(model, model->instanceName, fmi2Error, "", "Failed to load binary libfixedpoint.dll\n");
        return 0;
    }
    if (hInst=LoadLibraryA("libut.dll")) {
        logger(model, model->instanceName, fmi2OK, "", "...libut.dll");
    } else  {
        logger(model, model->instanceName, fmi2Error, "", "Failed to load binary libut.dll\n");
        return 0;
    }
    hMySelf=GetModuleHandleA(SFCN_FMI_MODEL_IDENTIFIER);
    if (GetModuleFileNameA(hMySelf, fmuPath, sizeof(fmuPath)/sizeof(*fmuPath))==0) {
#else
    if (dladdr(__builtin_return_address(0), &dli) != 0 && dli.dli_fname != NULL) {
        strcpy(fmuPath, dli.dli_fname);
    } else {
#endif
        logger(model, model->instanceName, fmi2Fatal, "", "Failed to retrieve module file name for %s\n", SFCN_FMI_MODEL_IDENTIFIER);
        return 0;
    }
    fmuPath[sizeof(fmuPath)/sizeof(*fmuPath)-1]=0; /* Make sure it is NUL-terminated */
    last=strrchr(fmuPath,'\\');
    if (last==0) last=strrchr(fmuPath,'/');
    if (last) {
        last[0]=0;
        last=strrchr(fmuPath,'\\');
        if (last==0) last=strrchr(fmuPath,'/');
        if (last) {
            last[0]=0;
            last=strrchr(fmuPath,'\\');
            if (last==0) last=strrchr(fmuPath,'/');
        }
    }
    if (last) last[1]=0;
#if defined(_MSC_VER)
    mexDir = strcat(fmuPath,"resources\\SFunctions\\");
#else
    mexDir = strcat(fmuPath,"resources/SFunctions/");
#endif
    for (i=0; i<SFCN_FMI_NBR_MEX; i++) {
        if (i==0) {
            logger(model, model->instanceName, fmi2OK, "", "Loading S-function MEX files from FMU resources...");
#if defined(_MSC_VER)
            SetDllDirectory(mexDir); /* To handle dependencies to other DLLs in the same folder */
#endif
        }
        strncpy(mexFile, mexDir, strlen(mexDir)+1);
        strcat(mexFile, SFCN_FMI_MEX_NAMES[i]);
#if defined(_MSC_VER)
        if (hInst=LoadLibraryA(mexFile)) {
#else
        if (stat(mexFile, &sb) == 0) {
            hInst = dlopen(mexFile, RTLD_NOW);
            if (hInst != NULL) {
#endif
                model->mexHandles[i]=hInst;
                logger(model, model->instanceName, fmi2OK, "", "...%s", SFCN_FMI_MEX_NAMES[i]);
            } else  {
                logger(model, model->instanceName, fmi2Error, "", "Failed to load binary MEX file: %s", SFCN_FMI_MEX_NAMES[i]);
                return 0;
            }
#if !defined(_MSC_VER)
        }
#endif
    }
    return 1;
}

typedef void (*mexFunctionPtr) (int_T nlhs, mxArray *plhs[], int_T nrhs, const mxArray *prhs[]);

static int sfcn_fmi_load_mex(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[], const char *functionName, const char* mexext)
{
    int i;
    char mexName[256];
    mexFunctionPtr mexFunc = NULL;
#if defined(_MSC_VER)
    HINSTANCE hInst = 0;
#else
    void* hInst = 0;
#endif

    sprintf(mexName, "%s.%s", functionName, mexext);
    /* Find handle index */
    for (i=0; i<SFCN_FMI_NBR_MEX; i++) {
        if (strcmp(mexName, SFCN_FMI_MEX_NAMES[i]) == 0) {
            break;
        }
    }
    if (i<SFCN_FMI_NBR_MEX) {
        hInst = currentModel->mexHandles[i];
    }
    if (hInst) {
#if defined(_MSC_VER)
        mexFunc = (mexFunctionPtr)GetProcAddress(hInst,"mexFunction");
        if (mexFunc == NULL) {
            mexFunc = (mexFunctionPtr)GetProcAddress(hInst,"_mexFunction");
        }
#else
        mexFunc = (mexFunctionPtr) dlsym(hInst,"mexFunction");
        if (mexFunc == NULL) {
            mexFunc = (mexFunctionPtr) dlsym(hInst,"_mexFunction"); /* probably not needed here, only LCC generates entrypoint _mexFunction */
        }
#endif
        if (mexFunc) {
            logger(currentModel, currentModel->instanceName, fmi2OK, "", "Calling MEX S-function: %s", mexName);
            mexFunc(nlhs, plhs, nrhs, (const mxArray**)prhs);
            if (currentModel->S != NULL) {
                if (ssGetErrorStatus(currentModel->S) != NULL) {
                    logger(currentModel, currentModel->instanceName, fmi2Fatal, "",
                        "Error in S-function (mdlInitializeSizes): %s\n", ssGetErrorStatus(currentModel->S));
                    return 1;
                }
            }
        } else {
            logger(currentModel, currentModel->instanceName, fmi2Fatal, "", "Failed to retrieve 'mexFunction' entry point from %s\n", mexName);
            return 1;
        }
    } else  {
        logger(currentModel, currentModel->instanceName, fmi2Fatal, "", "Failed to retrieve handle to call binary MEX file: %s", mexName);
        return 1;
    }

    return 0;
}

static int sfcn_fmi_load_mexw32(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[], const char *functionName)
{
    return sfcn_fmi_load_mex(nlhs, plhs, nrhs, prhs, functionName, "mexw32");
}

static int sfcn_fmi_load_mexw64(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[], const char *functionName)
{
    return sfcn_fmi_load_mex(nlhs, plhs, nrhs, prhs, functionName, "mexw64");
}

static int sfcn_fmi_load_mexglx(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[], const char *functionName)
{
    return sfcn_fmi_load_mex(nlhs, plhs, nrhs, prhs, functionName, "mexglx");
}

static int sfcn_fmi_load_mexa64(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[], const char *functionName)
{
    return sfcn_fmi_load_mex(nlhs, plhs, nrhs, prhs, functionName, "mexa64");
}


#endif // __APPLE__
