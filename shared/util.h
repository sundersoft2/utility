//shared file generic.h

#ifndef ILYA_HEADER_UTIL
#define ILYA_HEADER_UTIL

namespace util {
    struct less_ptr {
        template<class ptr_type> bool operator()(ptr_type a, ptr_type b) {
            return *a<*b;
        }
    };
}

#endif
