#ifndef ILYA_HEADER_MATH_GEOMETRY_TRANSFORMS
#define ILYA_HEADER_MATH_GEOMETRY_TRANSFORMS

#include "math_linear_vector.h"
#include "math_linear_transform.h"
#include "math_quaternion.h"

namespace math {

template<class type, int num> matrix_across_base<type, num, num> scale(vec_base<type, num>& factors) {
    matrix_across_base<type, num, num> res;
    for (int x=0;x<num;++x) res(x, x)=factors[x];
}

template<class type, int num> matrix_across_base<type, num, num> mirror(vec_base<type, num>& axis) {
    return matrix_across_base<type, num, num>()-type(2)*exterior(axis, axis);
}

template<class type> matrix_across_base<type, 2, 2> rotate(const type& sine, const type& cosine) {
    typedef vec_base<type, 2> v;
    return build_matrix_across_vec(v(cosine, -sine), v(sine, cosine));
}
template<class type> matrix_across_base<type, 2, 2> rotate(const type& angle) { return rotate(cos(angle), sin(angle)); }

template<class type> matrix_across_base<type, 3, 3> rotate_x(const type& sine, const type& cosine) {
    typedef vec_base<type, 3> v;
    return build_matrix_across_vec(v(1, 0, 0), v(0, cosine, -sine), v(0, sine, cosine));
}
template<class type> matrix_across_base<type, 3, 3> rotate_x(const type& angle) { return rotate_x(cos(angle), sin(angle)); }

template<class type> matrix_across_base<type, 3, 3> rotate_y(const type& sine, const type& cosine) {
    typedef vec_base<type, 3> v;
    return build_matrix_across_vec(v(cosine, 0, sine), v(0, 1, 0), v(-sine, 0, cosine));
}
template<class type> matrix_across_base<type, 3, 3> rotate_y(const type& angle) { return rotate_y(cos(angle), sin(angle)); }

template<class type> matrix_across_base<type, 3, 3> rotate_z(const type& sine, const type& cosine) {
    typedef vec_base<type, 3> v;
    return build_matrix_across_vec(v(cosine, -sine, 0), v(sine, cosine, 0), v(0, 0, 1));
}
template<class type> matrix_across_base<type, 3, 3> rotate_z(const type& angle) { return rotate_z(cos(angle), sin(angle)); }

template<class type> matrix_across_base<type, 3, 3> rotate(const vec_base<type, 3>& axis_unit, const type& s, const type& c) {
    typedef vec_base<type, 3> v;
    const v& u=axis_unit;
    return build_matrix_across_vec(
        v(u.x*u.x+(1-u.x*u.x)*c, u.x*u.y*(1-c)-u.z*s, u.x*u.z*(1-c)+u.y*s),
        v(u.x*u.y*(1-c)+u.z*s, u.y*u.y+(1-u.y*u.y)*c, u.y*u.z*(1-c)-u.x*s),
        v(u.x*u.y*(1-c)-u.y*s, u.y*u.z*(1-c)+u.x*s, u.z*u.z+(1-u.z*u.z)*c)
    );
}
template<class type> matrix_across_base<type, 3, 3> rotate(const vec_base<type, 3>& axis_unit, const type& angle) { return rotate(axis_unit, sin(angle), cos(angle)); }

template<class type> matrix_across_base<type, 3, 3> rotate(const quaternion<type>& targ) {
    typedef vec_base<type, 3> v;
    const type& a=targ.w;
    const type& b=targ.a.x;
    const type& c=targ.a.y;
    const type& d=targ.a.z;
    return build_matrix_across_vec(
        v(a*a+b*b-c*c-d*d, 2*b*c-2*a*d, 2*b*d+2*a*c),
        v(2*b*c+2*a*d, a*a-b*b+c*c-d*d, 2*c*d-2*a*b),
        v(2*b*d-2*a*c, 2*c*d+2*a*b, a*a-b*b-c*c+d*d)
    );
}

template<class type> vec_base<type, 3> rotate(const vec_base<type, 3>& vec, const quaternion<type>& transform) {
    return (transform*quaternion<type>(type(0), vec)*conj(transform)).a;
}

//

template<class type, int num_a, int num_b> matrix_across_base<type, num_a+1, num_b+1> homogenize(const matrix_across_base<type, num_a, num_b>& targ) {
    return targ;
}

template<class type, int num> vec_base<type, num+1> homogenize(const vec_base<type, num>& targ) {
    return vec_base<type, num+1>(targ, 1);
}

class milk {};
class homogenized_milk {};
homogenized_milk homogenize(milk targ) { return homogenized_milk(); }
homogenized_milk homogenize(homogenized_milk targ) { return homogenized_milk(); }

template<class type, int num> vec_base<type, num-1> linearize(const vec_base<type, num>& targ) {
    vec_base<type, num-1> res;
    for (int x=0;x<num-1;++x) res[x]=targ[x];
    res/=targ[num-1];
    return res;
}

template<class type, int num_a, int num_b> matrix_across_base<type, num_a+1, num_b+1> build_homogeneous(const matrix_across_base<type, num_a, num_b>& transform, const vec_base<type, num_b>& offset=vec_base<type, num_b>(), const vec_base<type, num_a>& scaling_vector=vec_base<type, num_a>(), const type& scalar=type(1)) {
    matrix_across_base<type, num_a+1, num_b+1> res=transform;
    res.across(num_a, vec_base<type, num_a+1>(scaling_vector, 0));
    res.down(num_b, vec_base<type, num_b+1>(offset, scalar));
    return res;
}

template<class type, int num> matrix_across_base<type, num+1, num+1> translate(const vec_base<type, num>& targ) {
    return build_homogeneous(matrix_across_base<type, num, num>(), targ);
}

template<class type> matrix_across_base<type, 4, 4> project_orthogonal(const type& left, const type& right, const type& bottom, const type& top, const type& near, const type& far) {
    typedef vec_base<type, 4> v;
    return build_matrix_across_vec(
        v(2/(right-left),              0,             0, -(right+left)/(right-left)),
        v(             0, 2/(top-bottom),             0, -(top+bottom)/(top-bottom)),
        v(             0,              0, -2/(far-near),     -(far+near)/(far-near)),
        v(             0,              0,             0,                          1)
    );
}

template<class type> matrix_across_base<type, 4, 4> inverse_project_orthogonal(const type& left, const type& right, const type& bottom, const type& top, const type& near, const type& far) {
    typedef vec_base<type, 4> v;
    return build_matrix_across_vec(
        v((right-left)/2,              0,             0, (right+left)/2),
        v(             0, (top-bottom)/2,             0, (top+bottom)/2),
        v(             0,              0, -(far-near)/2,  -(far+near)/2),
        v(             0,              0,             0,              1)
    );
}

template<class type> matrix_across_base<type, 4, 4> project_frustum(const type& left, const type& right, const type& bottom, const type& top, const type& near, const type& far) {
    typedef vec_base<type, 4> v;
    return build_matrix_across_vec(
        v(2*near/(right-left),                   0, (right+left)/(right-left),                      0),
        v(                  0, 2*near/(top-bottom), (top+bottom)/(top-bottom),                      0),
        v(                  0,                   0,    -(far+near)/(far-near), -2*far*near/(far-near)),
        v(                  0,                   0,                        -1,                      0)
    );
}

template<class type> matrix_across_base<type, 4, 4> inverse_project_frustum(const type& left, const type& right, const type& bottom, const type& top, const type& near, const type& far) {
    typedef vec_base<type, 4> v;
    return build_matrix_across_vec(
        v((right-left)/near/2,                   0,                        0,     (right+left)/near/2),
        v(                  0, (top-bottom)/near/2,                        0,     (top+bottom)/near/2),
        v(                  0,                   0,                        0,                      -1),
        v(                  0,                   0, -(far-near)/(far*near)/2, (far+near)/(far*near)/2)
    );
}

//

template<class type, int num> matrix_across_base<type, 1, num> expand_1d(const vec_base<type, num>& axis) {
    return build_matrix_down_vec(axis);
}

template<class type, int num> matrix_across_base<type, 2, num+1> expand_1d(const vec_base<type, num>& origin, const vec_base<type, num>& axis) {
    return build_homogeneous(expand_1d(axis), origin);
}

template<class type> matrix_across_base<type, 2, 3> expand_2d(const vec_base<type, 3>& x_axis, const vec_base<type, 3>& y_axis) {
    return build_matrix_down_vec(x_axis, y_axis);
}

template<class type> matrix_across_base<type, 3, 4> expand_2d(const vec_base<type, 3>& origin, const vec_base<type, 3>& x_axis, const vec_base<type, 3>& y_axis) {
    return build_homogeneous(expand_2d(x_axis, y_axis), origin);
}

template<class type> matrix_across_base<type, 2, 1> contract_2d(const vec_base<type, 2>& axis) {
    return build_matrix_across_vec(axis);
}

template<class type> matrix_across_base<type, 3, 2> contract_2d(const vec_base<type, 2>& origin, const vec_base<type, 2>& axis) {
    auto t=contract_2d(axis);
    return build_homogeneous(t, -t*origin);
}

template<class type> matrix_across_base<type, 3, 1> contract_3d(const vec_base<type, 3>& axis) {
    return build_matrix_across_vec(axis);
}

template<class type> matrix_across_base<type, 4, 2> contract_3d(const vec_base<type, 3>& origin, const vec_base<type, 3>& axis) {
    auto t=contract_3d(axis);
    return build_homogeneous(t, -t*origin);
}

template<class type> matrix_across_base<type, 3, 2> contract_3d(const vec_base<type, 3>& x_axis, const vec_base<type, 3>& y_axis) {
    return build_matrix_across_vec(x_axis, y_axis);
}

template<class type> matrix_across_base<type, 4, 3> contract_3d(const vec_base<type, 3>& origin, const vec_base<type, 3>& x_axis, const vec_base<type, 3>& y_axis) {
    auto t=contract_3d(x_axis, y_axis);
    return build_homogeneous(t, -t*origin);
}

}

#endif
