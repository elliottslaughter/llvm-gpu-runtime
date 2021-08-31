#if __has_include("hip/hip_runtime.h")
#include"llvm-hip.h"
#else
#warning "Couldn't find hip, not building hip support"
#include"nohip.h"
#endif

#if __has_include("cuda.h")
#include"llvm-cuda.h"
#else
#warning "Couldn't find cuda, not building cuda support"
#include"nocuda.h"
#endif

#ifdef found_spirv
#include"llvm-spirv.h"
#else
#include"nospirv.h"
#endif
