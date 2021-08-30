#ifndef __IMPLEMENTATIONS__
#ifdef found_hip
#include"llvm-hip.h"
#else
#include"nohip.h"
#endif

#ifdef found_cuda
#include"llvm-cuda.h"
#else
#include"nocuda.h"
#endif

#ifdef found_spirv
#include"llvm-spirv.h"
#else
#include"nospirv.h"
#endif

#endif
