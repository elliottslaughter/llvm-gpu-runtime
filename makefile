all: test libllvm-gpu.so 

installPrefix=${HOME}/usr/${PREFIX}
nvptx=${CUDA_PATH}/lib64/libnvptxcompiler_static.a
links:=-lrt -lcurses -ldl -lpthread -lz -lLLVM-12
clanginstall=/usr/local/
incs=-I${CUDA_PATH}/include/  -I${CUDA_PATH}/targets/x86_64-linux/include -I${clanginstall}/include/
opts=-g -fPIC
flags=${links} ${incs} ${opts} -Wall
clang=clang++

test: test.cc gpu.o spirv.o hip.o linkedcuda.o kernel.bc 
	${clang} $< gpu.o spirv.o hip.o linkedcuda.o ${incs} ${flags} -o $@

test.ll: test.cc 
	${clang} $< ${incs} -S -emit-llvm -o $@

libllvm-gpu.so: gpu.o spirv.o hip.o linkedcuda.o
	${clang} -shared $^ -o $@ ${flags}

kernel.bc: kernel.c
	clang -c -O2 -emit-llvm $< -o $@ 

kernel-cuda-nvptx64-nvidia-cuda-sm_75.ll: kernel.cu
	clang -S -emit-llvm $< --cuda-gpu-arch=sm_75

kernel.ll: kernel.c
	clang -S -emit-llvm $<

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
	install -d ${installPrefix}/lib
	install -m 644 libllvm-gpu.so ${installPrefix}/lib
	install -d ${installPrefix}/include
	install -m 644 gpu.h ${installPrefix}/include

.PHONY: clean
clean:
	rm -f *.o *.so *.bc *.ll test
