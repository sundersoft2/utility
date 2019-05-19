#ifndef ILYA_HEADER_MATH_QUATERNION
#define ILYA_HEADER_MATH_QUATERNION

#include <cmath>
#include <utility>
#include <iostream>

#include "math_base.h"

namespace math {
using std::sin;
using std::cos;
using std::acos;
using std::sqrt;
using std::isnan;
using std::pow;
using std::ostream;

template<class type> struct quaternion {
    type w;
    vec_base<type, 3> a;
    //
    quaternion() : w(type(0)) {}
    quaternion(const type& t_w, const vec_base<type, 3>& t_a) : w(t_w), a(t_a) {}
    quaternion(const type& t_w, const type& t_x, const type& t_y, const type& t_z) : w(t_w), a(t_x, t_y, t_z) {}
    quaternion(const vec_base<type, 3>& t_a) : w(type(0)), a(t_a) {}
    quaternion(const type& t_w) : w(t_w) {}
    //
    quaternion& operator=(const vec_base<type, 3>& t_a) { w=type(0); a=t_a; return *this; }
    quaternion& operator=(const type& t_w) { w=t_w; a=a(); return *this; }
};

template<class type> quaternion<type> rotation_quaternion(const vec_base<type, 3>& axis, const type& angle) {
    return quaternion<type>(cos(angle/2.0), axis*sin(angle/2.0));
}

template<class type> quaternion<type> rotation_quaternion(const vec_base<type, 3>& axis, bool sine_positive, const type& cosine) {
    type half_sine=sqrt((1-cosine)/2);
    type half_cosine=sqrt((cosine+1)/2)*(sine_positive? 1 : -1);
    return quaternion<type>(half_cosine, axis*half_sine);
}

template<class type> quaternion<type> rotation_quaternion(const vec_base<type, 3>& axis, const type& sine, const type& cosine) {
    return rotation_quaternion(axis, sine>0, cosine);
}

template<class type> quaternion<type>& operator+=(quaternion<type>& targ, const quaternion<type>& b) {
    targ.w+=b.w;
    targ.a+=b.a;
    return targ;
}
template<class type> quaternion<type>& operator-=(quaternion<type>& targ, const quaternion<type>& b) {
    targ.w-=b.w;
    targ.a-=b.a;
    return targ;
}
template<class type> quaternion<type> operator+(quaternion<type> a, const quaternion<type>& b) { return a+=b; }
template<class type> quaternion<type> operator-(quaternion<type> a, const quaternion<type>& b) { return a-=b; }

template<class type> quaternion<type> operator+(const quaternion<type>& targ) { return targ; }
template<class type> quaternion<type> operator-(quaternion<type> targ) { targ.w=-targ.w; targ.a=-targ.a; return targ; }

template<class type> quaternion<type>& operator+=(quaternion<type>& targ, const type& b) { targ.w+=b; return targ; }
template<class type> quaternion<type>& operator-=(quaternion<type>& targ, const type& b) { targ.w-=b; return targ; }
template<class type> quaternion<type> operator+(quaternion<type> a, const type& b) { return a+=b; }
template<class type> quaternion<type> operator+(const type& a, const quaternion<type>& b) { return b+=a; }
template<class type> quaternion<type> operator-(quaternion<type> a, const type& b) { return a-=b; }
template<class type> quaternion<type> operator-(const type& a, const quaternion<type>& b) { return b-=a; }

template<class type> quaternion<type>& operator*=(quaternion<type>& targ, const type& b) {
    targ.w*=b;
    targ.a*=b;
    return targ;
}
template<class type> quaternion<type>& operator/=(quaternion<type>& targ, const type& b) {
    targ.w/=b;
    targ.a/=b;
    return targ;
}
template<class type> quaternion<type> operator*(quaternion<type> a, const type& b) { return a*=b; }
template<class type> quaternion<type> operator/(quaternion<type> a, const type& b) { return a/=b; }

template<class type> quaternion<type> operator*(const quaternion<type>& a, const quaternion<type>& b) {
    return quaternion<type>(a.w*b.w-dot(a.a, b.a), a.w*b.a+b.w*a.a+cross(a.a, b.a));
}
template<class type> quaternion<type>& operator*=(quaternion<type>& targ, const quaternion<type>& a) { return targ=targ*a; }

//

template<class type> type square_length(const quaternion<type>& targ) { return targ.w*targ.w+square_length(targ.a); }
template<class type> type square_norm(const quaternion<type>& targ) { return square_length(targ); }

template<class type> type length(const quaternion<type>& targ) { return sqrt(square_length(targ)); }
template<class type> type norm(const quaternion<type>& targ) { return length(targ); }

template<class type> quaternion<type> normalize(const quaternion<type>& targ) { return targ/length(targ); }

template<class type> quaternion<type> conj(const quaternion<type>& targ) { return quaternion<type>(targ.w, -targ.a); }

template<class type> quaternion<type> inverse(const quaternion<type>& targ) { return conj(targ)/square_length(targ); }

template<class type> quaternion<type> operator/(const quaternion<type>& a, const quaternion<type>& b) { return a*inverse(b); }
template<class type> quaternion<type>& operator/=(quaternion<type>& a, const quaternion<type>& b) { return a*=inverse(b); }

//

template<class type> class quaternion_power {
    type norm, angle;
    vec_base<type, 3> axis;
    
    public:
    quaternion_power(const type& t_norm, const type& t_angle, const vec_base<type, 3>& t_axis) : norm(t_norm), angle(t_angle), axis(t_axis) {}
    
    quaternion<type> operator()(const type& t) {
        type angle_t=angle*t;
        type factor=std::pow(norm, t);
        return quaternion<type>(cos(angle_t)*factor, sin(angle_t)*axis*factor);
    }
    quaternion<type> pow(const type& t) { return operator()(t); }
};

template<class type> class unit_quaternion_power {
    type angle;
    vec_base<type, 3> axis;
    
    public:
    unit_quaternion_power(const type& t_angle, const vec_base<type, 3>& t_axis) : angle(t_angle), axis(t_axis) {}
    //
    quaternion<type> operator()(const type& t) {
        return quaternion<type>(cos(angle*t), sin(angle*t)*axis);
    }
    quaternion<type> pow(const type& t) { return operator()(t); }
};

template<class type> quaternion_power<type> delayed_pow(const quaternion<type>& t) {
    if (t.a.x==type(0) && t.a.y==type(0) && t.a.z==type(0)) {
        if (t.w==type(0)) {
            return quaternion_power<type>(type(0), type(0), vec_base<type, 3>());
        } else {
            return quaternion_power<type>(abs(t.w), (t.w>type(0))? type(0) : type(pi), vec_base<type, 3>());
        }
    } else {
        type norm=length(t);
        return quaternion_power<type>(norm, acos(t.w/norm), normalize(t.a));
    }
}

template<class type> unit_quaternion_power<type> delayed_pow_unit(const quaternion<type>& t) {
    if (t.a.x==type(0) && t.a.y==type(0) && t.a.z==type(0)) {
        return unit_quaternion_power<type>((t.w>type(0))? type(0) : type(pi), vec_base<type, 3>());
    } else {
        return unit_quaternion_power<type>(acos(t.w), normalize(t.a));
    }
}

template<class type> quaternion<type> pow(const quaternion<type>& a, const type& b) { return delayed_pow(a)(b); }
template<class type> quaternion<type> pow_unit(const quaternion<type>& a, const type& b) { return delayed_pow_unit(a)(b); }

//

template<class type> ostream& operator<<(ostream& out, const quaternion<type>& targ) {
    print_as_number(out, targ.w);
    out << " + " << targ.a;
    return out;
}

namespace quaternion_double {
    typedef quaternion<double> q;
    static q i(0, 1, 0, 0);
    static q j(0, 0, 1, 0);
    static q k(0, 0, 0, 1);
}

namespace quaternion_float {
    typedef quaternion<float> q;
    static q i(0, 1, 0, 0);
    static q j(0, 0, 1, 0);
    static q k(0, 0, 0, 1);
}

}

#endif
