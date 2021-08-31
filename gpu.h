

extern "C" {
void *gpuManagedMalloc(size_t n); 
void initRuntime(); 
void* launchKernel(llvm::Module& bc, void** args, size_t n); 
void waitKernel(void* wait); 
}
