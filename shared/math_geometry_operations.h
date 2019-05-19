#ifndef ILYA_HEADER_MATH_GEOMETRY_OPERATIONS
#define ILYA_HEADER_MATH_GEOMETRY_OPERATIONS

#include <algorithm>
#include <cmath>

#include "generic.h"

#include "math_linear_vector.h"
#include "math_linear_transform.h"

#include "math_geometry_transforms.h"
#include "math_geometry_objects.h"

namespace math {
using std::min;
using std::max;

template<class type> type convex_hull(const type& a) { { typename type::primitive_tag assertion; }
    return a;
}

template<class type, int num> lineoid_base<lineoid_segment_tag, type, num> convex_hull(const point_base<type, num>& a, const point_base<type, num>& b) {
    return lineoid_base<lineoid_segment_tag, type, num>(a.pos, b.pos-a.pos);
}

template<class type> half_space_base<type, 2> convex_hull(const lineoid_base<lineoid_line_tag, type, 2>& a, const lineoid_base<lineoid_ray_tag, type, 2>& b) {
    half_space_base<type, 2> res;
    res.normal.x=-a.tangent.y;
    res.normal.y=a.tangent.x;
    if (dot(res.normal, b.tangent)<0) {
        res.normal=-res.normal;
    }
    res.offset=max(dot(res.normal, a.origin), dot(res.normal, b.origin));
}

template<class type> half_space_base<type, 2> convex_hull(const lineoid_base<lineoid_ray_tag, type, 2>& a, const lineoid_base<lineoid_line_tag, type, 2>& b) {
    return convex_hull(b, a);
}

template<class type> half_space_base<type, 3> convex_hull(const plane_base<type>& a, const lineoid_base<lineoid_ray_tag, type, 3>& b) {
    half_space_base<type, 3> res(a, b.tangent);
    res.offset=max(res.offset, dot(res.normal, b.origin));
}

template<class type> half_space_base<type, 3> convex_hull(const lineoid_base<lineoid_ray_tag, type, 3>& a, const plane_base<type>& b) {
    return convex_hull(b, a);
}

//

namespace not_for_public_consumption {
    template<class t_type> struct affine_hull_base_class {
        static t_type run(const t_type& targ) { { typename t_type::primitive_tag assertion; }
            return targ;
        }
    };
    
    template<class line_type, class type, int num> struct affine_hull_base_class<lineoid_base<line_type, type, num>> {
        static lineoid_base<lineoid_line_tag, type, num> run(const lineoid_base<line_type, type, num>& targ) {
            return lineoid_base<lineoid_line_tag, type, num>(targ.origin, targ.tangent);
        }
    };
    
    template<class type, int num> struct affine_hull_base_class<segment_or_ray_base<type, num>> {
        static lineoid_base<lineoid_line_tag, type, num> run(const segment_or_ray_base<type, num>& targ) {
            return lineoid_base<lineoid_line_tag, type, num>(targ.origin, targ.tangent);
        }
    };
    
    template<class t_type> struct affine_hull_base_class<optional_base<t_type>> {
        static auto run(const optional_base<t_type>& targ) -> optional_base<decltype(affine_hull_base_class<t_type>::run(targ.data))> {
            typedef optional_base<decltype(affine_hull_base_class<t_type>::run(targ.data))> r;
            if (targ.exists) {
                return r(affine_hull_base_class<t_type>::run(targ.data));
            } else {
                return r();
            }
        }
    };
    
    template<class type> auto affine_hull_base(const type& a) -> decltype(affine_hull_base_class<type>::run(a)) {
        return affine_hull_base_class<type>::run(a);
    }
    
    template<class type, int num> lineoid_base<lineoid_line_tag, type, num> affine_hull_base(const point_base<type, num>& a, const point_base<type, num>& b) {
        return lineoid_base<lineoid_line_tag, type, num>(a.pos, b.pos-a.pos);
    }
    
