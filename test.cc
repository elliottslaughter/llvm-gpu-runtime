#include"implementations.h"
#include<llvm/IRReader/IRReader.h>

int main(){
  printf("cuda %d\n", initCUDA()); 
  printf("hip %d\n", initHIP()); 
  printf("opencl %d\n", initSPIRV()); 

  LLVMContext C; 
  SMDiagnostic SMD; 
  std::unique_ptr<Module> ExternalModule =
      parseIRFile("kernel.bc", SMD, C);

  int n = 10; 
  double* x; 
  cudaMallocManaged((void**)&x, n*sizeof(double)); 
   
  void* args[] = { &x }; 
  auto w = launchCUDAKernel(*ExternalModule.get(), args, 10); 
  std::cout << "Launched kernel...";
  waitCUDAKernel(w); 
  std::cout << "done" << std::endl; 
  for(int i=0; i<n; i++){
    printf("%f\n", x[i]);
  }
}


