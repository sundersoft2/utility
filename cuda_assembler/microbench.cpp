#include "include.h"

void error_check_impl(CUresult v, const char* file, int line) {
    if (v==CUDA_SUCCESS) {
        return;
    }

    print( "error_check failed. v=", v, "file=", file, "line=", line);

    const char* name=nullptr;
    cuGetErrorName(v, &name);
    if (name!=nullptr) {
        print(name);
    }

    const char* description=nullptr;
    cuGetErrorString(v, &description);
    if (description!=nullptr) {
        print(description);
    }

    assert(false);
}
#define error_check(v) error_check_impl((v), __FILE__, __LINE__)

int main() {
    const int num_blocks=1;
    const int threads_per_block=32;
    const int shared_mem_size=48*1024;

    error_check(cuInit(0));

    CUdevice device;
    error_check(cuDeviceGet(&device, 0));

    CUcontext context;
    error_check(cuCtxCreate(&context, CU_CTX_SCHED_BLOCKING_SYNC | CU_CTX_MAP_HOST, device));

    string cubin=getfile("microbench.src.cubin", true);

    CUmodule module;
    error_check(cuModuleLoadData(&module, &(cubin[0])));

    CUfunction cuda_function;
    error_check(cuModuleGetFunction(&cuda_function, module, "microbench"));

    CUdeviceptr regs_device;
    size_t regs_size;
    error_check(cuModuleGetGlobal(&regs_device, &regs_size, module, "regs"));
    assert(regs_size%4==0);

    vector<float> regs_host(regs_size/4);
    for (int x=0;x<regs_host.size();++x) {
        regs_host[x]=x;
    }

    error_check(cuFuncSetCacheConfig(cuda_function, CU_FUNC_CACHE_PREFER_SHARED));
    error_check(cuFuncSetSharedMemConfig(cuda_function, CU_SHARED_MEM_CONFIG_FOUR_BYTE_BANK_SIZE));

    error_check(cuMemcpyHtoD(regs_device, &(regs_host[0]), regs_size));
    error_check(cuLaunchKernel(
        cuda_function,
        num_blocks, 1, 1,
        threads_per_block, 1, 1,
        shared_mem_size,
        nullptr, nullptr, nullptr
    ));
    error_check(cuMemcpyDtoH(&(regs_host[0]), regs_device, regs_size));

    //for (int x=0;x<regs_host.size();++x) {
    for (int x=0;x<16;++x) {
        print( "Float: R", x, " = ", regs_host[x] );
    }
    print("");

    for (int x=0;x<16;++x) {
        unsigned int i_val=*(unsigned int*)&regs_host[x];
        print( "Int: R", x, " = ", i_val );
    }
    print("");

    for (int x=0;x<16;++x) {
        unsigned int i_val=*(unsigned int*)&regs_host[x];
        ostringstream ss;
        ss << "0x" << hex << i_val;
        print( "Hex: R", x, " = ", ss.str() );
    }
    print("");

    unsigned int last=0;
    for (int x=0;x<16;++x) {
        unsigned int i_val=*(unsigned int*)&regs_host[x];
        print( "Delta: R", x, " = ", i_val-last );
        last=i_val;
    }
    print("");

    error_check(cuModuleUnload(module));
    error_check(cuCtxDestroy(context));
}
