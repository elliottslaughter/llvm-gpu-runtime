all: test gpu.bc kernel.ll kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll

flags=-g \
	-lrt -ldl -lcudart_static -lnvptxcompiler_static -lpthread -lLLVM \
	-I/opt/cuda/include/ 

opt=-g

test: test.cc gpu.o llvm-hip.o kernel.bc 
	clang++ $< llvm-hip.o gpu.o ${flags} -o $@

kernel.bc: kernel.c
	clang -c -O2 -emit-llvm $< -o $@ 

kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll: kernel.cu
	clang -S -emit-llvm $< --cuda-gpu-arch=sm_75

kernel.ll: kernel.c
	clang -S -emit-llvm $<

llvm-hip.o: llvm-hip.cc
	clang++ ${opt} -c $< -o $@

gpu.o: gpu.cc *.h  
	clang++ ${opt} -c $< -o $@

