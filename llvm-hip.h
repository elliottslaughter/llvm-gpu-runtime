#include<stdbool.h>
#include<dlfcn.h>
#include<llvm/Target/TargetMachine.h>

int initHIP(){ 
  void* handle = dlopen("libhip.so", RTLD_LAZY); 
  return handle != NULL;
}
void* getHIPKernel(void* bc) {return NULL;}
void* runHIPKernel(void* kernel) {return NULL;}
void waitHIPKernel(void* wait) {}
