
gpu.bc: gpu.c
	clang -c -emit-llvm $< -o $@
