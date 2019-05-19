#ifndef ILYA_HEADER_MATH_LINEAR_TRANSFORM
#define ILYA_HEADER_MATH_LINEAR_TRANSFORM

#include <iostream>
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <vector>

#include "generic.h"

#include "math_base.h"
#include "math_linear_vector.h"

namespace math {
using std::ostream;
using std::sqrt;
using std::min;
using std::string;
using std::setw;
using std::vector;
using generic::print_as_number;
using generic::assert_true;
using generic::only_if_same_types;
using generic::only_if;

namespace not_public {
    template<class type, int width, int height> class matrix_names_down_base {
        typedef vec_base<type, height> vd;
        typedef vec_base<type, width> va;
        
        public:
        static vd read_linear(const type* data, int pos) {
            vd res;
            for (int x=0,y=height*pos;x<height;++x,++y) res[x]=data[y];
            return res;
        }
        static va read_stride(const type* data, int pos) {
            va res;
            for (int x=0,y=pos;x<width;++x,y+=height) res[x]=data[y];
            return res;
        }
        //
        static void write_linear(type* data, const vd& targ, int pos) {
            for (int x=0,y=height*pos;x<height;++x,++y) data[y]=targ[x];
        }
        static void write_stride(type* data, const va& targ, int pos) {
            for (int x=0,y=pos;x<width;++x,y+=height) data[y]=targ[x];
        }
        
        type data[width*height];
        //
        va across_x() const { return read_stride(data, 0); }
        void across_x(const va& targ) { write_stride(data, targ, 0); }
        //
        va across_y() const { return read_stride(data, 1); }
        void across_y(const va& targ) { write_stride(data, targ, 1); }
        //
        va across_z() const { return read_stride(data, 2); }
        void across_z(const va& targ) { write_stride(data, targ, 2); }
        //
        va across_w() const { return read_stride(data, 3); }
        void across_w(const va& targ) { write_stride(data, targ, 3); }
        va across_t() const { return across_w(); }
        void across_t(const va& targ) { across_w(targ); }
        //
        va across(int n) const { return read_stride(data, n); }
        void across(int n, const va& targ) { return write_stride(data, targ, n); }
        ////
        vd down_x() const { return read_linear(data, 0); }
        void down_x(const vd& targ) { write_linear(data, targ, 0); }
        //
        vd down_y() const { return read_linear(data, 1); }
        void down_y(const vd& targ) { write_linear(data, targ, 1); }
        //
        vd down_z() const { return read_linear(data, 2); }
        void down_z(const vd& targ) { write_linear(data, targ, 2); }
        //
        vd down_w() const { return read_linear(data, 3); }
        void down_w(const vd& targ) { write_linear(data, targ, 3); }
        vd down_t() const { return down_w(); }
        void down_t(const vd& targ) { down_w(targ); }
        //
        vd down(int n) const { return read_linear(data, n); }
        void down(int n, const vd& targ) { return write_linear(data, targ, n); }
        ////
        va x() const { return across_x(); }
        void x(const va& targ) { across_x(targ); }
        //
        va y() const { return across_y(); }
        void y(const va& targ) { across_y(targ); }
        //
        va z() const { return across_z(); }
        void z(const va& targ) { across_z(targ); }
        //
        va w() const { return across_w(); }
        void w(const va& targ) { across_w(targ); }
        va t() const { return across_t(); }
        void t(const va& targ) { across_t(targ); }
    };
    
    template<class type, int width, int height> class matrix_names_across_base : matrix_names_down_base<type, height, width> {
        typedef matrix_names_down_base<type, height, width> base;
        typedef vec_base<type, height> vd;
        typedef vec_base<type, width> va;
        
        public:
        using base::data;
        //
        va across_x() const { return base::down_x(); }
        void across_x(const va& targ) { base::down_x(targ); }
        //
        va across_y() const { return base::down_y(); }
        void across_y(const va& targ) { base::down_y(targ); }
        //
        va across_z() const { return base::down_z(); }
        void across_z(const va& targ) { base::down_z(targ); }
        //
        va across_w() const { return base::down_w(); }
        void across_w(const va& targ) { base::down_w(targ); }
        va across_t() const { return across_w(); }
        void across_t(const va& targ) { across_w(targ); }
        //
        va across(int n) const { return base::down(n); }
        void across(int n, const va& targ) { return base::down(n, targ); }
        ////
        vd down_x() const { return base::across_x(); }
        void down_x(const vd& targ) { base::across_x(targ); }
        //
        vd down_y() const { return base::across_y(); }
        void down_y(const vd& targ) { base::across_y(targ); }
        //
        vd down_z() const { return base::across_z(); }
        void down_z(const vd& targ) { base::across_z(targ); }
        //
        vd down_w() const { return base::across_w(); }
        void down_w(const vd& targ) { base::across_w(targ); }
        vd down_t() const { return down_w(); }
        void down_t(const vd& targ) { down_w(targ); }
        //
        vd down(int n) const { return base::across(n); }
        void down(int n, const vd& targ) { return base::across(n, targ); }
        ////
        va x() const { return across_x(); }
        void x(const va& targ) { across_x(targ); }
        //
        va y() const { return across_y(); }
        void y(const va& targ) { across_y(targ); }
        //
        va z() const { return across_z(); }
        void z(const va& targ) { across_z(targ); }
        //
        va w() const { return across_w(); }
        void w(const va& targ) { across_w(targ); }
        va t() const { return across_t(); }
        void t(const va& targ) { across_t(targ); }
    };
    
    template<template<class, int, int> class majorness, class type, int width, int height> class matrix_names;
    
    template<template<class, int, int> class majorness, class type, int width, int height> class matrix_names_downs : majorness<type, width, height> {
        friend class matrix_names<majorness, type, width, height>;
        typedef majorness<type, width, height> base;
        //
        public:
        using base::data;
        using base::down;
        using base::across;
        using base::down_x;
        using base::down_y;
        using base::down_z;
        using base::down_w;
        using base::down_t;
    };
    template<template<class, int, int> class majorness, class type, int height> class matrix_names_downs<majorness, type, 1, height> : majorness<type, 1, height> {
        friend class matrix_names<majorness, type, 1, height>;
        typedef majorness<type, 1, height> base;
        //
        public:
        using base::data;
        using base::down;
        using base::across;
        using base::down_x;
        //
        operator vec_base<type, height>() { return base::down_x(); }
        matrix_names_downs() {}
        template<int t_n> matrix_names_downs(const vec_base<type, t_n>& targ) { base::down_x(targ); }
        template<int t_n> matrix_names_downs& operator=(const vec_base<type, t_n>& targ) { base::down_x(targ); return *this; }
    };
    template<template<class, int, int> class majorness, class type, int height> class matrix_names_downs<majorness, type, 2, height> : majorness<type, 2, height> {
        friend class matrix_names<majorness, type, 2, height>;
        typedef majorness<type, 2, height> base;
        //
        public:
        using base::data;
        using base::down;
        using base::across;
        using base::down_x;
        using base::down_y;
    };
    template<template<class, int, int> class majorness, class type, int height> class matrix_names_downs<majorness, type, 3, height> : majorness<type, 3, height> {
        friend class matrix_names<majorness, type, 3, height>;
        typedef majorness<type, 3, height> base;
        //
        public:
        using base::data;
        using base::down;
        using base::across;
        using base::down_x;
        using base::down_y;
        using base::down_z;
    };
    
