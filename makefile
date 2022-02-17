all: test libllvm-gpu.so 

nvptx=${CUDA_PATH}/lib64/libnvptxcompiler_static.a
links:= -lrt -lncurses -ldl -lpthread -lz $(shell ${ROCM_PATH}/llvm/bin/llvm-config --ldflags) $(shell ${ROCM_PATH}/llvm/bin/llvm-config --libs)
incs=-std=c++14 $(shell ${ROCM_PATH}/llvm/bin/llvm-config --cflags)
opts=-g -fPIC
flags=${links} ${incs} ${opts} -Wall 
clang=hipcc # clang++ 

test: test.o gpu.o spirv.o hip.o linkedcuda.o kernel.bc 
	${clang} $< gpu.o spirv.o hip.o linkedcuda.o ${incs} ${flags} -o $@

test.o: test.cc
	${clang} ${opts} ${incs} -c $< -o $@

test.ll: test.cc 
	${clang} $< ${incs} -S -emit-llvm -o $@

libllvm-gpu.so: gpu.o spirv.o hip.o linkedcuda.o
	${clang} -shared $^ -o $@ ${flags}

kernel.bc: kernel.c
	${clang} -c -O2 -emit-llvm $< -o $@ 

kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll: kernel.cu
	${clang} -S -emit-llvm $< --cuda-gpu-arch=sm_75

kernel.ll: kernel.c
	${clang} -S -emit-llvm $<

cuda.o: check-cuda.cc llvm-cuda.cc nocuda.cc
	${clang} ${opts} ${incs} -c $< -o $@

linkedcuda.o: cuda.o
	ld -r -o $@ $< ${nvptx} || cp $< $@

hip.o: check-hip.cc llvm-hip.cc nohip.cc
	${clang} ${opts} ${incs} -c $< -o $@

spirv.o: check-spirv.cc llvm-spirv.cc nospirv.cc
	${clang} ${opts} ${incs} -c $< -o $@

gpu.o: gpu.cc *.h  
	${clang} ${opts} ${incs} -c $< -o $@

install: libllvm-gpu.so gpu.h
	install -d ${prefix}/lib
	install -m 644 libllvm-gpu.so ${prefix}/lib
	install -d ${prefix}/include
	install -m 644 gpu.h ${prefix}/include

.PHONY: clean
clean:
	rm -f *.o *.so *.bc *.ll test
