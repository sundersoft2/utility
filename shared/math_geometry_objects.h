#ifndef ILYA_HEADER_MATH_GEOMETRY_OBJECTS
#define ILYA_HEADER_MATH_GEOMETRY_OBJECTS

#include "math_linear_vector.h"

namespace math {

template<class type, int num> struct point_base {
    typedef int primitive_tag;
    vec_base<type, num> pos;
    
    point_base() {}
    point_base(const vec_base<type, num>& targ) : pos(targ) {}
    
    operator vec_base<type, num>() {
        return pos;
    }
};


class lineoid_line_tag {};
class lineoid_ray_tag {};
class lineoid_segment_tag {};
template<class tag, class type, int num> struct lineoid_base {
    typedef int primitive_tag;
    vec_base<type, num> origin;
    vec_base<type, num> tangent;
    
    lineoid_base() {}
    lineoid_base(const vec_base<type, num>& t_origin, const vec_base<type, num>& t_tangent) : origin(t_origin), tangent(t_tangent) {}
    lineoid_base(const point_base<type, num>& t_origin, const vec_base<type, num>& t_tangent) : origin(t_origin.pos), tangent(t_tangent) {}

    point_base<type, num> operator()(type t) {
        return origin + t*tangent;
    }
    
    point_base<type, num> start() {
        return origin;
    }
    
    point_base<type, num> end() {
        return origin+tangent;
    }
};


template<class type, int num> struct segment_or_ray_base {
    typedef int primitive_tag;
    bool is_ray;
    vec_base<type, num> origin;
    vec_base<type, num> tangent;
    
    segment_or_ray_base() : is_ray(0) {}
    segment_or_ray_base(bool t_is_ray, const vec_base<type, num>& t_origin, const vec_base<type, num>& t_tangent) : is_ray(t_is_ray), origin(t_origin), tangent(t_tangent) {}
    segment_or_ray_base(bool t_is_ray, const point_base<type, num>& t_origin, const vec_base<type, num>& t_tangent) : is_ray(t_is_ray), origin(t_origin.pos), tangent(t_tangent) {}

    point_base<type, num> operator()(type t) {
        return origin + t*tangent;
    }
};


template<class type> struct plane_base {
    typedef int primitive_tag;
    vec_base<type, 3> normal;
    type offset;
    
    plane_base() : offset(type(0)) {}
    plane_base(const vec_base<type, 3>& t_normal, const type& t_offset) : normal(t_normal), offset(t_offset) {}
    plane_base(const point_base<type, 3>& pos, const vec_base<type, 3>& t_normal) : normal(t_normal), offset(dot(t_normal, pos.pos)) {}
    plane_base(const lineoid_base<lineoid_line_tag, type, 3>& t_line, const vec_base<type, 3>& t_tangent) : normal(cross(t_line.tangent, t_tangent)) {
        offset=dot(normal, t_line.origin.pos);
    }
};


template<class type, int num> struct half_space_base {
    typedef int primitive_tag;
    vec_base<type, num> normal; //points to empty region
    //i.e. point is in half space if dot(p, normal)<offset
    type offset;
};


template<class type> class half_space_base<type, 2> {
    void ctor(const lineoid_base<lineoid_line_tag, type, 2>& targ, const vec_base<type, 2>& v, bool use_o) {
        normal.x=-targ.tangent.y;
        normal.y=targ.tangent.x;
        offset=dot(normal, targ.origin);
        if (dot(v, normal)<(use_o? offset : 0)) {
            offset=-offset;
            normal=-normal;
        }
    }
    void ctor(const half_space_base& targ, const vec_base<type, 2>& v, bool use_o) {
        normal=targ.normal;
        offset=targ.offset;
        if (dot(v, normal)<(use_o? offset : 0)) {
            offset=-offset;
            normal=-normal;
        }
    }
    
    public:
    typedef int primitive_tag;
    vec_base<type, 2> normal;
    type offset;
    
    half_space_base() : offset(type(0)) {}
    half_space_base(const lineoid_base<lineoid_line_tag, type, 2>& targ, const vec_base<type, 2>& points_to_empty) {
        ctor(targ, points_to_empty, 0);
    }
    half_space_base(const lineoid_base<lineoid_line_tag, type, 2>& targ, const point_base<type, 2>& empty_point) {
        ctor(targ, empty_point.pos, 1);
    }
    half_space_base(const half_space_base& targ, const vec_base<type, 2>& points_to_empty) {
        ctor(targ, points_to_empty, 0);
    }
    half_space_base(const half_space_base& targ, const point_base<type, 2>& empty_point) {
        ctor(targ, empty_point.pos, 1);
    }
};


template<class type> class half_space_base<type, 3> {
    void ctor(const vec_base<type, 3>& v, bool use_o) {
        if (dot(v, normal)<(use_o? offset : 0)) {
            offset=-offset;
            normal=-normal;
        }
    }
    
    public:
    typedef int primitive_tag;
    vec_base<type, 3> normal;
    type offset;
    
    half_space_base() : offset(type(0)) {}
    half_space_base(const plane_base<type>& targ) : normal(targ.normal), offset(targ.offset) {}
    half_space_base(const plane_base<type>& targ, const vec_base<type, 3>& points_to_empty) : normal(targ.normal), offset(targ.offset) {
        ctor(points_to_empty, 0);
    }
    half_space_base(const plane_base<type>& targ, const point_base<type, 3>& empty_point) : normal(targ.normal), offset(targ.offset) {
        ctor(empty_point.pos, 1);
    }
    half_space_base(const half_space_base& targ, const vec_base<type, 3>& points_to_empty) : normal(targ.normal), offset(targ.offset) {
        ctor(points_to_empty, 0);
    }
    half_space_base(const half_space_base& targ, const point_base<type, 3>& empty_point) : normal(targ.normal), offset(targ.offset) {
        ctor(empty_point.pos, 1);
    }
};


template<class type> struct optional_base {
    typedef int primitive_tag;
    type data;
    bool exists;
    //
    optional_base() : exists(0) {}
    optional_base(const type& t_data) : data(t_data), exists(1) {}
};

namespace geometry_double {
    typedef half_space_base<double, 2> half_space_2d;
    typedef half_space_base<double, 3> half_space;
    typedef plane_base<double> plane;
    typedef lineoid_base<lineoid_line_tag, double, 2> line_2d;
    typedef lineoid_base<lineoid_ray_tag, double, 2> ray_2d;
    typedef lineoid_base<lineoid_segment_tag, double, 2> segment_2d;
    typedef lineoid_base<lineoid_line_tag, double, 3> line;
    typedef lineoid_base<lineoid_ray_tag, double, 3> ray;
    typedef lineoid_base<lineoid_segment_tag, double, 3> segment;
    typedef point_base<double, 2> point_2d;
    typedef point_base<double, 3> point;
}

namespace geometry_float {
    typedef half_space_base<float, 2> half_space_2d;
    typedef half_space_base<float, 3> half_space;
    typedef plane_base<float> plane;
    typedef lineoid_base<lineoid_line_tag, float, 2> line_2d;
    typedef lineoid_base<lineoid_ray_tag, float, 2> ray_2d;
    typedef lineoid_base<lineoid_segment_tag, float, 2> segment_2d;
    typedef lineoid_base<lineoid_line_tag, float, 3> line;
    typedef lineoid_base<lineoid_ray_tag, float, 3> ray;
    typedef lineoid_base<lineoid_segment_tag, float, 3> segment;
    typedef point_base<float, 2> point_2d;
    typedef point_base<float, 3> point;
}

}

#endif
