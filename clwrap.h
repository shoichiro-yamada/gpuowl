// Copyright (C) 2017 Mihai Preda.

#include "tinycl.h"
#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <sys/time.h>

typedef unsigned char byte;
typedef long long i64;
typedef unsigned long long u64;
// typedef int64_t i64;
// typedef uint64_t u64;

u64 timeMillis() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

u64 timeMicros() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_usec;
}

class Timer {
  u64 prev;
  
 public:
  Timer() : prev (timeMillis()) { }

  u64 delta() {
    u64 now = timeMillis();
    u64 d   = now - prev;
    prev    = now;
    return d;
  }
};

class MicroTimer {
  u64 prev;
  
 public:
  MicroTimer() : prev(timeMicros()) { }

  u64 delta() {
    u64 now = timeMicros();
    u64 d = (now > prev) ? now - prev : (1000000 + now - prev);
    prev = now;
    return d;
  }
};

class TimeCounter {
  MicroTimer *timer;
  u64 us;
  
 public:
  TimeCounter(MicroTimer *t) : timer(t) , us(0) { }

  void tick() { us += timer->delta(); }
  u64 get() { return us; }
  void reset() { us = 0; }
};

#define CHECK(err) { int e = err; if (e != CL_SUCCESS) { fprintf(stderr, "error %d\n", e); assert(false); }}
#define CHECK2(err, mes) { int e = err; if (e != CL_SUCCESS) { fprintf(stderr, "error %d (%s)\n", e, mes); assert(false); }}

void getInfo(cl_device_id id, int what, size_t bufSize, char *buf) {
  size_t outSize = 0;
  CHECK(clGetDeviceInfo(id, what, bufSize, buf, &outSize));
  assert(outSize < bufSize);
  buf[outSize] = 0;
}

int getDeviceIDs(bool onlyGPU, size_t size, cl_device_id *out) {
  cl_platform_id platforms[8];
  unsigned nPlatforms;
  CHECK(clGetPlatformIDs(8, platforms, &nPlatforms));
  
  unsigned n = 0;
  for (int i = 0; i < (int) nPlatforms && size > n; ++i) {
    unsigned delta = 0;
    CHECK(clGetDeviceIDs(platforms[i], onlyGPU ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_ALL, size - n, out + n, &delta));
    n += delta;
  }
  return n;
}

int getNumberOfDevices() {
  cl_platform_id platforms[8];
  unsigned nPlatforms;
  CHECK(clGetPlatformIDs(8, platforms, &nPlatforms));
  
  unsigned n = 0;
  for (int i = 0; i < (int) nPlatforms; ++i) {
    unsigned delta = 0;
    CHECK(clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &delta));
    n += delta;
  }
  return n;
}

void getDeviceInfo(cl_device_id device, size_t infoSize, char *info) {
  char name[128];
  char version[128];
  getInfo(device, CL_DEVICE_NAME,    sizeof(name), name);
  getInfo(device, CL_DEVICE_VERSION, sizeof(version), version);

  unsigned isEcc = 0;
  CHECK(clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(isEcc), &isEcc, NULL));
  
  snprintf(info, infoSize, "%s; %s%s", name, version, isEcc ? " (ECC)" : "");
}

cl_context createContext(cl_device_id device) {
  int err;
  cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  CHECK(err);
  return context;
}

typedef cl_command_queue cl_queue;

void release(cl_context context) { CHECK(clReleaseContext(context)); }
void release(cl_program program) { CHECK(clReleaseProgram(program)); }
void release(cl_mem buf)         { CHECK(clReleaseMemObject(buf)); }
void release(cl_queue queue)     { CHECK(clReleaseCommandQueue(queue)); }
void release(cl_kernel k)        { CHECK(clReleaseKernel(k)); }