    template<class type, class line_type> plane_base<type> affine_hull_base(const point_base<type, 3>& a, const lineoid_base<line_type, type, 3>& b) {
        plane_base<type> res;
        res.normal=cross(b.tangent, a.pos-b.origin);
        res.offset=dot(res.normal, a.pos);
        return res;
    }
    
    template<class type> plane_base<type> affine_hull_base(const point_base<type, 3>& a, const segment_or_ray_base<type, 3>& b) {
        return affine_hull_base(a, lineoid_base<lineoid_line_tag, type, 3>(b.origin, b.tangent));
    }
    
    template<class type, class line_type> plane_base<type> affine_hull_base(const lineoid_base<line_type, type, 3>& a, const point_base<type, 3>& b) {
        return affine_hull_base(b, a);
    }
    
    template<class type> plane_base<type> affine_hull_base(const segment_or_ray_base<type, 3>& a, const point_base<type, 3>& b) {
        return affine_hull_base(b, a);
    }
}

template<class type> auto affine_hull(const type& a) -> decltype(not_for_public_consumption::affine_hull_base(a)) {
    return not_for_public_consumption::affine_hull_base(a);
}

template<class type_a, class type_b, class... types> auto affine_hull(const type_a& a, const type_b& b, const types&... targs)
    -> decltype(affine_hull(not_for_public_consumption::affine_hull_base(a, b), targs...))
{
    return affine_hull(not_for_public_consumption::affine_hull_base(a, b), targs...);
}

//

namespace not_for_public_consumption {
    template<class type> type intersection_base(const type& targ) { { typename type::primitive_tag assertion; }
        return targ;
    }
    
    template<class type, int num> optional_base<point_base<type, num>> intersection_base(const point_base<type, num>& a, const half_space_base<type, num>& b) {
        typedef optional_base<point_base<type, num>> r;
        if (dot(b.normal, a.pos)<b.offset) {
            return r(a.pos);
        } else {
            return r();
        }
    }
    
    template<class type, int num> optional_base<point_base<type, num>> intersection_base(const half_space_base<type, num>& a, const point_base<type, num>& b) {
        return intersection_base(b, a);
    }
    
    template<class type, int num> type find_t(const vec_base<type, num>& line_origin, const vec_base<type, num>& line_tangent, const vec_base<type, num>& plane_normal, const type& plane_offset) {
        return (plane_offset-dot(plane_normal, line_origin))/dot(plane_normal, line_tangent);
    }
    
    template<class type, int num> lineoid_base<lineoid_ray_tag, type, num> intersection_base(const lineoid_base<lineoid_line_tag, type, num>& a, const half_space_base<type, num>& b) {
        bool flip_tangent=dot(a.tangent, b.normal)>0;
        auto origin=a.origin+find_t(a.origin, a.tangent, b.normal, b.offset)*a.tangent;
        return lineoid_base<lineoid_ray_tag, type, num>(origin, flip_tangent? -a.tangent : a.tangent);
    }
    
    template<class type, int num> lineoid_base<lineoid_ray_tag, type, num> intersection_base(const half_space_base<type, num>& a, const lineoid_base<lineoid_line_tag, type, num>& b) {
        return intersection_base(b, a);
    }
    
    template<class type, int num> optional_base<segment_or_ray_base<type, num>> intersection_base(const lineoid_base<lineoid_ray_tag, type, num>& a, const half_space_base<type, num>& b) {
        typedef optional_base<segment_or_ray_base<type, num>> r;
        bool points_to_empty=dot(a.tangent, b.normal)>0;
        bool in_region=dot(a.origin, b.normal)<b.offset;
        auto t=find_t(a.origin, a.tangent, b.normal, b.offset);
        if (in_region && points_to_empty) {
            return r(segment_or_ray_base<type, num>(0, a.origin, t*a.tangent));
        } else
        if (in_region && !points_to_empty) {
            return r(segment_or_ray_base<type, num>(1, a.origin, a.tangent));
        } else
        if (!in_region && points_to_empty) {
            return r();
        } else {
            return r(segment_or_ray_base<type, num>(1, t*a.tangent+a.origin, a.tangent));
        }
    }
    
