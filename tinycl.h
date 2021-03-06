// Copyright (C) 2017 Mihai Preda.

#include <cstdint>
#include <cstddef>

typedef struct _cl_platform_id *    cl_platform_id;
typedef struct _cl_device_id *      cl_device_id;
typedef struct _cl_context *        cl_context;
typedef struct _cl_command_queue *  cl_command_queue;
typedef struct _cl_mem *            cl_mem;
typedef struct _cl_program *        cl_program;
typedef struct _cl_kernel *         cl_kernel;
typedef struct _cl_event *          cl_event;
typedef struct _cl_sampler *        cl_sampler;

typedef unsigned cl_bool;
typedef unsigned cl_program_build_info;
typedef unsigned cl_program_info;
typedef unsigned cl_device_info;

typedef unsigned long ulong;
typedef ulong cl_mem_flags;
typedef ulong cl_device_type;
typedef ulong cl_queue_properties;

extern "C" {

unsigned clGetPlatformIDs(unsigned, cl_platform_id *, unsigned *);
int clGetDeviceIDs(cl_platform_id, cl_device_type, unsigned,  cl_device_id *, unsigned *);
cl_context clCreateContext(const intptr_t *, unsigned, const cl_device_id *, void (*)(const char *, const void *, size_t, void *), void *, int *);
int clReleaseContext(cl_context);
int clReleaseProgram(cl_program);
int clReleaseCommandQueue(cl_command_queue);
int clEnqueueNDRangeKernel(cl_command_queue, cl_kernel, unsigned, const size_t *, const size_t *, const size_t *, unsigned, const cl_event *, cl_event *);
cl_program clCreateProgramWithSource(cl_context, unsigned, const char **, const size_t *, int *);
int clBuildProgram(cl_program, unsigned, const cl_device_id *, const char *, void (*)(cl_program, void *), void *);
int clGetProgramBuildInfo(cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);
int clGetProgramInfo(cl_program, cl_program_info, size_t, void *, size_t *);
int clGetDeviceInfo(cl_device_id, cl_device_info, size_t, void *, size_t *);
int clGetPlatformInfo(cl_platform_id, cl_device_info, size_t, void *, size_t *);

cl_kernel clCreateKernel(cl_program, const char *, int *);
int clReleaseKernel(cl_kernel);
cl_mem clCreateBuffer(cl_context, cl_mem_flags, size_t, void *, int *);
int clReleaseMemObject(cl_mem);
cl_command_queue clCreateCommandQueue(cl_context, cl_device_id, const cl_queue_properties *, int *);
  
int clEnqueueReadBuffer(cl_command_queue, cl_mem, cl_bool, size_t, size_t, void *,
                        unsigned numEvents, const cl_event *waitEvents, cl_event *outEvent);
int clEnqueueWriteBuffer(cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *,
                         unsigned numEvent, const cl_event *waitEvents, cl_event *outEvent);

int clFlush(cl_command_queue);
int clFinish(cl_command_queue);
int clSetKernelArg(cl_kernel, unsigned, size_t, const void *);

int clReleaseEvent(cl_event);
int clWaitForEvents(unsigned numEvents, const cl_event *);

}

#define CL_SUCCESS              0
#define CL_DEVICE_TYPE_GPU      (1 << 2)
#define CL_DEVICE_TYPE_ALL      0xFFFFFFFF
#define CL_PLATFORM_VERSION     0x0901
#define CL_DEVICE_ERROR_CORRECTION_SUPPORT 0x1024
#define CL_DEVICE_NAME          0x102B
#define CL_DEVICE_VERSION       0x102F
#define CL_DRIVER_VERSION       0x102D
#define CL_DEVICE_BUILT_IN_KERNELS 0x103F
#define CL_PROGRAM_BINARY_SIZES 0x1165
#define CL_PROGRAM_BINARIES     0x1166
#define CL_PROGRAM_BUILD_LOG    0x1183
#define CL_MEM_READ_WRITE       (1 << 0)
#define CL_MEM_READ_ONLY        (1 << 2)
#define CL_MEM_COPY_HOST_PTR    (1 << 5)
#define CL_MEM_HOST_NO_ACCESS   (1 << 9)
#define CL_INVALID_COMPILER_OPTIONS -66

