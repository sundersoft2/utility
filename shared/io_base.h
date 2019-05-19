#ifndef ILYA_HEADER_IO_BASE
#define ILYA_HEADER_IO_BASE

#include <iostream>

namespace io {
    using std::ostream;
    
    template<class type> ostream& operator/(ostream& targ, const type& t_type) { return targ << t_type; }
};

#endif
