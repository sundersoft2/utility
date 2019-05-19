#ifndef ILYA_HEADER_MATH_LINEAR_VECTOR
#define ILYA_HEADER_MATH_LINEAR_VECTOR

#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>

#include "generic.h"
#include "fixed_size.h"

#include "math_base.h"

namespace math {
using std::ostream;
using std::sqrt;
using std::min;
using generic::wrap_type;
using generic::print_as_number;
using generic::assert_true;
using fixed_size::u8;

namespace not_public {
    template<class type, int num> struct vec_base_names {
        union {
            type data[num];
            struct {
                union {
                    type x, r, u;
                };
                union {
                    type y, g, v;
                };
                union {
                    type z, b;
                };
                union {
                    type w, a, t;
                };
            };
        };
    };
    template<class type> struct vec_base_names<type, 1> {
        union {
            type data[1];
            union {
                type x;
            };
        };
        operator type() const { return x; }
    };
    template<class type> struct vec_base_names<type, 2> {
        union {
            type data[2];
            struct {
                union {
                    type x, r, u;
                };
                union {
                    type y, g, v;
                };
            };
        };
    };
    template<class type> struct vec_base_names<type, 3> {
        union {
            type data[3];
            struct {
                union {
                    type x, r, u;
                };
                union {
                    type y, g, v;
                };
                union {
                    type z, b;
                };
            };
        };
    };
}

template<class type, int t_num> class vec_base : public not_public::vec_base_names<type, t_num> {
    typedef not_public::vec_base_names<type, t_num> base;
    template<int pos> void constructor() { assert_true<pos == t_num>; }
    template<int pos, int t_num2, class... t_types> void constructor(const vec_base<type, t_num2>& targ, const t_types&... targs) { assert_true<pos+t_num2 <= t_num>;
        for (int x=0;x<t_num2;++x) {
            base::data[pos+x]=targ[x];
        }
        constructor<pos+t_num2>(targs...);
    }
    template<int pos, class... t_types> void constructor(const type& targ, const t_types&... targs) { assert_true<pos <= t_num-1>;
        base::data[pos]=targ;
        constructor<pos+1>(targs...);
    }
    
    template<int offset> vec_base<type, offset> swizzle_base() const {
        return vec_base<type, offset>();
    }
    template<int offset, int pos, int... poses> vec_base<type, sizeof...(poses)+offset+1> swizzle_base() const {
        assert_true<pos>=0 && pos<t_num>;
        auto previous=swizzle_base<offset+1, poses...>();
        previous[offset]=base::data[pos];
        return previous;
    }
    
    template<int t_num2, int offset> void swizzle_base_2(const vec_base<type, t_num2>& targ) {
        assert_true<offset==t_num2>;
    }
    template<int t_num2, int offset, int pos, int... poses> void swizzle_base_2(const vec_base<type, t_num2>& targ) {
        assert_true<pos>=0 && pos<t_num>;
        assert_true<offset<t_num2>;
        base::data[pos]=targ[offset];
        swizzle_base_2<t_num2, offset+1, poses...>(targ);
    }
    
    public:
    static const int num=t_num;
    
    vec_base() { for (int i=0;i<num;++i) base::data[i]=type(0); }
    template<class type_a, class type_b, class... t_types> vec_base(const type_a& targ_a, const type_b& targ_b, const t_types&... targs) { constructor<0>(targ_a, targ_b, targs...); }
    template<class... t_types> vec_base(const type& targ, const t_types&... targs) { constructor<0>(targ, targs...); }
    
    type& operator[](int pos) { return base::data[pos]; }
    const type& operator[](int pos) const { return base::data[pos]; }
    
    int size() const { return t_num; }
    type* begin() { return base::data; }
    const type* begin() const { return base::data; }
    type* end() { return base::data+t_num; }
    const type* end() const { return base::data+t_num; }
    
    template<int... poses> vec_base<type, sizeof...(poses)> swizzle() const { return swizzle_base<0, poses...>(); }
    template<int... poses, int t_num2> vec_base& swizzle(const vec_base<type, t_num2>& targ) {
        swizzle_base_2<t_num2, 0, poses...>(targ);
        return *this;
    }
};

template<class type> struct vec_traits {
    template<class tt_type> struct only_if {};
};
template<class t_type, int t_num> struct vec_traits<vec_base<t_type, t_num>> {
    static const int num=t_num;
    typedef t_type type;
    template<int tt_num> struct make_vec {
        typedef vec_base<t_type, tt_num> res;
    };
    template<class tt_type> struct only_if {
        typedef tt_type good;
    };
};

//

template<class type, int num> vec_base<type, num>& operator+=(vec_base<type, num>& a, const vec_base<type, num>& b) {
    for (int x=0;x<num;++x) {
        a[x]+=b[x];
    }
    return a;
}
template<class type, int num> vec_base<type, num>& operator-=(vec_base<type, num>& a, const vec_base<type, num>& b) {
    for (int x=0;x<num;++x) {
        a[x]-=b[x];
    }
    return a;
}

template<class type, int num> vec_base<type, num>& operator*=(vec_base<type, num>& a, const type& b) {
    for (int x=0;x<num;++x) {
        a[x]*=b;
    }
    return a;
}
template<class type, int num> vec_base<type, num>& operator/=(vec_base<type, num>& a, const type& b) {
    for (int x=0;x<num;++x) {
        a[x]/=b;
    }
    return a;
}

template<class type, int num> bool operator==(const vec_base<type, num>& a, const vec_base<type, num>& b) {
    for (int x=0;x<num;++x) {
        if (a[x]!=b[x]) {
            return false;
        }
    }
    return true;
}

template<class type, int num> bool operator!=(const vec_base<type, num>& a, const vec_base<type, num>& b) {
    return !(a==b);
}

template<class type, int num> vec_base<type, num> operator*(vec_base<type, num> a, const type& b) { return a*=b; }
template<class type, int num> vec_base<type, num> operator*(const type& b, vec_base<type, num> a) { return a*=b; }
template<class type, int num> vec_base<type, num> operator/(vec_base<type, num> a, const type& b) { return a/=b; }

template<class type, int num> vec_base<type, num> operator+(vec_base<type, num> a, const vec_base<type, num>& b) { return a+=b; }
template<class type, int num> vec_base<type, num> operator-(vec_base<type, num> a, const vec_base<type, num>& b) { return a-=b; }

template<class type, int num> vec_base<type, num> operator-(vec_base<type, num> a) {
    for (int x=0;x<num;++x) {
        a[x]=-a[x];
    }
    return a;
}

//

template<class type, int num> type dot(const vec_base<type, num>& a, const vec_base<type, num>& b) {
    type res=0;
    for (int x=0;x<num;++x) {
        res+=a[x]*b[x];
    }
    return res;
}

//

template<class type> vec_base<type, 3> cross(const vec_base<type, 3>& a, const vec_base<type, 3>& b) {
    vec_base<type, 3> res;
    res.x=a.y*b.z-a.z*b.y;
    res.y=a.z*b.x-a.x*b.z;
    res.z=a.x*b.y-a.y*b.x;
    return res;
}

//

template<class type> vec_base<type, 2> turn(const vec_base<type, 2>& a) {
    vec_base<type, 2> res;
    res.x=-a.y;
    res.y=a.x;
    return res;
}

//

template<class type, int num> type inverse_multiple(const vec_base<type, num>& a, const vec_base<type, num>& b) {
    int max=0;
    type v_max=abs(a[0]);
    for (int x=1;x<num;++x) {
        if (abs(a[x])>v_max) max=x, v_max=abs(a[x]);
    }
    return a[max]/b[max];
}

//

template<class type, int num> type square_length(const vec_base<type, num>& targ) {
    type res=0;
    for (int x=0;x<num;++x) {
        res+=square(targ[x]);
    }
    return res;
}
//
template<class type, int num> type square_norm(const vec_base<type, num>& targ) { return square_length(targ); }

template<class type, int num> type length(const vec_base<type, num>& targ) {
    return sqrt(square_length(targ));
}
//
template<class type, int num> type norm(const vec_base<type, num>& targ) { return length(targ); }

template<class type, int num> vec_base<type, num> normalize(vec_base<type, num> targ) {
    type div=length(targ);
    for (int x=0;x<num;++x) {
        targ[x]/=div;
    }
    return targ;
}

template<class type, int num, class func_type> auto apply(const vec_base<type, num>& targ, func_type func) -> vec_base<decltype(func(type())), num> {
    vec_base<decltype(func(type())), num> res;
    for (int x=0;x<num;++x) {
        res[x]=func(targ[x]);
    }
    return res;
}
template<class type, int num, class func_type> auto apply_locate(const vec_base<type, num>& targ, func_type func) -> vec_base<decltype(func(type(), 0)), num> {
    vec_base<decltype(func(type(), 0)), num> res;
    for (int x=0;x<num;++x) {
        res[x]=func(targ[x], x);
    }
    return res;
}

template<class type, int num> ostream& operator<<(ostream& out, const vec_base<type, num>& v) {
    out << "<";
    for (int x=0;x<num;++x) {
        if (x>0) out << ", ";
        print_as_number(out, v[x]);
    }
    out << ">";
    return out;
}

namespace vec_double {
    typedef vec_base<double, 1> v1;
    typedef vec_base<double, 2> v2;
    typedef vec_base<double, 3> v3;
    typedef vec_base<double, 4> v4;
}

namespace vec_float {
    typedef vec_base<float, 1> v1;
    typedef vec_base<float, 2> v2;
    typedef vec_base<float, 3> v3;
    typedef vec_base<float, 4> v4;
    
    template<int num> vec_base<float, num> vec_u8_to_float_norm(const vec_base<u8, num>& targ) {
        return apply(targ, [](u8 t) -> float{return float(t)/255.0f;});
    }
    template<int num> vec_base<u8, num> vec_float_to_u8_norm(const vec_base<float, num>& targ) {
        return apply(targ, [](float t) -> u8{return u8(clamp(t)*255.0f+0.5f);});
    }
    template<int num> vec_base<float, num> vec_u8_to_float_snorm(const vec_base<u8, num>& targ) {
        return apply(targ, [](u8 t) -> float{return float(t)*(1.0f/255.0f*2.0f)-1.0f;});
    }
    template<int num> vec_base<u8, num> vec_float_to_u8_snorm(const vec_base<float, num>& targ) {
        return apply(targ, [](float t) -> u8{return u8((clamp(t, -1.0f, 1.0f)+1.0f)*(255.0f/2.0f));});
    }
}

}

#endif