    template<class type, int num> optional_base<segment_or_ray_base<type, num>> intersection_base(const half_space_base<type, num>& a, const lineoid_base<lineoid_ray_tag, type, num>& b) {
        return intersection_base(b, a);
    }
    
    template<class type, int num> optional_base<lineoid_base<lineoid_segment_tag, type, num>> intersection_base(const lineoid_base<lineoid_segment_tag, type, num>& a, const half_space_base<type, num>& b) {
        typedef optional_base<lineoid_base<lineoid_segment_tag, type, num>> r;
        bool starts_in_region=dot(a.origin, b.normal)<b.offset;
        bool ends_in_region=dot(a.origin+a.tangent, b.normal)<b.offset;
        auto t=find_t(a.origin, a.tangent, b.normal, b.offset);
        if (starts_in_region && ends_in_region) {
            return r(lineoid_base<lineoid_segment_tag, type, num>(a.origin, a.tangent));
        } else
        if (starts_in_region && !ends_in_region) {
            return r(lineoid_base<lineoid_segment_tag, type, num>(a.origin, t*a.tangent));
        } else
        if (!starts_in_region && ends_in_region) {
            return r(lineoid_base<lineoid_segment_tag, type, num>(t*a.tangent+a.origin, (1-t)*a.tangent));
        } else {
            return r();
        }
    }
    
    template<class type, int num> optional_base<lineoid_base<lineoid_segment_tag, type, num>> intersection_base(const half_space_base<type, num>& a, const lineoid_base<lineoid_segment_tag, type, num>& b) {
        return intersection_base(b, a);
    }
    
    template<class type, int num> optional_base<segment_or_ray_base<type, num>> intersection_base(const segment_or_ray_base<type, num>& a, const half_space_base<type, num>& b) {
        typedef optional_base<segment_or_ray_base<type, num>> r;
        if (a.is_ray) {
            return intersect_base(lineoid_base<lineoid_ray_tag, type, num>(a.origin, a.tangent), b);
        } else {
            auto res=intersect_base(lineoid_base<lineoid_segment_tag, type, num>(a.origin, a.tangent), b);
            if (res.exists) {
                return r(segment_or_ray_base<type, num>(res.data.origin, res.data.tangent));
            } else {
                return r();
            }
        }
    }
    
