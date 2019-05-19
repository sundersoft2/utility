#ifndef ILYA_HEADER_GRAPHICS
#define ILYA_HEADER_GRAPHICS

#include <vector>
#include <utility>
#include <iostream>

#include "fixed_size.h"
#include "math_linear_vector.h"

namespace graphics {

using std::vector;
using std::swap;
using std::istream;
using std::ostream;
using std::pair;
using std::make_pair;
using fixed_size::u8;
using fixed_size::u16;
using fixed_size::u32;
using fixed_size::s32;
using fixed_size::read_u8;
using fixed_size::read_u16;
using fixed_size::read_u32;
using fixed_size::read_s32;
using fixed_size::write;
using math::vec_base;

struct rgba {
    union {
        u32 data_u32;
        u8 data_array[4];
        struct {
            u8 r, g, b, a;
        };
    };
    rgba() : r(0), g(0), b(0), a(255) {}
    rgba(u8 t_r, u8 t_g, u8 t_b, u8 t_a=255) : r(t_r), g(t_g), b(t_b), a(t_a) {}
    //
    bool operator==(const rgba& targ) const { return r==targ.r && g==targ.g && b==targ.b && a==targ.a; }
    bool operator!=(const rgba& targ) const { return !(*this==targ); }
    //
    template<class type> vec_base<type, 4> cast() const { return vec_base<type, 4>(type(r), type(g), type(b), type(a)); }
    vec_base<double, 4> cast() const { return cast<double>(); }
    template<class type> vec_base<type, 4> cast_normal() const { return vec_base<type, 4>(type(r)/255, type(g)/255, type(b)/255, type(a)/255); }
    vec_base<double, 4> cast_normal() const { return cast_normal<double>(); }
    template<class type> void assign(const vec_base<type, 4>& targ) { *this=rgba(u8(targ.x+0.5), u8(targ.y+0.5), u8(targ.z+0.5), u8(targ.w+0.5)); }
    template<class type> void assign_normal(const vec_base<type, 4>& targ) { *this=rgba(u8(targ.x*255+0.5), u8(targ.y*255+0.5), u8(targ.z*255+0.5), u8(targ.w*255+0.5)); }
    
    template<class type> static rgba create(const vec_base<type, 4>& targ) {
        rgba res;
        res.assign(targ);
        return res;
    }
    
