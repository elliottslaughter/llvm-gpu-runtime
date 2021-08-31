all: test gpu.bc kernel.ll kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll

links=-L${CUDA_PATH}/targets/x86_64-linux/lib -lrt -ldl -lcudart_static -lnvptxcompiler_static -lpthread -lLLVM 
incs=-I${CUDA_PATH}/include/  -I${CUDA_PATH}/targets/x86_64-linux/include
opts=-g
flags=${links} ${incs} ${opts}

test: test.cc gpu.o spirv.o hip.o cuda.o kernel.bc 
	clang++ $< spirv.o hip.o cuda.o gpu.o ${flags} -o $@

kernel.bc: kernel.c
	clang -c -O2 -emit-llvm $< -o $@ 

kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll: kernel.cu
	clang -S -emit-llvm $< --cuda-gpu-arch=sm_75

kernel.ll: kernel.c
	clang -S -emit-llvm $<

cuda.o: check-cuda.cc llvm-cuda.cc nocuda.cc
	clang++ ${opts} ${incs} -c $< -o $@

hip.o: check-hip.cc llvm-hip.cc nohip.cc
	clang++ ${opts} ${incs} -c $< -o $@

spirv.o: check-spirv.cc llvm-spirv.cc nospirv.cc
	clang++ ${opts} ${incs} -c $< -o $@

gpu.o: gpu.cc *.h  
	clang++ ${opts} ${incs} -c $< -o $@

