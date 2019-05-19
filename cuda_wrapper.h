namespace cuda_wrapper {


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

struct no_copy {
    no_copy(const no_copy&)=delete;
    no_copy& operator=(const no_copy&)=delete;
};

struct no_move {
    no_move(no_move&&)=delete;
    no_move& operator=(no_move&&)=delete;
};

struct cuda_device {
    CUdevice device;
    CUcontext context;

    cuda_device() {
        static bool is_init=false;
        if (!is_init) {
            cuInit(0);
            is_init=true;
        }

        int num_devices=0;
        error_check(cuDeviceGetCount(num_devices));

        assert(num_devices==1);
        error_check(cuDeviceGet(&device, 0));

        error_check(cuCtxCreate(&context, CU_CTX_SCHED_YIELD, device));
        error_check(cuCtxSetCacheConfig(&context, CU_FUNC_CACHE_PREFER_SHARED));
    }

    void bind() {
        error_check(cuCtxSetCurrent(context));
    }
};

struct cuda_event : no_copy {
    bool is_init=false;
    CUevent event;

    cuda_event() {
        is_init=true;
        error_check(cuEventCreate(&event, CU_EVENT_BLOCKING_SYNC | CU_EVENT_DISABLE_TIMING));
    }

    ~cuda_event() {
        if (init) {
            error_check(cuEventDestroy(event));
        }
    }

    cuda_event(cuda_event&& t) {
        is_init=t.init;
        event=t.event;
        t.is_init=false;
    }

    cuda_event& operator=(cuda_event&& t) {
        *this=move(t);
        return *this;
    }

    bool done() {
        assert(is_init);

        CUresult res=cuEventQuery(event);

        if (res==CUDA_ERROR_NOT_READY) {
            return false;
        }

        error_check(res);
        return true;
    }

    void sync() {
        assert(is_init);
        error_check(cuEventSynchronize(event));
    }
};

struct cuda_stream : no_copy {
    bool is_init=false;
    CUstream stream;

    cuda_stream() {
        is_init=true;
        error_check(cuStreamCreate(&stream, CU_STREAM_NON_BLOCKING));
    }

    ~cuda_stream() {
        if (init) {
            error_check(cuStreamDestroy(stream));
        }
    }

    cuda_stream(cuda_stream&& t) {
        is_init=t.init;
        stream=t.stream;
        t.is_init=false;
    }

    cuda_stream& operator=(cuda_stream&& t) {
        *this=move(t);
        return *this;
    }

    bool done() {
        assert(is_init);

        CUresult res=cuStreamQuery(stream);

        if (res==CUDA_ERROR_NOT_READY) {
            return false;
        }

        error_check(res);
        return true;
    }

    void sync() {
        assert(is_init);
        error_check(cuStreamSynchronize(stream));
    }

    void wait(cuda_event& c_event) {
        assert(is_init && c_event.is_init);
        error_check(cuStreamWaitEvent(stream, c_event.event, 0));
    }

    void record(cuda_event& c_event) {
        assert(is_init && c_event.is_init);
        error_check(cuEventRecord(c_event.event, stream));
    }

    static void callback_impl(void* f_copy_void) {
        auto f_copy=(function<void()>*)f_copy_void;
        f_copy();
        delete f_copy;
    }

    void callback(function<void()> f) {
        assert(is_init);
        auto f_copy=new function<void()>(move(f));
        error_check(cuLaunchHostFunc(stream, callback_impl, f_copy));
    }
};

struct cuda_gpu_memory {
    CUdeviceptr ptr=0;
    uint64 size=0;

    cuda_gpu_memory subset(uint64 start, uint64 t_size) {
        assert(ptr!=0);
        assert(start>=0 && start<size);
        assert(t_size>=1 && t_size<=size-start);

        cuda_gpu_memory res;
        res.is_init=true;
        res.is_dynamic=false;
        res.ptr=ptr+start;
        res.size=t_size;

        return res;
    }
};

struct cuda_cpu_memory {
    void* ptr=0;
    uint64 size=0;

    cuda_cpu_memory subset(uint64 start, uint64 t_size) {
        assert(ptr!=0);
        assert(start>=0 && start<size);
        assert(t_size>=1 && t_size<=size-start);

        cuda_cpu_memory res;
        res.is_init=true;
        res.is_dynamic=false;
        res.ptr=((char*)ptr)+start;
        res.size=t_size;

        return res;
    }
};

struct cuda_gpu_memory_dynamic : no_copy {
    cuda_gpu_memory m;

    cuda_gpu_memory_dynamic() {}

    cuda_gpu_memory_dynamic(cuda_gpu_memory&& t) {
        m=move(t.m);
        t.m=cuda_gpu_memory();
    }

    cuda_gpu_memory_dynamic& operator=(cuda_gpu_memory_dynamic&& t) {
        *this=move(t);
        return *this;
    }

    cuda_gpu_memory_dynamic(uint64 t_size) {
        assert(size>=1);
        error_check(cuMemAlloc(&m.ptr, t_size));
        m.size=t_size;
    }

    ~cuda_gpu_memory() {
        if (m.ptr!=0) {
            error_check(cuMemFree(ptr));
        }
    }
};

struct cuda_cpu_memory_dynamic : no_copy {
    cuda_cpu_memory m;

    cuda_cpu_memory_dynamic() {}

    cuda_cpu_memory_dynamic(cuda_cpu_memory_dynamic&& t) {
        m=t.m;
        t.m=cuda_cpu_memory();
    }

    cuda_cpu_memory_dynamic& operator=(cuda_cpu_memory_dynamic&& t) {
        *this=move(t);
        return *this;
    }

    cuda_cpu_memory_dynamic(uint64 t_size) {
        error_check(cuMemAllocHost(&m.ptr, t_size));
        m.size=t_size;

        assert(size>=1);
    }

    cuda_cpu_memory(void* t_ptr, uint64 t_size) {
        is_init=true;
        is_dynamic=false;

        ptr=t_ptr;
        size=t_size;

        assert(ptr!=nullptr);
        assert(size>=1);
    }

    ~cuda_cpu_memory() {
        if (is_init && is_dynamic) {
            error_check(cuMemFreeHost(ptr));
        }
    }

    cuda_cpu_memory subset(uint64 start, uint64 t_size) {
        assert(is_init);
        assert(start>=0 && start<size);
        assert(t_size>=1 && t_size<=size-start);

        cuda_cpu_memory res;
        res.is_init=true;
        res.is_dynamic=false;
        res.ptr=((char*)ptr)+start;
        res.size=t_size;

        return res;
    }
};

void copy(cuda_stream& stream, cuda_gpu_memory& to, cuda_cpu_memory& from) {
    assert(to.is_init && from.is_init && to.size==from.size);
    assert(stream.is_init);
    error_check(cuMemcpyHtoDAsync(to.ptr, from.ptr, to.size, stream.stream));
}

void copy(cuda_stream& stream, cuda_cpu_memory& to, cuda_gpu_memory& from) {
    assert(to.is_init && from.is_init && to.size==from.size);
    assert(stream.is_init);
    error_check(cuMemcpyDtoHAsync(to.ptr, from.ptr, to.size, stream.stream));
}

struct cuda_ivec3 {
    int x=1;
    int y=1;
    int z=1;

    cuda_ivec3(int t_x=1, int t_y=1, int t_z=1) : x(t_x), y(t_y), z(t_z) {}
};

struct cuda_module : no_copy {
    bool is_init=false;
    CUmodule module;

    cuda_module() {}

    cuda_module(cuda_module&& t) {
        is_init=t.init;
        module=t.module;
        t.is_init=false;
    }

    cuda_module& operator=(cuda_module&& t) {
        *this=move(t);
        return *this;
    }

    cuda_module(const string& data) {
        is_init=true;
        error_check(cuModuleLoadData(&module, &data[0]));
    }

    ~cuda_module() {
        error_check(cuModuleUnload(&module));
    }

    cuda_gpu_memory get_global(const string& name) {
        CUdeviceptr ptr;
        size_t size;
        error_check(cuModuleGetGlobal(&ptr, &size, module, name.c_str()));
        return cuda_gpu_memory(ptr, size);
    }

    void run(
        cuda_stream& stream, const string& name,
        cuda_ivec3 grid_size, cuda_ivec3 block_size, int shared_memory_bytes, bool is_cooperative,
        const vector<vector<char>>& params
    ) {
        assert(is_init && stream.is_init);

        CUfunction function;
        error_check(cuModuleGetFunction(&function, module, name.c_str()));

        vector<void*> params_ptr;
        for (const vector<char>& c : params) {
            params_ptr.push_back((void*)&c[0]);
        }

        if (is_cooperative) {
            error_check(cuLaunchCooperativeKernel(
                function, grid_size.x, grid_size.y, grid_size.z, block_size.x, block_size.y, block_size.z,
                shared_memory_bytes, stream.stream, &params_ptr[0]
            );
        } else {
            error_check(cuLaunchKernel(
                function, grid_size.x, grid_size.y, grid_size.z, block_size.x, block_size.y, block_size.z,
                shared_memory_bytes, stream.stream, &params_ptr[0], nullptr
            );
        }
    }
};


};