    template<class type> static rgba create_normal(const vec_base<type, 4>& targ) {
        rgba res;
        res.assign_normal(targ);
        return res;
    }
};

template<class pixel_type> struct image_base {
    vector<pixel_type> data;
    int c_width, c_height;
    
    public:
    template<class iter_pixel_type> class xyiterator_base {
        friend class image_base;
        iter_pixel_type *data;
        int c_width;
        xyiterator_base(iter_pixel_type* t_data, int t_width) : data(t_data), c_width(t_width) {}
        
        public:
        class iterator {
            friend class xyiterator_base;
            iter_pixel_type *data;
            iterator(iter_pixel_type* t_data) : data(t_data) {}
            
            public:
            operator iter_pixel_type() const { return *data; }
            iterator& operator=(const iter_pixel_type& targ) { *data=targ; return *this; }
            iterator& operator++() { ++data; return *this; }
            iterator operator++(int) { iterator res=*this; ++res; return res; }
            iterator& operator--() { --data; return *this; }
            iterator operator--(int) { iterator res=*this; --res; return res; }
            iterator& operator+=(int amount) { data+=amount; return *this; }
            iterator& operator-=(int amount) { data-=amount; return *this; }
            iterator operator+(int amount) const { iterator res=*this; res+=amount; return res; }
            iterator operator-(int amount) const { iterator res=*this; res-=amount; return res; }
            bool operator==(const iterator& targ) const { return data==targ.data; }
            bool operator!=(const iterator& targ) const { return data!=targ.data; }
            iter_pixel_type& operator*() const { return *data; }
            iter_pixel_type* operator->() const { return data; }
        };
        
        iter_pixel_type& operator[](int t_pos) { return *(data+t_pos); }
        const iter_pixel_type& operator[](int t_pos) const { return *(data+t_pos); }
        iterator begin() { return iterator(data); }
        iterator end() { return iterator(data+c_width); }
        xyiterator_base& operator++() { data+=c_width; return *this; }
        xyiterator_base operator++(int) { xyiterator_base res=*this; ++res; return res; }
        xyiterator_base& operator--() { data-=c_width; return *this; }
        xyiterator_base operator--(int) { xyiterator_base res=*this; --res; return res; }
        xyiterator_base& operator+=(int amount) { data+=c_width*amount; return *this; }
        xyiterator_base& operator-=(int amount) { data-=c_width*amount; return *this; }
        xyiterator_base operator+(int amount) const { xyiterator_base res=*this; res+=amount; return res; }
        xyiterator_base operator-(int amount) const { xyiterator_base res=*this; res-=amount; return res; }
        bool operator==(const xyiterator_base& targ) const { return data==targ.data; }
        bool operator!=(const xyiterator_base& targ) const { return data!=targ.data; }
    };
    
    template<class iter_pixel_type> class iterator_base {
        friend class image_base;
        iter_pixel_type *data;
        iterator_base() : data() {}
        iterator_base(iter_pixel_type* t_data) : data(t_data) {}
        
        public:
        iter_pixel_type& operator*() { return *data; }
        const iter_pixel_type& operator*() const { return *data; }
        iter_pixel_type* operator->() { return &(*data); }
        const iter_pixel_type* operator->() const { return &(*data); }
        iterator_base& operator++() { ++data; return *this; }
        iterator_base operator++(int) { iterator_base res=*this; ++res; return res; }
        iterator_base& operator--() { --data; return *this; }
        iterator_base operator--(int) { iterator_base res=*this; --res; return res; }
        iterator_base& operator+=(int amount) { data+=amount; return *this; }
        iterator_base& operator-=(int amount) { data-=amount; return *this; }
        iterator_base operator+(int amount) const { iterator_base res=*this; res+=amount; return res; }
        iterator_base operator-(int amount) const { iterator_base res=*this; res-=amount; return res; }
        bool operator==(const iterator_base& targ) const { return data==targ.data; }
        bool operator!=(const iterator_base& targ) const { return data!=targ.data; }
    };
    
    typedef xyiterator_base<pixel_type> xyiterator;
    typedef xyiterator_base<const pixel_type> const_xyiterator;
    typedef iterator_base<pixel_type> iterator;
    typedef iterator_base<const pixel_type> const_iterator;
    
    image_base() : c_width(0), c_height(0) {}
    image_base(int t_width, int t_height) {
        resize(t_width, t_height);
    }
    int width() const { return c_width; }
    int height() const { return c_height; }
    int size() const { return data.size(); }
    void resize(int t_width, int t_height) {
        data.clear();
        data.resize(t_width*t_height);
        c_width=t_width;
        c_height=t_height;
        if (c_width==0 || c_height==0) c_width=0, c_height=0;
    }
    void swap(image_base& targ) {
        swap(c_width, targ.c_width);
        swap(c_height, targ.c_height);
        swap(data, targ.data);
    }
    void fill(pixel_type targ) {
        pixel_type *c=&(data.front());
        pixel_type *c_end=&(data.back())+1;
        for (;c!=c_end;++c) *c=targ;
    }
    void flip() {
        swap(c_width, c_height);
        vector<pixel_type> data_trans;
        data_trans.resize(data.size());
        int stride=c_height;
        int offset=0;
        int max=data.size();
        int pos=0;
        for (int x=0;x<stride;++x) {
            for (int y=x;y<max;y+=stride,++pos) {
                data_trans[pos]=data[y];
            }
        }
        std::swap(data, data_trans);
    }
    
    xyiterator operator[](int y) {
        return xyiterator(&(data.front())+y*c_width, c_width);
    }
    xyiterator begin() {
        return xyiterator(&(data.front()), c_width);
    }
    xyiterator end() {
        return xyiterator(&(data.back())+1, c_width);
    }
    iterator pbegin() { return iterator(&(data.front())); }
    iterator pend() { return iterator(&(data.back())+1); }
    //
    const_xyiterator operator[](int pos) const {
        return const_xyiterator(&(data.front())+pos*c_width, c_width);
    }
    const_xyiterator begin() const {
        return const_xyiterator(&(data.front()), c_width);
    }
    const_xyiterator end() const {
        return const_xyiterator(&(data.back())+1, c_width);
    }
    const_iterator pbegin() const { return const_iterator(&(data.front())); }
    const_iterator pend() const { return const_iterator(&(data.back())+1); }
};

template<class pixel_type> class fixed_image_ref_base {
    image_base<pixel_type> *ptr;
    
    public:
    typedef typename image_base<pixel_type>::template xyiterator_base<pixel_type> xyiterator;
    typedef typename image_base<pixel_type>::template xyiterator_base<const pixel_type> const_xyiterator;
    typedef typename image_base<pixel_type>::template iterator_base<pixel_type> iterator;
    typedef typename image_base<pixel_type>::template iterator_base<const pixel_type> const_iterator;
    
    fixed_image_ref_base(image_base<pixel_type>& targ) : ptr(&targ) {}
    
    int width() const { return ptr->width(); }
    int height() const { return ptr->height(); }
    int size() const { return ptr->size(); }
    void fill(pixel_type targ) { ptr->fill(targ); }
    
    xyiterator operator[](int pos) { return (*ptr)[pos]; }
    xyiterator begin() { return ptr->begin(); }
    xyiterator end() { return ptr->end(); }
    iterator pbegin() { return ptr->pbegin(); }
    iterator pend() { return ptr->pend(); }
    //
    const_xyiterator operator[](int pos) const { return (*ptr)[pos]; }
    const_xyiterator begin() const { return ptr->begin(); }
    const_xyiterator end() const { return ptr->end(); }
    const_iterator pbegin() const { return ptr->pbegin(); }
    const_iterator pend() const { return ptr->pend(); }
};

typedef image_base<rgba> image_rgba;
typedef fixed_image_ref_base<rgba> fixed_rgba;

/*template<class image_type> void mirror_main(image_base<image_type>& targ) {
    int width=targ.width();
    int height=targ.height();
    int half_width=width/2;
    for (int x1=0;x1<half_width;++x1) {
        int x2=width-1-x1;
        auto x1_i=targ[x1];
        auto x2_i=targ[x2];
        for (int y=0;y<height;++y) {
            swap(*(x1_i[y]), *(x2_i[y]));
        }
    }
}*/

image_rgba read_bmp(istream& targ, bool lower_left_origin=false) {
    image_rgba res(0, 0);
    assert(read_u8(targ)==0x42);
    assert(read_u8(targ)==0x4D);
    targ.ignore(12);
    assert(read_u32(targ)==40); //format
    s32 width=read_s32(targ);
    s32 height=read_s32(targ);
    bool reverse_y=1;
    if (height<0) { height=-height; reverse_y=0; }
    assert(width>=0 && height>=0);
    targ.ignore(2);
    u16 depth=read_u16(targ);
    assert(depth>=24);
    assert(read_u32(targ)==0); //compression
    targ.ignore(20);
    res.resize(width, height);
    int row_padding=0;
    if (depth==24) {
        row_padding=(width*3)&3;
        if (row_padding!=0) row_padding=4-row_padding;
    }
    int pixel_padding=0;
    if (depth==32) pixel_padding=1;
    
    if (lower_left_origin) {
        reverse_y=!reverse_y;
    }
    
    if (reverse_y) {
        for (int y=height-1;y>=0;--y) {
            image_rgba::xyiterator::iterator res_pos=res[y].begin();
            for (int x=0;x<width;++x) {
                rgba data;
                data.b=read_u8(targ);
                data.g=read_u8(targ);
                data.r=read_u8(targ);
                targ.ignore(pixel_padding);
                res_pos=data;
                ++res_pos;
            }
            targ.ignore(row_padding);
        }
    } else {
        image_rgba::iterator res_pos=res.pbegin();
        for (int y=0;y<height;++y) {
            for (int x=0;x<width;++x) {
                rgba data;
                data.b=read_u8(targ);
                data.g=read_u8(targ);
                data.r=read_u8(targ);
                targ.ignore(pixel_padding);
                *res_pos=data;
                ++res_pos;
            }
            targ.ignore(row_padding);
        }
    }
    return res;
}

void write_bmp(const image_rgba& in, ostream& out, bool lower_left_origin=false) {
    int row_padding=(in.width()*3)&3;
    if (row_padding!=0) row_padding=4-row_padding;
    write(u8(0x42), out);
    write(u8(0x4D), out);
    write(u8(0x36), out);
    write(u8(0x0C), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x36), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x28), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(s32(in.width()), out);
    write(s32(in.height()), out);
    write(u8(0x01), out);
    write(u8(0x00), out);
    write(u8(0x18), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u32(in.height()*(row_padding+in.width()*3)), out);
    write(u8(0xC2), out);
    write(u8(0x1E), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0xC2), out);
    write(u8(0x1E), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    write(u8(0x00), out);
    for (int y=(lower_left_origin? 0 : in.height()-1);(lower_left_origin? y<in.height() : y>=0);(lower_left_origin? ++y : --y)) {
        image_rgba::const_xyiterator::iterator res_pos=in[y].begin();
        for (int x=0;x<in.width();++x) {
            write(u8(rgba(res_pos).b), out);
            write(u8(rgba(res_pos).g), out);
            write(u8(rgba(res_pos).r), out);
            ++res_pos;
        }
        for (int x=0;x<row_padding;++x) write(u8(0x00), out);
    }
}

}
#endif
