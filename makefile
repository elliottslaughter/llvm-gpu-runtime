all: test kernel.ll kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll

flags=-g \
	-Dfound_cuda -Dfound_hip -Dfound_spirv \
	-lrt -ldl -lcudart_static -lnvptxcompiler_static -lpthread -lLLVM \
	-I/opt/cuda/include/

test: test.cc *.h kernel.bc 
	clang++ $< ${flags} -o $@

kernel.bc: kernel.c
	clang -c -emit-llvm $< -o $@ 

kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll: kernel.cu
	clang -S -emit-llvm $< --cuda-gpu-arch=sm_75
	

kernel.ll: kernel.c
	clang -S -emit-llvm $<

gpu.bc: gpu.c
	clang++ -c ${flags} -emit-llvm $< -o $@

