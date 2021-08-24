#include"implementations.h"
#include<error.h>
#include<stdbool.h>

void err(const char* msg){
  return error(1, 1, "%s", msg);
}

typedef enum {
  none,
  spirv,
  hip,
  cuda
} runtime;  

runtime globalRuntime = none;

void initRuntime(){
  if(globalRuntime != none) return;
  if(initSPIRV()) globalRuntime = spirv; 
  if(initHIP()) globalRuntime = spirv; 
  if(initCUDA()) globalRuntime = cuda;
  err("No gpu runtimes found, needed OpenCL with SPIRV support, HIP, or CUDA\n");
}

void* getKernel(void* bc){
  switch(globalRuntime){
    case spirv: 
      return getSPIRVKernel(bc);
    case hip:
      return getHIPKernel(bc);
    case cuda:
      return getCUDAKernel(bc);
    default:
      err("Can't get kernel without valid runtime");
  }
  return NULL; 
}

void* runKernel(void* kernel){
  switch(globalRuntime){
    case spirv:
      return runSPIRVKernel(kernel);
    case hip:
      return runHIPKernel(kernel);
    case cuda:
      return runCUDAKernel(kernel);
    default:
      err("Can't run kernel without valid runtime");
  }
  return NULL; 
}

void waitKernel(void* wait){
  switch(globalRuntime){
    case spirv:
      return waitSPIRVKernel(wait);
    case hip:
      return waitHIPKernel(wait);
    case cuda:
      return waitCUDAKernel(wait);
    default:
      err("Can't wait kernel without valid runtime");
  }
}