    template<template<class, int, int> class majorness, class type, int width, int height> class matrix_names : public matrix_names_downs<majorness, type, width, height> {
        typedef matrix_names_downs<majorness, type, width, height> base;
        //
        public:
        using base::across_x;
        using base::x;
        using base::across_y;
        using base::y;
        using base::across_z;
        using base::z;
        using base::across_w;
        using base::w;
        using base::across_t;
        using base::t;
        //
        template<class t_type> matrix_names(const t_type& targ) : base(targ) {}
        template<class t_type> matrix_names& operator=(const t_type& targ) { base::operator=(targ); return *this; }
        matrix_names() {}
    };
    template<template<class, int, int> class majorness, class type> class matrix_names<majorness, type, 1, 1> : public matrix_names_downs<majorness, type, 1, 1> {
        typedef matrix_names_downs<majorness, type, 1, 1> base;
        //
        public:
        using base::across_x;
        using base::x;
        //
        operator vec_base<type, 1>() { return base::across_x(); }
        operator type() { return base::data[0]; }
        matrix_names() {}
        template<int t_n> matrix_names(const vec_base<type, t_n>& targ) { base::across_x(targ); }
        template<int t_n> matrix_names& operator=(const vec_base<type, t_n>& targ) { base::across_x(targ); return *this; } 
        matrix_names(const type& targ) { base::data[0]=targ; }
        matrix_names& operator=(const type& targ) { base::data[0]=targ; return *this; }
    };
    template<template<class, int, int> class majorness, class type, int width> class matrix_names<majorness, type, width, 1> : public matrix_names_downs<majorness, type, width, 1> {
        typedef matrix_names_downs<majorness, type, width, 1> base;
        //
        public:
        using base::across_x;
        using base::x;
        //
        operator vec_base<type, width>() { return base::across_x(); }
        matrix_names() {}
        template<int t_n> matrix_names(const vec_base<type, t_n>& targ) { base::across_x(targ); }
        template<int t_n> matrix_names& operator=(const vec_base<type, t_n>& targ) { base::across_x(targ); return *this; } 
    };
    template<template<class, int, int> class majorness, class type, int width> class matrix_names<majorness, type, width, 2> : public matrix_names_downs<majorness, type, width, 2> {
        typedef matrix_names_downs<majorness, type, width, 2> base;
        //
        public:
        using base::across_x;
        using base::x;
        using base::across_y;
        using base::y;
        //
        template<class t_type> matrix_names(const t_type& targ) : base(targ) {}
        template<class t_type> matrix_names& operator=(const t_type& targ) { base::operator=(targ); return *this; }
        matrix_names() {}
    };
    template<template<class, int, int> class majorness, class type, int width> class matrix_names<majorness, type, width, 3> : public matrix_names_downs<majorness, type, width, 3> {
        typedef matrix_names_downs<majorness, type, width, 3> base;
        //
        public:
        using base::across_x;
        using base::x;
        using base::across_y;
        using base::y;
        using base::across_z;
        using base::z;
        //
        template<class t_type> matrix_names(const t_type& targ) : base(targ) {}
        template<class t_type> matrix_names& operator=(const t_type& targ) { base::operator=(targ); return *this; }
        matrix_names() {}
    };
}

template<class type, int t_width, int t_height> class matrix_across_base;

template<class type, int t_width, int t_height> class matrix_down_base : public not_public::matrix_names<not_public::matrix_names_down_base, type, t_width, t_height> {
    typedef not_public::matrix_names<not_public::matrix_names_down_base, type, t_width, t_height> base;
    
    public:
    static const int num=t_width*t_height;
    static const int width=t_width;
    static const int height=t_height;
    
    matrix_down_base() {
        type *pos=base::data;
        for (int c=0,nc=0;c<num;++c,++pos) {
            if (c==nc) {
                *pos=type(1);
                nc+=height+1;
            } else *pos=type(0);
        }
    }
    template<class t_type> matrix_down_base(const t_type& targ) : base(targ) {}
    template<int height2, int width2> matrix_down_base(const matrix_down_base<type, height2, width2>& targ) { *this=targ; }
    template<int height2, int width2> matrix_down_base(const matrix_across_base<type, height2, width2>& targ);
    //
    template<int height2, int width2> matrix_down_base& operator=(const matrix_down_base<type, height2, width2>& targ) { assert_true<width2 <= width>; assert_true<height2 <= height>;
        type *pos=base::data;
        const type *n_pos=targ.data;
        int x=0;
        for (;x<width2;++x) {
            int y=0;
            for (;y<height2;++y,++pos,++n_pos) *pos=*n_pos;
            for (;y<height;++y,++pos) *pos=(x==y)? type(1) : type(0);
        }
        for (int c=width2*height,nc=c+width2;c<num;++c,++pos) {
            if (c==nc) {
                *pos=type(1);
                nc+=height+1;
            } else *pos=type(0);
        }
        return *this;
    }
    template<int height2, int width2> matrix_down_base& operator=(const matrix_across_base<type, height2, width2>& targ);
    //
    type& operator()(int y, int x) { return base::data[x*height+y]; }
    const type& operator()(int y, int x) const { return base::data[x*height+y]; }
    type& operator[](int pos) { return base::data[pos]; }
    const type& operator[](int pos) const { return base::data[pos]; }
    //
    int size() const { return num; }
    type* begin() { return base::data; }
    const type* begin() const { return base::data; }
    type* end() { return base::data+num; }
    const type* end() const { return base::data+num; }
};

template<class type, int t_width, int t_height> class matrix_across_base : public not_public::matrix_names<not_public::matrix_names_across_base, type, t_width, t_height> {
    typedef not_public::matrix_names<not_public::matrix_names_across_base, type, t_width, t_height> base;
    
    public:
    static const int num=t_width*t_height;
    static const int width=t_width;
    static const int height=t_height;
    
    matrix_across_base() {
        type *pos=base::data;
        for (int c=0,nc=0;c<num;++c,++pos) {
            if (c==nc) {
                *pos=type(1);
                nc+=width+1;
            } else *pos=type(0);
        }
    }
    template<class t_type> matrix_across_base(const t_type& targ) : base(targ) {}
    template<int width2, int height2> matrix_across_base(const matrix_across_base<type, width2, height2>& targ) { *this=targ; }
    template<int width2, int height2> matrix_across_base(const matrix_down_base<type, width2, height2>& targ) { *this=targ; }
    //
    template<int width2, int height2> matrix_across_base& operator=(const matrix_across_base<type, width2, height2>& targ) { assert_true<width2 <= width>; assert_true<height2 <= height>;
        type *pos=base::data;
        const type *n_pos=targ.data;
        int y=0;
        for (;y<height2;++y) {
            int x=0;
            for (;x<width2;++x,++pos,++n_pos) *pos=*n_pos;
            for (;x<width;++x,++pos) *pos=(x==y)? type(1) : type(0);
        }
        for (int c=height2*width,nc=c+height2;c<num;++c,++pos) {
            if (c==nc) {
                *pos=type(1);
                nc+=width+1;
            } else *pos=type(0);
        }
        return *this;
    }
    template<int width2, int height2> matrix_across_base& operator=(const matrix_down_base<type, width2, height2>& targ) { assert_true<width2 <= width>; assert_true<height2 <= height>;
        type *pos=base::data;
        const type *n_offset=targ.data;
        int y=0;
        for (;y<height2;++y) {
            int x=0;
            const type *n_pos=n_offset;
            for (;x<width2-1;++x,++pos,n_pos+=height2) *pos=*n_pos;
            *pos=*n_pos;
            ++pos;
            ++x;
            ++n_offset;
            for (;x<width;++x,++pos) *pos=(x==y)? type(1) : type(0);
            
        }
        for (int c=height2*width,nc=c+height2;c<num;++c,++pos) {
            if (c==nc) {
                *pos=type(1);
                nc+=width+1;
            } else *pos=type(0);
        }
        return *this;
    }
    //
    type& operator()(int y, int x) { return base::data[x+y*width]; }
    const type& operator()(int y, int x) const { return base::data[x+y*width]; }
    type& operator[](int pos) { return base::data[pos]; }
    const type& operator[](int pos) const { return base::data[pos]; }
    //
    int size() const { return num; }
    type* begin() { return base::data; }
    const type* begin() const { return base::data; }
    type* end() { return base::data+num; }
    const type* end() const { return base::data+num; }
};

template<class type, int t_width, int t_height> template<int width2, int height2> matrix_down_base<type, t_width, t_height>::matrix_down_base(const matrix_across_base<type, width2, height2>& targ) { *this=targ; }
template<class type, int t_width, int t_height> template<int width2, int height2> matrix_down_base<type, t_width, t_height>& matrix_down_base<type, t_width, t_height>::operator=(const matrix_across_base<type, width2, height2>& targ) { assert_true<width2 <= width>; assert_true<height2 <= height>;
    type *pos=base::data;
    const type *n_offset=targ.data;
    int x=0;
    for (;x<width2;++x) {
        int y=0;
        const type *n_pos=n_offset;
        for (;y<height2-1;++y,++pos,n_pos+=width2) *pos=*n_pos;
        *pos=*n_pos;
        ++pos;
        ++y;
        ++n_offset;
        for (;y<height;++y,++pos) *pos=(x==y)? type(1) : type(0);
    }
    for (int c=width2*height,nc=c+width2;c<num;++c,++pos) {
        if (c==nc) {
            *pos=type(1);
            nc+=height+1;
        } else *pos=type(0);
    }
    return *this;
}

template<class type> struct matrix_traits {
    template<class tt_type> struct only_if {};
};
template<class t_type, int t_width, int t_height> struct matrix_traits<matrix_down_base<t_type, t_width, t_height>> {
    static const int width=t_width;
    static const int height=t_height;
    static const int num=t_width*t_height;
    typedef t_type type;
    template<int tt_width, int tt_height> struct make_matrix {
        typedef matrix_down_base<t_type, tt_width, tt_height> res;
    };
    template<class tt_type, int tt_width, int tt_height> struct make_typed_matrix {
        typedef matrix_down_base<tt_type, tt_width, tt_height> res;
    };
    template<class tt_type> struct only_if {
        typedef tt_type good;
    };
};
template<class t_type, int t_width, int t_height> struct matrix_traits<matrix_across_base<t_type, t_width, t_height>> {
    static const int width=t_width;
    static const int height=t_height;
    static const int num=t_width*t_height;
    typedef t_type type;
    template<int tt_width, int tt_height> struct make_matrix {
        typedef matrix_across_base<t_type, tt_width, tt_height> res;
    };
    template<class tt_type, int tt_width, int tt_height> struct make_typed_matrix {
        typedef matrix_across_base<tt_type, tt_width, tt_height> res;
    };
    template<class tt_type> struct only_if {
        typedef tt_type good;
    };
};
//
template<class type, class type2> struct matrix_bi_traits {
    template<class tt_type> struct only_if {};
};
template<class t_type, int t_width, int t_height, int t_width2, int t_height2> struct matrix_bi_traits<matrix_down_base<t_type, t_width, t_height>, matrix_down_base<t_type, t_width2, t_height2>> {
    static const int width=t_width;
    static const int height=t_height;
    static const int num=t_width*t_height;
    static const int width2=t_width2;
    static const int height2=t_height2;
    static const int num2=t_width2*t_height2;
    typedef t_type type;
    template<int tt_width, int tt_height> struct make_matrix_first {
        typedef matrix_down_base<t_type, tt_width, tt_height> res;
    };
    template<int tt_width, int tt_height> struct make_matrix_second {
        typedef matrix_down_base<t_type, tt_width, tt_height> res;
    };
    template<class tt_type> struct only_if {
        typedef tt_type good;
    };
};
template<class t_type, int t_width, int t_height, int t_width2, int t_height2> struct matrix_bi_traits<matrix_down_base<t_type, t_width, t_height>, matrix_across_base<t_type, t_width2, t_height2>> {
    static const int width=t_width;
    static const int height=t_height;
    static const int num=t_width*t_height;
    static const int width2=t_width2;
    static const int height2=t_height2;
    static const int num2=t_width2*t_height2;
    typedef t_type type;
    template<int tt_width, int tt_height> struct make_matrix_first {
        typedef matrix_down_base<t_type, tt_width, tt_height> res;
    };
    template<int tt_width, int tt_height> struct make_matrix_second {
        typedef matrix_across_base<t_type, tt_width, tt_height> res;
    };
    template<class tt_type> struct only_if {
        typedef tt_type good;
    };
};
template<class t_type, int t_width, int t_height, int t_width2, int t_height2> struct matrix_bi_traits<matrix_across_base<t_type, t_width, t_height>, matrix_down_base<t_type, t_width2, t_height2>> {
    static const int width=t_width;
    static const int height=t_height;
    static const int num=t_width*t_height;
    static const int width2=t_width2;
    static const int height2=t_height2;
    static const int num2=t_width2*t_height2;
    typedef t_type type;
    template<int tt_width, int tt_height> struct make_matrix_first {
        typedef matrix_across_base<t_type, tt_width, tt_height> res;
    };
    template<int tt_width, int tt_height> struct make_matrix_second {
        typedef matrix_down_base<t_type, tt_width, tt_height> res;
    };
    template<class tt_type> struct only_if {
        typedef tt_type good;
    };
};
template<class t_type, int t_width, int t_height, int t_width2, int t_height2> struct matrix_bi_traits<matrix_across_base<t_type, t_width, t_height>, matrix_across_base<t_type, t_width2, t_height2>> {
    static const int width=t_width;
    static const int height=t_height;
    static const int num=t_width*t_height;
    static const int width2=t_width2;
    static const int height2=t_height2;
    static const int num2=t_width2*t_height2;
    typedef t_type type;
    template<int tt_width, int tt_height> struct make_matrix_first {
        typedef matrix_across_base<t_type, tt_width, tt_height> res;
    };
    template<int tt_width, int tt_height> struct make_matrix_second {
        typedef matrix_across_base<t_type, tt_width, tt_height> res;
    };
    template<class tt_type> struct only_if {
        typedef tt_type good;
    };
};
//
template<class type, class r_type> struct only_if_matrix {};
template<class t_type, int t_width, int t_height, class r_type> struct only_if_matrix<matrix_down_base<t_type, t_width, t_height>, r_type> { typedef r_type good; };
template<class t_type, int t_width, int t_height, class r_type> struct only_if_matrix<matrix_across_base<t_type, t_width, t_height>, r_type> { typedef r_type good; };
//
template<class type, class type2, class r_type> struct only_if_both_matrix {};
template<class t_type, int t_width, int t_height, int t_width2, int t_height2, class r_type> struct only_if_both_matrix<matrix_down_base<t_type, t_width, t_height>, matrix_down_base<t_type, t_width2, t_height2>, r_type> { typedef r_type good; };
template<class t_type, int t_width, int t_height, int t_width2, int t_height2, class r_type> struct only_if_both_matrix<matrix_down_base<t_type, t_width, t_height>, matrix_across_base<t_type, t_width2, t_height2>, r_type> { typedef r_type good; };
template<class t_type, int t_width, int t_height, int t_width2, int t_height2, class r_type> struct only_if_both_matrix<matrix_across_base<t_type, t_width, t_height>, matrix_down_base<t_type, t_width2, t_height2>, r_type> { typedef r_type good; };
template<class t_type, int t_width, int t_height, int t_width2, int t_height2, class r_type> struct only_if_both_matrix<matrix_across_base<t_type, t_width, t_height>, matrix_across_base<t_type, t_width2, t_height2>, r_type> { typedef r_type good; };
template<class type> void assert_is_matrix() { typename only_if_matrix<type, int>::good x; }

//
//

template<class matrix_a, class matrix_b> typename matrix_bi_traits<matrix_a, matrix_b>::template make_matrix_first<matrix_b::width, matrix_a::height>::res operator*(const matrix_a& a, const matrix_b& b) {
    assert_true<matrix_a::width==matrix_b::height>;
    typename matrix_bi_traits<matrix_a, matrix_b>::template make_matrix_first<matrix_b::width, matrix_a::height>::res res;
    for (int y=0;y<matrix_a::height;++y) {
        for (int x=0;x<matrix_b::width;++x) {
            res(y, x)=dot(a.across(y), b.down(x));
        }
    }
    return res;
}

template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator+=(matrix& a, const matrix& b) {
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            a(y, x)+=b(y, x);
        }
    }
    return a;
}
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator-=(matrix& a, const matrix& b) {
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            a(y, x)-=b(y, x);
        }
    }
    return a;
}
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator*=(matrix& a, const matrix& b) { assert_true<matrix::width==matrix::height>;
    if (&a==&b) {
        matrix b_copy=b;
        return a*=b_copy;
    } else {
        for (int y=0;y<matrix::height;++y) {
            auto across=a.across(y);
            for (int x=0;x<matrix::width;++x) {
                a(y, x)=dot(across, b.down(x));
            }
        }
        return a;
    }
}

