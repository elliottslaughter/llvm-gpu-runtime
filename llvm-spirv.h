#include<stdbool.h>
#include<dlfcn.h>
#include<llvm/Target/TargetMachine.h>

bool initSPIRV(){ 
  void* openclHandle = dlopen("libOpenCL.so", RTLD_LAZY); 
  void* spirvHandle = dlopen("libLLVMSPIRVLib.so", RTLD_LAZY); 
  bool res = false; 
  if(openclHandle && spirvHandle){
    // TODO: Fixme to check whether clCreateProgramWithIL is actually
    // supported by device
    void* ilHandle = dlsym(openclHandle, "clCreateProgramWithIL");
    res = ilHandle != NULL; 
  }

  dlclose(openclHandle);
  dlclose(spirvHandle); 
  return res;
}
void* launchSPIRVKernel(void* kernel) {return NULL;}
void waitSPIRVKernel(void* wait) {}
