#ifdef found_hip
#include"hip.h"
#else
#include"nohip.h"
#endif

#ifdef found_cuda
#include"cuda.h"
#else
#include"nocuda.h"
#endif

#ifdef found_spirv
#include"spirv.h"
#else
#include"nospirv.h"
#endif