template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator*=(matrix& a, const typename matrix_traits<matrix>::type& b) {
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            a(y, x)*=b;
        }
    }
    return a;
}
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator/=(matrix& a, const typename matrix_traits<matrix>::type& b) {
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            a(y, x)/=b;
        }
    }
    return a;
}
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator+=(matrix& a, const typename matrix_traits<matrix>::type& b) {
    for (int y=0;y<min(matrix::width, matrix::height);++y) {
        a(y, y)+=b;
    }
    return a;
}
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator-=(matrix& a, const typename matrix_traits<matrix>::type& b) {
    for (int y=0;y<min(matrix::width, matrix::height);++y) {
        a(y, y)-=b;
    }
    return a;
}

template<class matrix_a, class vec_b> vec_base<typename matrix_traits<typename vec_traits<vec_b>::template only_if<matrix_a>::good>::type, matrix_a::height> operator*(const matrix_a& a, const vec_b& b) {
    return vec_base<typename matrix_traits<matrix_a>::type, matrix_a::height>(a*typename matrix_traits<matrix_a>::template make_matrix<1, vec_b::num>::res(b));
}
template<class vec_a, class matrix_b> vec_base<typename matrix_traits<typename vec_traits<vec_a>::template only_if<matrix_b>::good>::type, matrix_b::width> operator*(const vec_a& a, const matrix_b& b) {
    return vec_base<typename matrix_traits<matrix_b>::type, matrix_b::width>(typename matrix_traits<matrix_b>::template make_matrix<vec_a::num, 1>::res(a)*b);
}