cl_program compile(cl_device_id device, cl_context context, const char *fileName, const char *opts) {
  FILE *fi = fopen(fileName, "r");
  if (!fi) {
    fprintf(stderr, "Could not open cl source file '%s'\n", fileName);
    return 0;
  }
    
  char buf[64 * 1024];
  size_t size = fread(buf, 1, sizeof(buf), fi);
  fclose(fi);
  assert(size < sizeof(buf));

  char *pbuf = buf;
  int err;
  cl_program program = clCreateProgramWithSource(context, 1, (const char **)&pbuf, &size, &err);
  CHECK(err);

  // First try CL2.0 compilation.
  snprintf(buf, sizeof(buf), "-cl-fast-relaxed-math -cl-std=CL2.0 -cl-uniform-work-group-size %s", opts);
  if ((err = clBuildProgram(program, 1, &device, buf, NULL, NULL)) < 0) {
    printf("Falling back to CL1.x compilation (error %d)\n", err);
    snprintf(buf, sizeof(buf), "-cl-fast-relaxed-math %s", opts);
    err = clBuildProgram(program, 1, &device, buf, NULL, NULL);
  }

  if (err != CL_SUCCESS) {
    size_t logSize;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buf), buf, &logSize);
    buf[logSize] = 0;
    fprintf(stderr, "OpenCL compilation error %d, log:\n%s\n", err, buf);
    return 0;
  }

  return program;
}  
  // Other options:
  // * to output GCN ISA: -save-temps or -save-temps=prefix or -save-temps=folder/
  // * to disable all OpenCL optimization (do not use): -cl-opt-disable
  // * -cl-uniform-work-group-size
  // * -fno-bin-llvmir
  // * various: -fno-bin-source -fno-bin-amdil


cl_kernel makeKernel(cl_program program, const char *name) {
  int err;
  cl_kernel k = clCreateKernel(program, name, &err);
  CHECK2(err, name);
  return k;
}

void setArg(cl_kernel k, int pos, const auto &value) { CHECK(clSetKernelArg(k, pos, sizeof(value), &value)); }
void setArgs(cl_kernel k, const auto &a) { setArg(k, 0, a); }
void setArgs(cl_kernel k, const auto &a, const auto &b) { setArgs(k, a); setArg(k, 1, b); }
void setArgs(cl_kernel k, const auto &a, const auto &b, const auto &c) { setArgs(k, a, b); setArg(k, 2, c); }
void setArgs(cl_kernel k, const auto &a, const auto &b, const auto &c, const auto &d) { setArgs(k, a, b, c); setArg(k, 3, d); }
void setArgs(cl_kernel k, const auto &a, const auto &b, const auto &c, const auto &d, const auto &e) {
  setArgs(k, a, b, c, d);
  setArg(k, 4, e);
}
void setArgs(cl_kernel k, const auto &a, const auto &b, const auto &c, const auto &d, const auto &e, const auto &f) {
  setArgs(k, a, b, c, d, e);
  setArg(k, 5, f);
}

cl_mem makeBuf(cl_context context, unsigned kind, size_t size, const void *ptr = 0) {
  int err;
  cl_mem buf = clCreateBuffer(context, kind, size, (void *) ptr, &err);
  CHECK(err);
  return buf;
}

cl_queue makeQueue(cl_device_id d, cl_context c) {
  int err;
  cl_queue q = clCreateCommandQueue(c, d, 0, &err);
  CHECK(err);
  return q;
}

/*
void run(cl_queue queue, cl_kernel kernel, size_t workSize) {
  size_t groupSize = 256;
  CHECK(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &workSize, &groupSize, 0, NULL, NULL));
}
*/

void flush( cl_queue q) { CHECK(clFlush(q)); }
void finish(cl_queue q) { CHECK(clFinish(q)); }

void run(cl_queue queue, cl_kernel kernel, size_t workSize, TimeCounter *counter = nullptr) {
  size_t groupSize = 256;
  CHECK(clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &workSize, &groupSize, 0, NULL, NULL));
  if (counter) { 
    finish(queue);
    counter->tick();
  }
}

void run(cl_queue queue, cl_kernel kernel, size_t workSize, const auto &a) {
  setArgs(kernel, a);
  run(queue, kernel, workSize);
}

void run(cl_queue queue, cl_kernel kernel, size_t workSize, const auto &a, const auto &b) {
  setArgs(kernel, a, b);
  run(queue, kernel, workSize);
}

void read(cl_queue queue, bool blocking, cl_mem buf, size_t size, void *data, size_t start = 0) {
  CHECK(clEnqueueReadBuffer(queue, buf, blocking, start, size, data, 0, NULL, NULL));
}

void write(cl_queue queue, bool blocking, cl_mem buf, size_t size, const void *data, size_t start = 0) {
  CHECK(clEnqueueWriteBuffer(queue, buf, blocking, start, size, data, 0, NULL, NULL));
}
