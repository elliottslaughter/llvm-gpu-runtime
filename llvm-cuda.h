#include<stdbool.h>
#include<sstream>
#include<iostream>
#include<dlfcn.h>
#include<llvm/IR/LegacyPassManager.h>
#include<llvm/IR/Constants.h>
#include<llvm/IR/Instruction.h>
#include<llvm/IR/Verifier.h>
#include<llvm/Support/TargetSelect.h>
#include<llvm/Support/CommandLine.h>
#include<llvm/Support/raw_os_ostream.h>
#include<llvm/Target/TargetMachine.h>
#include<llvm/Support/ToolOutputFile.h>
#include<llvm/Support/TargetRegistry.h>
#include<llvm/Transforms/IPO/PassManagerBuilder.h>
#include<cuda_runtime_api.h>
#include<nvPTXCompiler.h>
#include<cuda.h>

CUcontext context;

// TODO: These globals are problematic and implementation dependent.
void* handle = dlopen("libcuda.so", RTLD_LAZY); 
#define tryLoad(name) decltype(name)* name##_p = (decltype(name)*)dlsym(handle, #name)
tryLoad(cuInit); 
tryLoad(cuStreamCreate); 
tryLoad(cuStreamDestroy_v2); 
tryLoad(cuStreamSynchronize); 
tryLoad(cuLaunchKernel); 
tryLoad(cuDeviceGet); 
tryLoad(cuGetErrorName); 
tryLoad(cuModuleLoadDataEx); 
tryLoad(cuModuleGetFunction); 
tryLoad(cuModuleUnload); 
tryLoad(cuCtxCreate_v2); 
tryLoad(cuCtxDestroy_v2); 
tryLoad(cuCtxSetCurrent); 

cudaStream_t *pStream; 

using namespace llvm; 

#define CUDA_SAFE_CALL(x)                                               \
    do {                                                                \
        CUresult result = x;                                            \
        if (result != CUDA_SUCCESS) {                                   \
            const char *msg;                                            \
            cuGetErrorName_p(result, &msg);                               \
            printf("error: %s failed with error %s\n", #x, msg);        \
            exit(1);                                                    \
        }                                                               \
    } while(0)

#define NVPTXCOMPILER_SAFE_CALL(x)                                       \
    do {                                                                 \
        nvPTXCompileResult result = x;                                   \
        if (result != NVPTXCOMPILE_SUCCESS) {                            \
            printf("error: %s failed with error code %d\n", #x, result); \
            exit(1);                                                     \
        }                                                                \
    } while(0)

bool initCUDA(){
  if(!handle) return false; 
  if(!cuInit_p) return false; 
  return true;
}

std::string cudaarch = "sm_70";
std::string cudafeatures = "+ptx64"; 

void* PTXtoELF(const char* ptx){
      nvPTXCompilerHandle compiler = NULL;
  nvPTXCompileResult status;

  size_t elfSize, infoSize, errorSize;
  char *elf, *infoLog, *errorLog;
  unsigned int minorVer, majorVer;

  std::string gpuName = "--gpu-name=" + cudaarch; 
  //std::string gpuFeatures = "--gpu-features=" + cudafeatures; 

  const char* compile_options[] = { gpuName.c_str(), 
                                    //gpuFeatures.c_str(),
                                    "--verbose"
                                  };

  NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerGetVersion(&majorVer, &minorVer));
  printf("Current PTX Compiler API Version : %d.%d\n", majorVer, minorVer);

  NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerCreate(&compiler,
                                              (size_t)strlen(ptx),  /* ptxCodeLen */
                                              ptx)                  /* ptxCode */
                          );

  status = nvPTXCompilerCompile(compiler,
                                2,                 /* numCompileOptions */
                                compile_options);  /* compileOptions */

  if (status != NVPTXCOMPILE_SUCCESS) {
      NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerGetErrorLogSize(compiler, &errorSize));

      if (errorSize != 0) {
          errorLog = (char*)malloc(errorSize+1);
          NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerGetErrorLog(compiler, errorLog));
          printf("Error log: %s\n", errorLog);
          free(errorLog);
      }
      exit(1);
  }

  NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerGetCompiledProgramSize(compiler, &elfSize));

  elf = (char*) malloc(elfSize);
  NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerGetCompiledProgram(compiler, (void*)elf));

  NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerGetInfoLogSize(compiler, &infoSize));

  if (infoSize != 0) {
      infoLog = (char*)malloc(infoSize+1);
      NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerGetInfoLog(compiler, infoLog));
      printf("Info log: %s\n", infoLog);
      free(infoLog);
  }

  NVPTXCOMPILER_SAFE_CALL(nvPTXCompilerDestroy(&compiler));

  return elf; 
}