template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator+(matrix a, const matrix& b) { return a+=b; }
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator-(matrix a, const matrix& b) { return a-=b; }
//
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator+(matrix a, const typename matrix_traits<matrix>::type& b) { return a+=b; }
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator-(matrix a, const typename matrix_traits<matrix>::type& b) { return a-=b; }
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator*(matrix a, const typename matrix_traits<matrix>::type& b) { return a*=b; }
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator/(matrix a, const typename matrix_traits<matrix>::type& b) { return a/=b; }
//
template<class matrix, class vec> typename only_if_same_types<typename only_if<matrix::height==vec::num, typename matrix_traits<matrix>::type>::good, typename vec_traits<vec>::type, vec>::good exterior_down(const vec& a, const matrix& b) {
    return vec(typename matrix_traits<matrix>::template make_matrix<vec::num, 1>::res(a)*b);
}
template<class matrix, class vec> typename only_if_same_types<typename only_if<matrix::height==vec::num, typename matrix_traits<matrix>::type>::good, typename vec_traits<vec>::type, vec>::good exterior_down(const matrix& a, const vec& b) {
    return vec(a*typename matrix_traits<matrix>::template make_matrix<1, vec::num>(b));
}

template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good operator-(const matrix& targ) {
    matrix res;
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            res(y, x)=-targ(y, x);
        }
    }
    return res;
}

template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good transpose(const matrix& targ) {
    typename matrix_traits<matrix>::template make_matrix<matrix::height, matrix::width>::res res;
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            res(x, y)=targ(y, x);
        }
    }
    return res;
}

template<class matrix> typename matrix_traits<matrix>::type trace(const matrix& targ) {
    auto res=typename matrix_traits<matrix>::type(0);
    for (int y=0;y<min(matrix::width, matrix::height);++y) {
        res+=targ(y, y);
    }
    return res;
}

template<int width, int height, class matrix> typename matrix_traits<matrix>::template make_matrix<width, height>::res submatrix(const matrix& targ, int top, int left) {
    typedef typename matrix_traits<matrix>::type type;
    const int matrix_height=matrix::height; //gcc bug?
    const int matrix_width=matrix::width;
    const int max_read_y=min(top+height, matrix_height);
    const int max_read_x=min(left+width, matrix_width);
    typename matrix_traits<matrix>::template make_matrix<width, height>::res res;
    //
    int read_y=top;
    int write_y=0;
    for (;read_y<max_read_y;++read_y, ++write_y) {
        int read_x=left;
        int write_x=0;
        for (;read_x<max_read_x;++read_x, ++write_x) {
            res(write_y, write_x)=targ(read_y, read_x);
        }
        for (;write_x<width;++read_x, ++write_x) {
            res(write_y, write_x)=(read_x==read_y)? type(1) : type(0);
        }
    }
    for (;write_y<height;++read_y, ++write_y) {
        int read_x=left;
        int write_x=0;
        for (;write_x<width;++read_x, ++write_x) {
            res(write_y, write_x)=(read_x==read_y)? type(1) : type(0);
        }
    }
    return res;
}

