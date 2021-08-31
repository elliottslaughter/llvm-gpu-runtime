#include"gpu.h"
#include<error.h>
#include<llvm-cuda.h>
#include<llvm-hip.h>
#include<nospirv.h>
#include<stdbool.h>
#include<llvm/IR/Module.h>

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

void *gpuManagedMalloc(size_t n){
	switch(globalRuntime){
		case hip:
			return hipManagedMalloc(n);
		case cuda:
			return cudaManagedMalloc(n);
		default:
			err("no spirv managed malloc");
	}	
	return NULL;
}

void initRuntime(){
  if(globalRuntime != none) return;
  if(initCUDA()) {
		globalRuntime = cuda;
		return;
	}
  if(initHIP()){
		globalRuntime = hip; 
		return;
	}
  if(initSPIRV()){
		globalRuntime = spirv; 
		return;
	}
	err("No gpu runtimes found, needed OpenCL with SPIRV support, HIP, or CUDA\n");
}

void* launchKernel(llvm::Module& bc, void** args, size_t n){
  switch(globalRuntime){
    case spirv: 
      return launchSPIRVKernel(bc, args, n);
    case hip:
      return launchHIPKernel(bc, args, n);
    case cuda:
      return launchCUDAKernel(bc, args, n);
    default:
      err("Can't get kernel without valid runtime");
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