const char* LLVMtoPTX(Module& m) {
  LLVMContext& ctx = m.getContext(); 
  int deviceCount = 0;
  cudaGetDeviceCount(&deviceCount);
  for(int dev=0; dev<deviceCount; dev++){
    cudaSetDevice(dev);
    cudaDeviceProp deviceProp;
    cudaGetDeviceProperties(&deviceProp, dev);
    std::ostringstream arch;
    arch << "sm_" << deviceProp.major << deviceProp.minor; 
    cudaarch = arch.str(); 
    std::cout << cudaarch << std::endl; 
  }

  Triple TT("nvptx64", "nvidia", "cuda"); 
  m.setTargetTriple(TT.str()); 
  for(auto &F : m){
    AttrBuilder Attrs;
    Attrs.addAttribute("target-cpu", cudaarch);
    Attrs.addAttribute("target-features", cudafeatures + ",+" + cudaarch);
    /*
    Attrs.addAttribute(Attribute::NoRecurse); 
    Attrs.addAttribute(Attribute::Convergent); 
    */
    F.removeFnAttr("target-cpu");
    F.removeFnAttr("target-features");
    /*
    F.removeFnAttr(Attribute::StackProtectStrong); 
    F.removeFnAttr(Attribute::UWTable); 
    */
    F.addAttributes(AttributeList::FunctionIndex, Attrs);
    NamedMDNode *Annotations =
      m.getOrInsertNamedMetadata("nvvm.annotations");

    SmallVector<Metadata *, 3> AV;
    AV.push_back(ValueAsMetadata::get(&F));
    AV.push_back(MDString::get(ctx, "kernel"));
    AV.push_back(ValueAsMetadata::get(ConstantInt::get(Type::getInt32Ty(ctx),
                                                       1)));
    Annotations->addOperand(MDNode::get(ctx, AV));
  }

  Type* IntTy = Type::getInt32Ty(ctx); 
  auto tid = m.getOrInsertFunction("llvm.nvvm.read.ptx.sreg.tid.x", 
    FunctionType::get(IntTy,  IntTy )); 

  // Create PTX
  auto ptxbuf = new SmallVector<char, 1<<20>(); 
  raw_svector_ostream ptx(*ptxbuf); 

  legacy::PassManager PM;
  legacy::FunctionPassManager FPM(&m); 
  PassManagerBuilder Builder;
  Builder.OptLevel = 2; 
  Builder.populateFunctionPassManager(FPM);  
  Builder.populateModulePassManager(PM); 

  // TODO: Hard coded machine configuration, use cuda to check 
  std::string error;
  raw_os_ostream ostr(std::cout); 
  InitializeAllTargets(); 
  InitializeAllTargetMCs(); 
  InitializeAllAsmPrinters(); 

  const Target *PTXTarget = TargetRegistry::lookupTarget("", TT, error);
  if(!PTXTarget){
    std::cerr << error << std:: endl;   
    exit(1);
  }
  auto PTXTargetMachine =
      PTXTarget->createTargetMachine(TT.getTriple(), cudaarch,
                                     "+ptx64", TargetOptions(), Reloc::PIC_,
                                     CodeModel::Small, CodeGenOpt::Aggressive);
  m.setDataLayout(PTXTargetMachine->createDataLayout());

  bool Fail = PTXTargetMachine->addPassesToEmitFile(PM, ptx, nullptr, CodeGenFileType::CGFT_AssemblyFile, false); 
  assert(!Fail && "Failed to emit PTX"); 
  
  FPM.doInitialization();
  for(Function &F : m) FPM.run(F);
  FPM.doFinalization();
  PM.add(createVerifierPass());
  PM.run(m); 
  
  m.print(llvm::errs(), nullptr); 
  std::cout << ptx.str().str() << std::endl;
  return ptx.str().data();  
}

CUstream launchCudaELF(void* elf, void** args, size_t n){
  CUstream stream;

  CUmodule module;
  CUfunction kernel;
  CUdevice device; 

  CUDA_SAFE_CALL(cuInit_p(0));
  CUDA_SAFE_CALL(cuDeviceGet_p(&device, 0));

  CUDA_SAFE_CALL(cuCtxCreate_v2_p(&context, 0, device));
  CUDA_SAFE_CALL(cuModuleLoadDataEx_p(&module, elf, 0, 0, 0));
  CUDA_SAFE_CALL(cuModuleGetFunction_p(&kernel, module, "f"));

  CUDA_SAFE_CALL(cuStreamCreate_p(&stream, 0)); 
  CUDA_SAFE_CALL( cuLaunchKernel_p(kernel,
                                 1, 1, 1, // grid dim
                                 n, 1, 1, // block dim
                                 0, stream, // shared mem and stream
                                 args, NULL)); // arguments

  // Release resources.
  CUDA_SAFE_CALL(cuModuleUnload_p(module));
 
  return stream;
}

CUstream launchCUDAKernel(Module& m, void** args, int n) {
  const char* ptx = LLVMtoPTX(m);
  void* elf = PTXtoELF(ptx); 
  return launchCudaELF(elf, args, n); 
}

void waitCUDAKernel(CUstream wait) {
  CUDA_SAFE_CALL(cuStreamSynchronize_p(wait)); 
  CUDA_SAFE_CALL(cuStreamDestroy_v2_p(wait)); 
  CUDA_SAFE_CALL(cuCtxDestroy_v2_p(context));
}