template<class matrix, class func_type> auto apply(matrix targ, func_type func) -> typename matrix_traits<matrix>::template make_typed_matrix<decltype(func(typename matrix_traits<matrix>::type())), matrix::width, matrix::height>::res {
    typename matrix_traits<matrix>::template make_typed_matrix<decltype(func(typename matrix_traits<matrix>::type())), matrix::width, matrix::height>::res res;
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            res(y, x)=func(targ(y, x));
        }
    }
    return res;
}
template<class matrix, class func_type> auto apply_locate(matrix targ, func_type func) -> typename matrix_traits<matrix>::template make_typed_matrix<decltype(func(typename matrix_traits<matrix>::type(), 0, 0)), matrix::width, matrix::height>::res {
    typename matrix_traits<matrix>::template make_typed_matrix<decltype(func(typename matrix_traits<matrix>::type(), 0, 0)), matrix::width, matrix::height>::res res;
    for (int y=0;y<matrix::height;++y) {
        for (int x=0;x<matrix::width;++x) {
            res(y, x)=func(targ(y, x), y, x);
        }
    }
    return res;
}

namespace not_public {
    template<class matrix_type, int width, int height, class t_type> void build_matrix_across(matrix_type& res, int c_y, int c_x) {}
    template<class matrix_type, int width, int height, class t_type, class... t_types> void build_matrix_across(matrix_type& res, int c_y, int c_x, const t_type& targ, const t_types&... targs) {
        res(c_y, c_x)=targ;
        c_x+=1;
        if (c_x==width) {
            c_x=0;
            c_y+=1;
        }
        build_matrix_across<matrix_type, width, height, t_type>(res, c_y, c_x, targs...);
    }
    template<class matrix_type, int width, int height, class t_type> void build_matrix_down(matrix_type& res, int c_y, int c_x) {}
    template<class matrix_type, int width, int height, class t_type, class... t_types> void build_matrix_down(matrix_type& res, int c_y, int c_x, const t_type& targ, const t_types&... targs) {
        res(c_y, c_x)=targ;
        c_y+=1;
        if (c_y==height) {
            c_y=0;
            c_x+=1;
        }
        build_matrix_down<matrix_type, width, height, t_type>(res, c_y, c_x, targs...);
    }
    //
    template<class matrix_type, int width, int height, class vec_type> void build_matrix_across_vec(matrix_type& res, int c_p) {}
    template<class matrix_type, int width, int height, class vec_type, class... t_types> void build_matrix_across_vec(matrix_type& res, int c_p, const vec_type& targ, const t_types&... targs) {
        res.across(c_p, targ);
        c_p+=1;
        build_matrix_across_vec<matrix_type, width, height, vec_type>(res, c_p, targs...);
    }
    template<class matrix_type, int width, int height, class vec_type> void build_matrix_down_vec(matrix_type& res, int c_p) {}
    template<class matrix_type, int width, int height, class vec_type, class... t_types> void build_matrix_down_vec(matrix_type& res, int c_p, const vec_type& targ, const t_types&... targs) {
        res.down(c_p, targ);
        c_p+=1;
        build_matrix_down_vec<matrix_type, width, height, vec_type>(res, c_p, targs...);
    }
    
    template<int num> void assert_not_too_much() {}
    template<int num, class t_type> void assert_not_too_much(const t_type& targ) {
        assert_true<(num>0)>;
    }
    template<int num, class t_type, class... t_types> void assert_not_too_much(const t_type& targ, const t_types&... targs) {
        assert_true<(num>0)>;
        assert_not_too_much<num-1>(targs...);
    }
    template<class... t_types> struct count {};
    template<class t_type, class... t_types> struct count<t_type, t_types...> {
        static const int res=1+count<t_types...>::res;
    };
    template<> struct count<> {
        static const int res=0;
    };
}

namespace down {
    template<class vec_a, class vec_b> matrix_down_base<typename only_if_same_types<typename vec_traits<vec_a>::type, typename vec_traits<vec_b>::type, typename vec_traits<vec_a>::type>::good, vec_b::num, vec_a::num> exterior(const vec_a& a, const vec_b& b) {
        matrix_down_base<typename vec_traits<vec_a>::type, vec_b::num, vec_a::num> res;
        for (int y=0;y<vec_a::num;++y) {
            for (int x=0;x<vec_b::num;++x) {
                res(y,x)=a[y]*b[x];
            }
        }
        return res;
    }
    
    template<int width, int height, class t_type, class... t_types> matrix_down_base<t_type, width, height> build_matrix_across(const t_type& targ, const t_types&... targs) {
        not_public::assert_not_too_much<width*height, t_type, t_types...>;
        matrix_down_base<t_type, width, height> res;
        not_public::build_matrix_across<matrix_down_base<t_type, width, height>, width, height, t_type>(res, 0, 0, targ, targs...);
        return res;
    }
    template<int width, int height, class t_type, class... t_types> matrix_down_base<t_type, width, height> build_matrix_down(const t_type& targ, const t_types&... targs) {
        not_public::assert_not_too_much<width*height, t_type, t_types...>;
        matrix_down_base<t_type, width, height> res;
        not_public::build_matrix_down<matrix_down_base<t_type, width, height>, width, height, t_type>(res, 0, 0, targ, targs...);
        return res;
    }
    template<int width, int height, class t_type, class... t_types> matrix_down_base<t_type, width, height> build_matrix(const t_type& targ, const t_types&... targs) { return build_matrix_across<width, height>(targ, targs...); }
    //
    template<class t_type, class... t_types> matrix_down_base<typename vec_traits<t_type>::type, vec_traits<t_type>::num, not_public::count<t_type, t_types...>::res> build_matrix_across_vec(const t_type& targ, const t_types&... targs) {
        static const int width=vec_traits<t_type>::num;
        static const int height=not_public::count<t_type, t_types...>::res;
        matrix_down_base<typename vec_traits<t_type>::type, width, height> res;
        not_public::build_matrix_across_vec<decltype(res), width, height, t_type>(res, 0, targ, targs...);
        return res;
    }
    template<class t_type, class... t_types> matrix_down_base<typename vec_traits<t_type>::type, not_public::count<t_type, t_types...>::res, vec_traits<t_type>::num> build_matrix_down_vec(const t_type& targ, const t_types&... targs) {
        static const int width=not_public::count<t_type, t_types...>::res;
        static const int height=vec_traits<t_type>::num;
        matrix_down_base<typename vec_traits<t_type>::type, width, height> res;
        not_public::build_matrix_down_vec<decltype(res), width, height, t_type>(res, 0, targ, targs...);
        return res;
    }
    template<class t_type, class... t_types> auto build_matrix_vec(const t_type& targ, const t_types&... targs) -> decltype(build_matrix_across_vec(targ, targs...)) { return build_matrix_across_vec(targ, targs...); }
};

namespace across {
    template<class vec_a, class vec_b> matrix_across_base<typename vec_traits<vec_a>::type, vec_b::num, vec_a::num> exterior(const vec_a& a, const vec_b& b) { return down::exterior(a, b); }
    
