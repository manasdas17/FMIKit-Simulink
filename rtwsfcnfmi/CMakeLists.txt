cmake_minimum_required (VERSION 3.2)

set(MODEL_NAME "" CACHE STRING "Model name")
set(SOLVER "ode1" CACHE STRING "Solver")
set_property(CACHE SOLVER PROPERTY STRINGS ode1 ode2 ode3 ode4 ode5 ode8 ode14x)
set(MATLAB_ROOT "C:/Program Files/MATLAB/R2019b" CACHE STRING "MATLAB install directory")
set(MATLAB_VERSION "9.7" CACHE STRING "MATLAB version for conditional compilation")
set(RTW_DIR "" CACHE STRING "RTW generated model directory")
set(CUSTOM_SOURCE "" CACHE STRING "Additional source files")
set(FMU_BUILD_DIR "${RTW_DIR}/FMUArchive")

project (${MODEL_NAME})

if (WIN32)
  set(FMI_PLATFORM win)
elseif (APPLE)
  set(FMI_PLATFORM darwin)
else ()
  set(FMI_PLATFORM linux)
endif ()

if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
  set (FMI_PLATFORM ${FMI_PLATFORM}64)
else ()
  set (FMI_PLATFORM ${FMI_PLATFORM}32)
endif ()

set(SOLVER_SOURCES "${MATLAB_ROOT}/rtw/c/src/${SOLVER}.c")

if ("${SOLVER}" STREQUAL "ode14x")
  set(SOLVER_SOURCES
    ${SOLVER_SOURCES}
    "${MATLAB_ROOT}/rtw/c/src/matrixmath/rt_backsubrr_dbl.c"
    "${MATLAB_ROOT}/rtw/c/src/matrixmath/rt_forwardsubrr_dbl.c"
    "${MATLAB_ROOT}/rtw/c/src/matrixmath/rt_lu_real.c"
  )
endif ()

add_library(${SOLVER} STATIC ${SOLVER_SOURCES})

target_compile_definitions(${SOLVER} PRIVATE
	_CRT_SECURE_NO_WARNINGS
	NRT
	RT_MALLOC
)

target_include_directories(${SOLVER} PRIVATE
  "${RTW_DIR}"
  "${MATLAB_ROOT}/extern/include"
  "${MATLAB_ROOT}/simulink/include"
  "${MATLAB_ROOT}/rtw/c/src/matrixmath"
  "${MATLAB_ROOT}/rtw/c/src"
)

set(MODEL_SOURCES 
  ../include/fmi2Functions.h
  ../include/fmi2FunctionTypes.h
  ../include/fmi2TypesPlatform.h
  c/fmi2Functions.c
  c/sfunction.h
  c/sfunction.c
  "${RTW_DIR}/model_interface.h"
  "${RTW_DIR}/model_interface.c"
  "${RTW_DIR}/sfcn_fmi.h"
  "${RTW_DIR}/sfcn_fmi.c"
  "${RTW_DIR}/${MODEL_NAME}_sf.h"
  "${RTW_DIR}/${MODEL_NAME}_sf_data.c"
  "${RTW_DIR}/rtGetInf.c"
  "${RTW_DIR}/rtGetNaN.c"
  "${RTW_DIR}/modelDescription.xml"
)

if (EXISTS "${RTW_DIR}/rt_zcfcn.c")
  set (MODEL_SOURCES ${MODEL_SOURCES} "${RTW_DIR}/rt_zcfcn.c")
endif ()

add_library(${MODEL_NAME} SHARED
  ${MODEL_SOURCES}
  ${CUSTOM_SOURCE}
)

target_include_directories(${MODEL_NAME} PRIVATE
  ../include
  c
)

if (WIN32)
  target_link_libraries(${MODEL_NAME}
    "${MATLAB_ROOT}/extern/lib/win64/microsoft/libmx.lib"
    "${MATLAB_ROOT}/extern/lib/win64/microsoft/libmex.lib"
    "${MATLAB_ROOT}/extern/lib/win64/microsoft/sf_runtime.lib"
    ${SOLVER}
  )
elseif (APPLE)
  target_link_libraries(${MODEL_NAME} ${SOLVER})
endif ()

target_compile_definitions(${MODEL_NAME} PRIVATE
  _CRT_SECURE_NO_WARNINGS
  FIPXT_SHARED_MODULE
  rt_matrx_h
  RT_MALLOC
  NDEBUG
  ${MATLAB_VERSION}
)

# don't add the "lib" prefix to the shared library
set_target_properties(${MODEL_NAME} PROPERTIES PREFIX "")

target_include_directories(${MODEL_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${RTW_DIR}"
  "${MATLAB_ROOT}/extern/include"
  "${MATLAB_ROOT}/simulink/include"
  "${MATLAB_ROOT}/rtw/c/src/matrixmath"
  "${MATLAB_ROOT}/rtw/c/src"
)

# modelDescription.xml
add_custom_command(TARGET ${MODEL_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  ${RTW_DIR}/modelDescription.xml
  ${FMU_BUILD_DIR}/modelDescription.xml
)

# shared library
add_custom_command(TARGET ${MODEL_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy
  $<TARGET_FILE:${MODEL_NAME}>
  ${FMU_BUILD_DIR}/binaries/${FMI_PLATFORM}/$<TARGET_FILE_NAME:${MODEL_NAME}>
)

# create ZIP archive
add_custom_command(TARGET ${MODEL_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E tar "cfv" ${RTW_DIR}/${MODEL_NAME}.fmu --format=zip
  modelDescription.xml
  binaries
  WORKING_DIRECTORY ${FMU_BUILD_DIR}
)
