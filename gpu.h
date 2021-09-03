#include<stdint.h>
#include<stddef.h>
#include<llvm/IR/Module.h>

#ifdef _cplusplus
extern "C" {
#endif
void *gpuManagedMalloc(size_t n); 
void initRuntime(); 
void* launchBCKernel(void* bc, void** args, size_t n); 
void* launchKernel(llvm::Module& bc, void** args, size_t n); 
void waitKernel(void* wait); 
#ifdef _cplusplus
}
#endif