    template<int width, int height, class t_type, class... t_types> matrix_across_base<t_type, width, height> build_matrix_across(const t_type& targ, const t_types&... targs) {
        not_public::assert_not_too_much<width*height, t_type, t_types...>;
        matrix_across_base<t_type, width, height> res;
        not_public::build_matrix_across<decltype(res), width, height, t_type>(res, 0, 0, targ, targs...);
        return res;
    }
    template<int width, int height, class t_type, class... t_types> matrix_across_base<t_type, width, height> build_matrix_down(const t_type& targ, const t_types&... targs) {
        not_public::assert_not_too_much<width*height, t_type, t_types...>;
        matrix_across_base<t_type, width, height> res;
        not_public::build_matrix_down<decltype(res), width, height, t_type>(res, 0, 0, targ, targs...);
        return res;
    }
    template<int width, int height, class t_type, class... t_types> matrix_across_base<t_type, width, height> build_matrix(const t_type& targ, const t_types&... targs) { return build_matrix_across<width, height>(targ, targs...); }
    //
    template<class t_type, class... t_types> matrix_across_base<typename vec_traits<t_type>::type, vec_traits<t_type>::num, not_public::count<t_type, t_types...>::res> build_matrix_across_vec(const t_type& targ, const t_types&... targs) {
        static const int width=vec_traits<t_type>::num;
        static const int height=not_public::count<t_type, t_types...>::res;
        matrix_across_base<typename vec_traits<t_type>::type, width, height> res;
        not_public::build_matrix_across_vec<decltype(res), width, height, t_type>(res, 0, targ, targs...);
        return res;
    }
    template<class t_type, class... t_types> matrix_across_base<typename vec_traits<t_type>::type, not_public::count<t_type, t_types...>::res, vec_traits<t_type>::num> build_matrix_down_vec(const t_type& targ, const t_types&... targs) {
        static const int width=not_public::count<t_type, t_types...>::res;
        static const int height=vec_traits<t_type>::num;
        matrix_across_base<typename vec_traits<t_type>::type, width, height> res;
        not_public::build_matrix_down_vec<decltype(res), width, height, t_type>(res, 0, targ, targs...);
        return res;
    }
    template<class t_type, class... t_types> auto build_matrix_vec(const t_type& targ, const t_types&... targs) -> decltype(build_matrix_across_vec(targ, targs...)) { return build_matrix_across_vec(targ, targs...); }
};