    template<class type, int num> optional_base<segment_or_ray_base<type, num>> intersection_base(const half_space_base<type, num>& a, const segment_or_ray_base<type, num>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> vec_base<type, 2> find_ts(const vec_base<type, 2>& origin_a, const vec_base<type, 2>& tangent_a, const vec_base<type, 2>& origin_b, const vec_base<type, 2>& tangent_b) {
        return inverse(build_matrix_down_vec(-tangent_a, tangent_b))*(origin_a-origin_b);
    }
    
    template<int num, class base_type, class type> optional_base<point_base<base_type, num>> generic_lineoid_intersect(bool is_null, const base_type& t, const type& specified_line) {
        typedef optional_base<point_base<base_type, num>> r;
        if (is_null) {
            return r();
        } else {
            return r(point_base<base_type, num>(t*specified_line.tangent+specified_line.origin));
        }
    }
    
    template<class type> point_base<type, 2> intersection_base(const lineoid_base<lineoid_line_tag, type, 2>& a, const lineoid_base<lineoid_line_tag, type, 2>& b) {
        return point_base<type, 2>(find_ts(a.origin, a.tangent, b.origin, b.tangent)[0]*a.tangent+a.origin);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_line_tag, type, 2>& a, const lineoid_base<lineoid_ray_tag, type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[1]<0, ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_line_tag, type, 2>& a, const lineoid_base<lineoid_segment_tag, type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[1]<0 || ts[1]>1, ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_line_tag, type, 2>& a, const segment_or_ray_base<type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[1]<0 || (!b.is_ray && ts[1]>1), ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_ray_tag, type, 2>& a, const lineoid_base<lineoid_line_tag, type, 2>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_ray_tag, type, 2>& a, const lineoid_base<lineoid_ray_tag, type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[0]<0 || ts[1]<0, ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_ray_tag, type, 2>& a, const lineoid_base<lineoid_segment_tag, type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[0]<0 || ts[1]<0 || ts[1]>1, ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_ray_tag, type, 2>& a, const segment_or_ray_base<type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[0]<0 || ts[1]<0 || (!b.is_ray && ts[1]>1), ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_segment_tag, type, 2>& a, const lineoid_base<lineoid_line_tag, type, 2>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_segment_tag, type, 2>& a, const lineoid_base<lineoid_ray_tag, type, 2>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_segment_tag, type, 2>& a, const lineoid_base<lineoid_segment_tag, type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[0]<0 || ts[0]>1 || ts[1]<0 || ts[1]>1, ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const lineoid_base<lineoid_segment_tag, type, 2>& a, const segment_or_ray_base<type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[0]<0 || ts[0]>1 || ts[1]<0 || (!b.is_ray && ts[1]>1), ts[1], b);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const segment_or_ray_base<type, 2>& a, const lineoid_base<lineoid_line_tag, type, 2>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const segment_or_ray_base<type, 2>& a, const lineoid_base<lineoid_ray_tag, type, 2>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const segment_or_ray_base<type, 2>& a, const lineoid_base<lineoid_segment_tag, type, 2>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 2>> intersection_base(const segment_or_ray_base<type, 2>& a, const segment_or_ray_base<type, 2>& b) {
        auto ts=find_ts(a.origin, a.tangent, b.origin, b.tangent);
        return generic_lineoid_intersect<2>(ts[0]<0 || (!a.is_ray && ts[0]>1) || ts[1]<0 || (!b.is_ray && ts[1]>1), ts[1], b);
    }
    
    template<class type> lineoid_base<lineoid_line_tag, type, 3> intersection_base(const plane_base<type>& a, const plane_base<type>& b) {
        lineoid_base<lineoid_line_tag, type, 3> res;
        res.tangent=cross(a.normal, b.normal);
        res.origin=inverse(build_matrix_across_vec(a.normal, b.normal, res.tangent))*vec_base<type, 3>(a.offset, b.offset, 0);
        return res;
    }
    
    template<class type> point_base<type, 3> intersection_base(const plane_base<type>& a, const lineoid_base<lineoid_line_tag, type, 3>& b) {
        return point_base<type, 3>(find_t(b.origin, b.tangent, a.normal, a.offset)*b.tangent+b.origin);
    }
    
    template<class type> point_base<type, 3> intersection_base(const lineoid_base<lineoid_line_tag, type, 3>& a, const plane_base<type>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 3>> intersection_base(const plane_base<type>& a, const lineoid_base<lineoid_ray_tag, type, 3>& b) {
        type t=find_t(b.origin, b.tangent, a.normal, a.offset);
        return generic_lineoid_intersect(t<0, t, b);
    }
    
    template<class type> optional_base<point_base<type, 3>> intersection_base(const lineoid_base<lineoid_ray_tag, type, 3>& a, const plane_base<type>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 3>> intersection_base(const plane_base<type>& a, const lineoid_base<lineoid_segment_tag, type, 3>& b) {
        type t=find_t(b.origin, b.tangent, a.normal, a.offset);
        return generic_lineoid_intersect(t<0 || t>1, t, b);
    }
    
    template<class type> optional_base<point_base<type, 3>> intersection_base(const lineoid_base<lineoid_segment_tag, type, 3>& a, const plane_base<type>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> optional_base<point_base<type, 3>> intersection_base(const plane_base<type>& a, const segment_or_ray_base<type, 3>& b) {
        type t=find_t(b.origin, b.tangent, a.normal, a.offset);
        return generic_lineoid_intersect(t<0 || (!b.is_ray && t>1), t, b);
    }
    
    template<class type> optional_base<point_base<type, 3>> intersection_base(const segment_or_ray_base<type, 3>& a, const plane_base<type>& b) {
        return intersection_base(b, a);
    }
    
    template<class type> struct add_optional_base {
        typedef optional_base<type> res;
    };
    
    template<class type> struct add_optional_base<optional_base<type>> {
        typedef optional_base<type> res;
    };
    
    template<class type_a, class type_b> auto intersection_base(const optional_base<type_a>& a, const type_b& b) -> typename add_optional_base<decltype(intersection_base(a.data, b))>::res {
        typedef typename add_optional_base<decltype(intersection_base(a.data, b))>::res r;
        if (a.exists) {
            return r(intersection_base(a.data, b));
        } else {
            return r();
        }
    }
    
    template<class type_a, class type_b> auto intersection_base(const type_a& a, const optional_base<type_b>& b) -> decltype(intersection_base(b, a)) {
        return intersection_base(b, a);
    }
    
    template<class type_a, class type_b> auto intersection_base(const optional_base<type_a>& a, const optional_base<type_b>& b) -> typename add_optional_base<decltype(intersection_base(a.data, b.data))>::res {
        typedef typename add_optional_base<decltype(intersection_base(a.data, b.data))>::res r;
        if (a.exists) {
            return intersection_base(a.data, b);
        } else {
            return r();
        }
    }
};

template<class type> auto intersection(const type& a) -> decltype(not_for_public_consumption::intersection_base(a)) {
    return not_for_public_consumption::intersection_base(a);
}

template<class type_a, class type_b, class... types> auto intersection(const type_a& a, const type_b& b, const types&... targs)
    -> decltype(intersection(not_for_public_consumption::intersection_base(a, b), targs...))
{
    return intersection(not_for_public_consumption::intersection_base(a, b), targs...);
}

//

template<class type, int num> half_space_base<type, num> compliment(const half_space_base<type, num>& a) {
    half_space_base<type, num> res=a;
    res.normal=-res.normal;
    res.offset=-res.offset;
    return res;
}

template<class type> auto compliment(const optional_base<type>& a) -> optional_base<decltype(compliment(a.data))> {
    typedef optional_base<decltype(compliment(a.data))> r;
    if (a.exists) {
        return r(compliment(a.data));
    } else {
        return r();
    }
}

//

template<class type> lineoid_base<lineoid_line_tag, type, 2> boundary(const half_space_base<type, 2>& a) {
    vec_base<type, 2> tangent(a.normal.y, -a.normal.x);
    vec_base<type, 2> origin=a.normal*(a.offset/square_norm(a.normal));
    return lineoid_base<lineoid_line_tag, type, 2>(origin, tangent);
}

template<class type> plane_base<type> boundary(const half_space_base<type, 3>& a) {
    return plane_base<type>(a.normal, a.offset);
}

template<class type> const type& boundary(const type& a) { { typename type::primitive_tag assertion; }
    return a;
}

template<class type> auto boundary(const optional_base<type>& a) -> optional_base<decltype(boundary(a.data))> {
    typedef optional_base<decltype(boundary(a.data))> r;
    if (a.exists) {
        return r(boundary(a.data));
    } else {
        return r();
    }
}

//

template<class type> bool exists(const type& targ) { { typename type::primitive_tag assertion; }
    return 1;
}

template<class type> bool exists(const optional_base<type>& targ) {
    return targ.exists;
}

//

template<class type> const type& assume_exists(const type& targ) { { typename type::primitive_tag assertion; }
    return targ;
}

template<class type> const type& assume_exists(const optional_base<type>& targ) {
    return targ.data;
}

//

template<class type> bool is_segment(const type& targ) { { typename type::primitive_tag assertion; }
    return 0;
}

template<class type, int num> bool is_segment(const lineoid_base<lineoid_segment_tag, type, num>& targ) {
    return 1;
}

template<class type, int num> bool is_segment(const segment_or_ray_base<type, num>& targ) {
    return !targ.is_ray;
}

template<class type> bool is_segment(const optional_base<type>& targ) {
    return targ.exists && is_segment(targ.data);
}

//

template<class type, int num> const lineoid_base<lineoid_segment_tag, type, num>& assume_segment(const lineoid_base<lineoid_segment_tag, type, num>& targ) {
    return targ;
}

template<class type, int num> lineoid_base<lineoid_segment_tag, type, num> assume_segment(const segment_or_ray_base<type, num>& targ) {
    return lineoid_base<lineoid_segment_tag, type, num>(targ.origin, targ.offset);
}

template<class type> auto assume_segment(const optional_base<type>& targ) -> decltype(assume_segment(targ.data)) {
    return assume_segment(targ.data);
}

//

template<class type> bool is_ray(const type& targ) { { typename type::primitive_tag assertion; }
    return 0;
}

template<class type, int num> bool is_ray(const lineoid_base<lineoid_ray_tag, type, num>& targ) {
    return 1;
}

template<class type, int num> bool is_ray(const segment_or_ray_base<type, num>& targ) {
    return targ.is_ray;
}

template<class type> bool is_ray(const optional_base<type>& targ) {
    return targ.exists && is_ray(targ.data);
}

//

template<class type, int num> const lineoid_base<lineoid_ray_tag, type, num>& assume_ray(const lineoid_base<lineoid_ray_tag, type, num>& targ) {
    return targ;
}

template<class type, int num> lineoid_base<lineoid_ray_tag, type, num> assume_ray(const segment_or_ray_base<type, num>& targ) {
    return lineoid_base<lineoid_ray_tag, type, num>(targ.origin, targ.offset);
}

template<class type> auto assume_ray(const optional_base<type>& targ) -> decltype(assume_ray(targ.data)) {
    return assume_ray(targ.data);
}

//

template<class type, int num> point_base<type, num> transform(const point_base<type, num>& targ, const matrix_across_base<type, num, num>& t) {
    return point_base<type, num>(t*targ.pos);
}

template<class type, int num> point_base<type, num> transform(const point_base<type, num>& targ, const matrix_across_base<type, num+1, num+1>& t) {
    return point_base<type, num>(linearize(t*homogenize(targ.pos)));
}

template<class line_type, class type, int num> lineoid_base<line_type, type, num> transform(const lineoid_base<line_type, type, num>& targ, const matrix_across_base<type, num, num>& t) {
    return lineoid_base<line_type, type, num>(t*targ.origin, t*targ.tangent);
}

template<class line_type, class type, int num> lineoid_base<line_type, type, num> transform(const lineoid_base<line_type, type, num>& targ, const matrix_across_base<type, num+1, num+1>& t) {
    auto o=linearize(t*homogenize(targ.origin));
    auto d=linearize(t*homogenize(targ.origin+targ.tangent))-o;
    return lineoid_base<line_type, type, num>(o, d);
}

template<class type, int num, int num2> segment_or_ray_base<type, num> transform(const segment_or_ray_base<type, num>& targ, const matrix_across_base<type, num2, num2>& t) {
    auto res=transform(lineoid_base<lineoid_line_tag, type, num>(targ.origin, targ.tangent), t);
    return segment_or_ray_base<type, num>(targ.is_ray, res.origin, res.tangent);
}

template<class type, int num> half_space_base<type, num> transform(const half_space_base<type, num>& targ, const matrix_across_base<type, num, num>& t) {
    auto p=targ.normal*(targ.offset/square_length(targ.normal));
    auto n=t*targ.normal;
    half_space_base<type, num> res;
    res.normal=n;
    res.offset=dot(n, t*p);
    return res;
}

template<class type> half_space_base<type, 2> transform(const half_space_base<type, 2>& targ, const matrix_across_base<type, 3, 3>& t) {
    auto p=targ.normal*(targ.offset/square_length(targ.normal));
    vec_base<type, 2> tangent(-targ.normal.y, targ.normal.x);
    point_base<type, 2> boundary_a(linearize(t*homogenize(p)));
    point_base<type, 2> boundary_b(linearize(t*homogenize(p+tangent)));
    point_base<type, 2> inner(linearize(t*homogenize(p-targ.normal)));
    return half_space_base<type, 2>(affine_hull(boundary_a, boundary_b), inner);
}

template<class type> half_space_base<type, 3> transform(const half_space_base<type, 3>& targ, const matrix_across_base<type, 4, 4>& t) {
    int max_coord=0;
    type val_max_coord=targ.normal[0];
    for (int x=1;x<3;++x) {
        if (abs(targ.normal[x])>val_max_coord) {
            max_coord=x;
            val_max_coord=abs(targ.normal[x]);
        }
    }
    vec_base<type, 3> p_vec;
    p_vec[max_coord]=1;
    p_vec=p_vec-dot(p_vec, targ.normal)*targ.normal;
    auto p=targ.normal*(targ.offset/square_length(targ.normal));
    point_base<type, 3> boundary_a(linearize(t*homogenize(p)));
    point_base<type, 3> boundary_b(linearize(t*homogenize(p+p_vec)));
    point_base<type, 3> boundary_c(linearize(t*homogenize(p+cross(p_vec, targ.normal))));
    point_base<type, 3> inner(linearize(t*homogenize(p-targ.normal)));
    auto plane=affine_hull(boundary_a, boundary_b, boundary_c);
    return half_space_base<type, 3>(plane, inner);
}

template<class type, int num2> plane_base<type> transform(const plane_base<type>& targ, const matrix_across_base<type, num2, num2>& t) {
    half_space_base<type, 3> res;
    res.normal=targ.normal;
    res.tangent=targ.tangent;
    res=transform(res, t);
    return plane_base<type>(res.normal, res.offset);
}

template<class type, class type_2, int num> auto transform(const optional_base<type>& targ, const matrix_across_base<type_2, num, num>& t) -> optional_base<decltype(transform(targ.data, t))> {
    if (targ.exists) {
        return optional_base<type>(transform(targ.data, t));
    } else {
        return optional_base<type>();
    }
}

//

template<class type, int num> point_base<type, num> translate(const point_base<type, num>& targ, const vec_base<type, num>& t) {
    return point_base<type, num>(targ.pos+t);
}

template<class line_type, class type, int num> lineoid_base<line_type, type, num> translate(const lineoid_base<line_type, type, num>& targ, const vec_base<type, num>& t) {
    return lineoid_base<line_type, type, num>(targ.origin+t, targ.tangent);
}

template<class type, int num, int num2> segment_or_ray_base<type, num> translate(const segment_or_ray_base<type, num>& targ, const vec_base<type, num>& t) {
    return segment_or_ray_base<type, num>(targ.is_ray, targ.origin+t, targ.tangent);
}

template<class type, int num> half_space_base<type, num> translate(const half_space_base<type, num>& targ, const vec_base<type, num>& t) {
    half_space_base<type, num> res;
    res.normal=targ.normal;
    res.offset=targ.offset+dot(targ.normal, t);
    return res;
}

template<class type, int num2> plane_base<type> translate(const plane_base<type>& targ, const vec_base<type, num2>& t) {
    half_space_base<type, 3> res;
    res.normal=targ.normal;
    res.tangent=targ.tangent;
    res=translate(res, t);
    return plane_base<type>(res.normal, res.offset);
}

template<class type, class type_2, int num> auto translate(const optional_base<type>& targ, const vec_base<type, num>& t) -> optional_base<decltype(translate(targ.data, t))> {
    if (targ.exists) {
        return optional_base<type>(translate(targ.data, t));
    } else {
        return optional_base<type>();
    }
}

}

#endif
