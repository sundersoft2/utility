#ifndef ILYA_HEADER_MATH_BASE
#define ILYA_HEADER_MATH_BASE

#include <shared/generic.h>

namespace math {
template<class type> type abs(const type& targ) { return targ<type(0)? -targ : targ; }
template<> float abs<float>(const float& targ) { return std::abs(targ); }
template<> double abs<double>(const double& targ) { return std::abs(targ); }
template<> long double abs<long double>(const long double& targ) { return std::abs(targ); }
template<class type> type square(const type& targ) { return targ*targ; }
template<class type> type sgn(const type& t) { return t>type(0)? type(1) : (t<type(0)? type(-1) : type(0)); }
template<class type> type inf() { return std::numeric_limits<type>::infinity(); }
template<class type> type nan() { return std::numeric_limits<type>::quiet_NaN(); }
double inf() { return inf<double>(); }
double nan() { return nan<double>(); }
template<class type> type clamp(const type& targ, const type& min=type(0), const type& max=type(1)) {
    if (!(targ>min)) return min;
    if (!(targ<max)) return max;
    return targ;
}
template<class type> type lerp(const type& a, const type& b, const type& t) { return a + t*(b-a); }

const double pi=3.14159265358979323846;
const double e=2.71828182845904523536;

template<class type> type deg_to_rad(const type& targ) { return targ*type(pi/180.0); }
template<class type> type rad_to_deg(const type& targ) { return targ*type(180.0/pi); }

int exp2(int targ) { return 1<<targ; }
int log2(int targ) {
    int res=0;
    while(targ>>=1) ++res;
    return res;
}
bool is2(int targ) {
    while(targ) {
        if ((targ&1) && (targ^1)) return 0;
        targ>>=1;
    }
    return 1;
}

double rand() {
    return double(std::rand())*(1.0/RAND_MAX);
}

template<class type> type& times_equals(type& a, const type& b) { return a*=b; }
template<class type, class func_type=type&(*)(type&, const type&)> type peasant(const type& targ, int num, const type& identity, func_type func=times_equals<type>) {
    type res;
    type cache=targ;
    int c_num=num;
    int x=1;
    bool res_set=0;
    while(1) {
        if (c_num&x) {
            if (res_set==0) {
                res=cache;
                res_set=1;
            } else {
                func(res, cache);
            }
            c_num^=x;
        }
        if (c_num!=0) {
            func(cache, cache);
            x<<=1;
        } else break;
    }
    return res_set? res : identity;
}

/*using generic::only_if; //fix this later
using generic::assert_true;
using generic::only_if_same_types;
using generic::assert_same_types;
using generic::static_abs;
using generic::static_sgn;
using generic::static_max;
using generic::static_min;*/
}

#endif