template<class matrix> typename matrix_traits<typename only_if<matrix::width==1, typename only_if<matrix::height==1, matrix>::good>::good>::type static_determinant(const matrix& targ) {
    return targ(0, 0);
}
template<class matrix> typename matrix_traits<typename only_if<matrix::width==2, typename only_if<matrix::height==2, matrix>::good>::good>::type static_determinant(const matrix& targ) {
    return targ(0, 0)*targ(1, 1)-targ(0, 1)*targ(1, 0);
}
template<class matrix> typename matrix_traits<typename only_if<matrix::width==3, typename only_if<matrix::height==3, matrix>::good>::good>::type static_determinant(const matrix& targ) {
    int a1=0;
    int a2=1;
    int a3=2;
    
    return
         targ(a1, a1)*targ(a2, a2)*targ(a3, a3)
        +targ(a2, a1)*targ(a3, a2)*targ(a1, a3)
        +targ(a3, a1)*targ(a1, a2)*targ(a2, a3)
        -targ(a1, a1)*targ(a3, a2)*targ(a2, a3)
        -targ(a3, a1)*targ(a2, a2)*targ(a1, a3)
        -targ(a2, a1)*targ(a1, a2)*targ(a3, a3)
    ;
}
/*template<class matrix> typename matrix_traits<typename only_if<matrix::width==4, typename only_if<matrix::height==4, matrix>::good>::good>::type static_determinant(const matrix& targ) {
    return
         targ(0, 3)*targ(1, 2)*targ(2, 1)*targ(3, 0)
        -targ(0, 2)*targ(1, 3)*targ(2, 1)*targ(3, 0)
        -targ(0, 3)*targ(1, 1)*targ(2, 2)*targ(3, 0)
        +targ(0, 1)*targ(1, 3)*targ(2, 2)*targ(3, 0)
        +targ(0, 2)*targ(1, 1)*targ(2, 3)*targ(3, 0)
        -targ(0, 1)*targ(1, 2)*targ(2, 3)*targ(3, 0)
        -targ(0, 3)*targ(1, 2)*targ(2, 0)*targ(3, 1)
        +targ(0, 2)*targ(1, 3)*targ(2, 0)*targ(3, 1)
        +targ(0, 3)*targ(1, 0)*targ(2, 2)*targ(3, 1)
        -targ(0, 0)*targ(1, 3)*targ(2, 2)*targ(3, 1)
        -targ(0, 2)*targ(1, 0)*targ(2, 3)*targ(3, 1)
        +targ(0, 0)*targ(1, 2)*targ(2, 3)*targ(3, 1)
        +targ(0, 3)*targ(1, 1)*targ(2, 0)*targ(3, 2)
        -targ(0, 1)*targ(1, 3)*targ(2, 0)*targ(3, 2)
        -targ(0, 3)*targ(1, 0)*targ(2, 1)*targ(3, 2)
        +targ(0, 0)*targ(1, 3)*targ(2, 1)*targ(3, 2)
        +targ(0, 1)*targ(1, 0)*targ(2, 3)*targ(3, 2)
        -targ(0, 0)*targ(1, 1)*targ(2, 3)*targ(3, 2)
        -targ(0, 2)*targ(1, 1)*targ(2, 0)*targ(3, 3)
        +targ(0, 1)*targ(1, 2)*targ(2, 0)*targ(3, 3)
        +targ(0, 2)*targ(1, 0)*targ(2, 1)*targ(3, 3)
        -targ(0, 0)*targ(1, 2)*targ(2, 1)*targ(3, 3)
        -targ(0, 1)*targ(1, 0)*targ(2, 2)*targ(3, 3)
        +targ(0, 0)*targ(1, 1)*targ(2, 2)*targ(3, 3)
    ;
}*/
//
template<class matrix> typename only_if<matrix::width==1, typename only_if<matrix::height==1, matrix>::good>::good static_inverse(const matrix& targ) {
    matrix res;
    res(0, 0)=targ(0, 0);
    res/=determinant(targ);
    return res;
}
template<class matrix> typename only_if<matrix::width==2, typename only_if<matrix::height==2, matrix>::good>::good static_inverse(const matrix& targ) {
    matrix res;
    res(0, 0)=targ(1, 1);
    res(0, 1)=-targ(0, 1);
    res(1, 0)=-targ(1, 0);
    res(1, 1)=targ(0, 0);
    res/=determinant(targ);
    return res;
}
template<class matrix> typename only_if<matrix::width==3, typename only_if<matrix::height==3, matrix>::good>::good static_inverse(const matrix& targ) {
    int a1=0;
    int a2=1;
    int a3=2;
    
    matrix res;
    res(a1, a1)=targ(a2, a2)*targ(a3, a3) - targ(a2, a3)*targ(a3, a2);
    res(a1, a2)=targ(a1, a3)*targ(a3, a2) - targ(a1, a2)*targ(a3, a3);
    res(a1, a3)=targ(a1, a2)*targ(a2, a3) - targ(a1, a3)*targ(a2, a2);
    res(a2, a1)=targ(a2, a3)*targ(a3, a1) - targ(a2, a1)*targ(a3, a3);
    res(a2, a2)=targ(a1, a1)*targ(a3, a3) - targ(a1, a3)*targ(a3, a1);
    res(a2, a3)=targ(a1, a3)*targ(a2, a1) - targ(a1, a1)*targ(a2, a3);
    res(a3, a1)=targ(a2, a1)*targ(a3, a2) - targ(a2, a2)*targ(a3, a1);
    res(a3, a2)=targ(a1, a2)*targ(a3, a1) - targ(a1, a1)*targ(a3, a2);
    res(a3, a3)=targ(a1, a1)*targ(a2, a2) - targ(a1, a2)*targ(a2, a1);
    res/=determinant(targ);
    return res;
}
/*template<class matrix> typename only_if<matrix::width==4, typename only_if<matrix::height==4, matrix>::good>::good static_inverse(const matrix& targ) {
    matrix res;
    targ(0, 0)=targ(1, 2)*targ(2, 3)*targ(3, 1)-targ(1, 3)*targ(2, 2)*targ(3, 1)+targ(1, 3)*targ(2, 1)*targ(3, 2)-targ(1, 1)*targ(2, 3)*targ(3, 2)-targ(1, 2)*targ(2, 1)*targ(3, 3)+targ(1, 1)*targ(2, 2)*targ(3, 3);
    targ(0, 1)=targ(0, 3)*targ(2, 2)*targ(3, 1)-targ(0, 2)*targ(2, 3)*targ(3, 1)-targ(0, 3)*targ(2, 1)*targ(3, 2)+targ(0, 1)*targ(2, 3)*targ(3, 2)+targ(0, 2)*targ(2, 1)*targ(3, 3)-targ(0, 1)*targ(2, 2)*targ(3, 3);
    targ(0, 2)=targ(0, 2)*targ(1, 3)*targ(3, 1)-targ(0, 3)*targ(1, 2)*targ(3, 1)+targ(0, 3)*targ(1, 1)*targ(3, 2)-targ(0, 1)*targ(1, 3)*targ(3, 2)-targ(0, 2)*targ(1, 1)*targ(3, 3)+targ(0, 1)*targ(1, 2)*targ(3, 3);
    targ(0, 3)=targ(0, 3)*targ(1, 2)*targ(2, 1)-targ(0, 2)*targ(1, 3)*targ(2, 1)-targ(0, 3)*targ(1, 1)*targ(2, 2)+targ(0, 1)*targ(1, 3)*targ(2, 2)+targ(0, 2)*targ(1, 1)*targ(2, 3)-targ(0, 1)*targ(1, 2)*targ(2, 3);
    targ(1, 0)=targ(1, 3)*targ(2, 2)*targ(3, 0)-targ(1, 2)*targ(2, 3)*targ(3, 0)-targ(1, 3)*targ(2, 0)*targ(3, 2)+targ(1, 0)*targ(2, 3)*targ(3, 2)+targ(1, 2)*targ(2, 0)*targ(3, 3)-targ(1, 0)*targ(2, 2)*targ(3, 3);
    targ(1, 1)=targ(0, 2)*targ(2, 3)*targ(3, 0)-targ(0, 3)*targ(2, 2)*targ(3, 0)+targ(0, 3)*targ(2, 0)*targ(3, 2)-targ(0, 0)*targ(2, 3)*targ(3, 2)-targ(0, 2)*targ(2, 0)*targ(3, 3)+targ(0, 0)*targ(2, 2)*targ(3, 3);
    targ(1, 2)=targ(0, 3)*targ(1, 2)*targ(3, 0)-targ(0, 2)*targ(1, 3)*targ(3, 0)-targ(0, 3)*targ(1, 0)*targ(3, 2)+targ(0, 0)*targ(1, 3)*targ(3, 2)+targ(0, 2)*targ(1, 0)*targ(3, 3)-targ(0, 0)*targ(1, 2)*targ(3, 3);
    targ(1, 3)=targ(0, 2)*targ(1, 3)*targ(2, 0)-targ(0, 3)*targ(1, 2)*targ(2, 0)+targ(0, 3)*targ(1, 0)*targ(2, 2)-targ(0, 0)*targ(1, 3)*targ(2, 2)-targ(0, 2)*targ(1, 0)*targ(2, 3)+targ(0, 0)*targ(1, 2)*targ(2, 3);
    targ(2, 0)=targ(1, 1)*targ(2, 3)*targ(3, 0)-targ(1, 3)*targ(2, 1)*targ(3, 0)+targ(1, 3)*targ(2, 0)*targ(3, 1)-targ(1, 0)*targ(2, 3)*targ(3, 1)-targ(1, 1)*targ(2, 0)*targ(3, 3)+targ(1, 0)*targ(2, 1)*targ(3, 3);
    targ(2, 1)=targ(0, 3)*targ(2, 1)*targ(3, 0)-targ(0, 1)*targ(2, 3)*targ(3, 0)-targ(0, 3)*targ(2, 0)*targ(3, 1)+targ(0, 0)*targ(2, 3)*targ(3, 1)+targ(0, 1)*targ(2, 0)*targ(3, 3)-targ(0, 0)*targ(2, 1)*targ(3, 3);
    targ(2, 2)=targ(0, 1)*targ(1, 3)*targ(3, 0)-targ(0, 3)*targ(1, 1)*targ(3, 0)+targ(0, 3)*targ(1, 0)*targ(3, 1)-targ(0, 0)*targ(1, 3)*targ(3, 1)-targ(0, 1)*targ(1, 0)*targ(3, 3)+targ(0, 0)*targ(1, 1)*targ(3, 3);
    targ(2, 3)=targ(0, 3)*targ(1, 1)*targ(2, 0)-targ(0, 1)*targ(1, 3)*targ(2, 0)-targ(0, 3)*targ(1, 0)*targ(2, 1)+targ(0, 0)*targ(1, 3)*targ(2, 1)+targ(0, 1)*targ(1, 0)*targ(2, 3)-targ(0, 0)*targ(1, 1)*targ(2, 3);
    targ(3, 0)=targ(1, 2)*targ(2, 1)*targ(3, 0)-targ(1, 1)*targ(2, 2)*targ(3, 0)-targ(1, 2)*targ(2, 0)*targ(3, 1)+targ(1, 0)*targ(2, 2)*targ(3, 1)+targ(1, 1)*targ(2, 0)*targ(3, 2)-targ(1, 0)*targ(2, 1)*targ(3, 2);
    targ(3, 1)=targ(0, 1)*targ(2, 2)*targ(3, 0)-targ(0, 2)*targ(2, 1)*targ(3, 0)+targ(0, 2)*targ(2, 0)*targ(3, 1)-targ(0, 0)*targ(2, 2)*targ(3, 1)-targ(0, 1)*targ(2, 0)*targ(3, 2)+targ(0, 0)*targ(2, 1)*targ(3, 2);
    targ(3, 2)=targ(0, 2)*targ(1, 1)*targ(3, 0)-targ(0, 1)*targ(1, 2)*targ(3, 0)-targ(0, 2)*targ(1, 0)*targ(3, 1)+targ(0, 0)*targ(1, 2)*targ(3, 1)+targ(0, 1)*targ(1, 0)*targ(3, 2)-targ(0, 0)*targ(1, 1)*targ(3, 2);
    targ(3, 3)=targ(0, 1)*targ(1, 2)*targ(2, 0)-targ(0, 2)*targ(1, 1)*targ(2, 0)+targ(0, 2)*targ(1, 0)*targ(2, 1)-targ(0, 0)*targ(1, 2)*targ(2, 1)-targ(0, 1)*targ(1, 0)*targ(2, 2)+targ(0, 0)*targ(1, 1)*targ(2, 2);
    res/=determinant(targ);
    return res;
}*/

template<class matrix> typename matrix_traits<matrix>::type determinant(const matrix& targ) { assert_true<matrix::width==matrix::height>;
    return static_determinant(targ);
}
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good inverse(const matrix& targ) { assert_true<matrix::width==matrix::height>;
    return static_inverse(targ);
}
//
template<class matrix> typename matrix_traits<matrix>::template only_if<matrix>::good& operator/=(matrix& a, const matrix& b) { assert_true<matrix::width==matrix::height>;
    a*=inverse(b);
}
template<class matrix_a, class matrix_b> typename matrix_bi_traits<matrix_a, matrix_b>::template make_matrix_first<matrix_b::width, matrix_a::height>::res operator/(const matrix_a& a, const matrix_b& b) { assert_true<matrix_b::width==matrix_b::height>;
    return a*inverse(b);
}

//op list: combine_across, combine_down, delete_across, delete_down, static eigenvectors and values
//list of gauss: generalized guass-jordan elimination, inverse, determinant, solution to a system of equations
//generalized inverse
//polynomial roots for 2nd, 3rd, 4th degree
/*
A=[[a b][c d]]
(A-kI)=[[a-k b][c d-k]]
det(A-kI)=ad-bc+k^2-(a+d)k=0

etc for higher order matricies, if I want to bother with this
results are pairs of <eigenvalue, eigenvector> with arbitrary length eigenvectors, arbitrary order eigenvalues [not real, so we can't sort], but all duplicate eigenvalues must be consecutive and compare equal
*/

template<class matrix> typename only_if_matrix<matrix, ostream>::good& operator<<(ostream& out, const matrix& targ) {
    static vector<string> cache;
    static vector<int> padding;
    //
    cache.resize(matrix::num);
    padding.resize(matrix::width);
    for (int x=0;x<matrix::width;++x) {
        padding[x]=0;
        for (int y=0;y<matrix::height;++y) {
            string& c=cache[y*matrix::width+x];
            c=to_string(out, targ(y, x));
            if (c.size()>padding[x]) padding[x]=c.size();
        }
    }
    //
    out << "[";
    for (int y=0;y<matrix::height;++y) {
        if (y>0) out << "\n ";
        for (int x=0;x<matrix::width;++x) {
            if (x>0) out << " ";
            out << setw(padding[x]);
            print_as_number(out, cache[y*matrix::width+x]);
        }
    }
    out << "]";
    return out;
}

namespace matrix_double {
    namespace down {
        typedef matrix_down_base<double, 1, 1> m1x1;
        typedef matrix_down_base<double, 1, 2> m1x2;
        typedef matrix_down_base<double, 1, 3> m1x3;
        typedef matrix_down_base<double, 1, 4> m1x4;
        typedef matrix_down_base<double, 2, 1> m2x1;
        typedef matrix_down_base<double, 2, 2> m2x2;
        typedef matrix_down_base<double, 2, 3> m2x3;
        typedef matrix_down_base<double, 2, 4> m2x4;
        typedef matrix_down_base<double, 3, 1> m3x1;
        typedef matrix_down_base<double, 3, 2> m3x2;
        typedef matrix_down_base<double, 3, 3> m3x3;
        typedef matrix_down_base<double, 3, 4> m3x4;
        typedef matrix_down_base<double, 4, 1> m4x1;
        typedef matrix_down_base<double, 4, 2> m4x2;
        typedef matrix_down_base<double, 4, 3> m4x3;
        typedef matrix_down_base<double, 4, 4> m4x4;
        //
        typedef m1x1 m1;
        typedef m2x2 m2;
        typedef m3x3 m3;
        typedef m4x4 m4;
    }
    
    namespace across {
        typedef matrix_across_base<double, 1, 1> m1x1;
        typedef matrix_across_base<double, 1, 2> m1x2;
        typedef matrix_across_base<double, 1, 3> m1x3;
        typedef matrix_across_base<double, 1, 4> m1x4;
        typedef matrix_across_base<double, 2, 1> m2x1;
        typedef matrix_across_base<double, 2, 2> m2x2;
        typedef matrix_across_base<double, 2, 3> m2x3;
        typedef matrix_across_base<double, 2, 4> m2x4;
        typedef matrix_across_base<double, 3, 1> m3x1;
        typedef matrix_across_base<double, 3, 2> m3x2;
        typedef matrix_across_base<double, 3, 3> m3x3;
        typedef matrix_across_base<double, 3, 4> m3x4;
        typedef matrix_across_base<double, 4, 1> m4x1;
        typedef matrix_across_base<double, 4, 2> m4x2;
        typedef matrix_across_base<double, 4, 3> m4x3;
        typedef matrix_across_base<double, 4, 4> m4x4;
        //
        typedef m1x1 m1;
        typedef m2x2 m2;
        typedef m3x3 m3;
        typedef m4x4 m4;
    }
    
    using namespace across;
}

namespace matrix_float {
    namespace down {
        typedef matrix_down_base<float, 1, 1> m1x1;
        typedef matrix_down_base<float, 1, 2> m1x2;
        typedef matrix_down_base<float, 1, 3> m1x3;
        typedef matrix_down_base<float, 1, 4> m1x4;
        typedef matrix_down_base<float, 2, 1> m2x1;
        typedef matrix_down_base<float, 2, 2> m2x2;
        typedef matrix_down_base<float, 2, 3> m2x3;
        typedef matrix_down_base<float, 2, 4> m2x4;
        typedef matrix_down_base<float, 3, 1> m3x1;
        typedef matrix_down_base<float, 3, 2> m3x2;
        typedef matrix_down_base<float, 3, 3> m3x3;
        typedef matrix_down_base<float, 3, 4> m3x4;
        typedef matrix_down_base<float, 4, 1> m4x1;
        typedef matrix_down_base<float, 4, 2> m4x2;
        typedef matrix_down_base<float, 4, 3> m4x3;
        typedef matrix_down_base<float, 4, 4> m4x4;
        //
        typedef m1x1 m1;
        typedef m2x2 m2;
        typedef m3x3 m3;
        typedef m4x4 m4;
    }
    
    namespace across {
        typedef matrix_across_base<float, 1, 1> m1x1;
        typedef matrix_across_base<float, 1, 2> m1x2;
        typedef matrix_across_base<float, 1, 3> m1x3;
        typedef matrix_across_base<float, 1, 4> m1x4;
        typedef matrix_across_base<float, 2, 1> m2x1;
        typedef matrix_across_base<float, 2, 2> m2x2;
        typedef matrix_across_base<float, 2, 3> m2x3;
        typedef matrix_across_base<float, 2, 4> m2x4;
        typedef matrix_across_base<float, 3, 1> m3x1;
        typedef matrix_across_base<float, 3, 2> m3x2;
        typedef matrix_across_base<float, 3, 3> m3x3;
        typedef matrix_across_base<float, 3, 4> m3x4;
        typedef matrix_across_base<float, 4, 1> m4x1;
        typedef matrix_across_base<float, 4, 2> m4x2;
        typedef matrix_across_base<float, 4, 3> m4x3;
        typedef matrix_across_base<float, 4, 4> m4x4;
        //
        typedef m1x1 m1;
        typedef m2x2 m2;
        typedef m3x3 m3;
        typedef m4x4 m4;
    }
    
    using namespace across;
}
//
using namespace across;

}

#endif
