#ifndef ILYA_HEADER_OGL_ENUMERATION
#define ILYA_HEADER_OGL_ENUMERATION

#include <gl/gl.h>
#include <gl/glext.h>
#include <glex/GLEX.c>

namespace ogl {

class image_format {
    image_format() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static image_format generate(GLuint targ) { image_format res; res.data=targ; return res; }
    
    static image_format rgba8;
    static image_format rgba16;
    static image_format rgba_f16;
    static image_format rgba_f32;
    static image_format rgba_s8;
    static image_format rgba_s16;
    static image_format rgba_s32;
    static image_format rgba_u8;
    static image_format rgba_u16;
    static image_format rgba_u32;
    static image_format rg8;
    static image_format rg16;
    static image_format rg_f16;
    static image_format rg_f32;
    static image_format rg_s8;
    static image_format rg_s16;
    static image_format rg_s32;
    static image_format rg_u8;
    static image_format rg_u16;
    static image_format rg_u32;
    static image_format r8;
    static image_format r16;
    static image_format r_f16;
    static image_format r_f32;
    static image_format r_s8;
    static image_format r_s16;
    static image_format r_s32;
    static image_format r_u8;
    static image_format r_u16;
    static image_format r_u32;
    static image_format depth16;
    static image_format depth24;
    static image_format depth_f32;
    static image_format depth24_stencil8;
    static image_format depth_f32_stencil8;
    
    bool is_color() const { return !is_depth(); }
    bool is_depth() const {
        return
            data==depth16.data ||
            data==depth24.data ||
            data==depth_f32.data ||
            is_depth_stencil()
        ;
    }
    bool is_depth_stencil() const {
        return
            data==depth24_stencil8.data ||
            data==depth_f32_stencil8.data
        ;
    }
};
image_format image_format::rgba8=image_format::generate(GL_RGBA8);
image_format image_format::rgba16=image_format::generate(GL_RGBA16);
image_format image_format::rgba_f16=image_format::generate(GL_RGBA16F);
image_format image_format::rgba_f32=image_format::generate(GL_RGBA32F);
image_format image_format::rgba_s8=image_format::generate(GL_RGBA8I);
image_format image_format::rgba_s16=image_format::generate(GL_RGBA16I);
image_format image_format::rgba_s32=image_format::generate(GL_RGBA32I);
image_format image_format::rgba_u8=image_format::generate(GL_RGBA8UI);
image_format image_format::rgba_u16=image_format::generate(GL_RGBA16UI);
image_format image_format::rgba_u32=image_format::generate(GL_RGBA32UI);
image_format image_format::rg8=image_format::generate(GL_RG8);
image_format image_format::rg16=image_format::generate(GL_RG16);
image_format image_format::rg_f16=image_format::generate(GL_RG16F);
image_format image_format::rg_f32=image_format::generate(GL_RG32F);
image_format image_format::rg_s8=image_format::generate(GL_RG8I);
image_format image_format::rg_s16=image_format::generate(GL_RG16I);
image_format image_format::rg_s32=image_format::generate(GL_RG32I);
image_format image_format::rg_u8=image_format::generate(GL_RG8UI);
image_format image_format::rg_u16=image_format::generate(GL_RG16UI);
image_format image_format::rg_u32=image_format::generate(GL_RG32UI);
image_format image_format::r8=image_format::generate(GL_R8);
image_format image_format::r16=image_format::generate(GL_R16);
image_format image_format::r_f16=image_format::generate(GL_R16F);
image_format image_format::r_f32=image_format::generate(GL_R32F);
image_format image_format::r_s8=image_format::generate(GL_R8I);
image_format image_format::r_s16=image_format::generate(GL_R16I);
image_format image_format::r_s32=image_format::generate(GL_R32I);
image_format image_format::r_u8=image_format::generate(GL_R8UI);
image_format image_format::r_u16=image_format::generate(GL_R16UI);
image_format image_format::r_u32=image_format::generate(GL_R32UI);
image_format image_format::depth16=image_format::generate(GL_DEPTH_COMPONENT16);
image_format image_format::depth24=image_format::generate(GL_DEPTH_COMPONENT24);
image_format image_format::depth_f32=image_format::generate(GL_DEPTH_COMPONENT32F);
image_format image_format::depth24_stencil8=image_format::generate(GL_DEPTH24_STENCIL8);
image_format image_format::depth_f32_stencil8=image_format::generate(GL_DEPTH32F_STENCIL8);



class framebuffer_attachment {
    framebuffer_attachment() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static framebuffer_attachment generate(GLuint targ) { framebuffer_attachment res; res.data=targ; return res; }
    
    static framebuffer_attachment depth;
    static framebuffer_attachment color0;
};
framebuffer_attachment framebuffer_attachment::depth=framebuffer_attachment::generate(GL_DEPTH_ATTACHMENT);
framebuffer_attachment framebuffer_attachment::color0=framebuffer_attachment::generate(GL_COLOR_ATTACHMENT0);



class shader_type {
    shader_type() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static shader_type generate(GLuint targ) { shader_type res; res.data=targ; return res; }
    
    static shader_type vertex;
    static shader_type fragment;
};
shader_type shader_type::vertex=shader_type::generate(GL_VERTEX_SHADER);
shader_type shader_type::fragment=shader_type::generate(GL_FRAGMENT_SHADER);



class logic_op {
    logic_op() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static logic_op generate(GLuint targ) { logic_op res; res.data=targ; return res; }
    
    static logic_op clear;
    static logic_op set;
    static logic_op copy;
    static logic_op copy_inverted;
    static logic_op no_op;
    static logic_op invert;
    static logic_op op_and;
    static logic_op op_nand;
    static logic_op op_or;
    static logic_op op_nor;
    static logic_op op_xor;
    static logic_op op_nxor;
    static logic_op and_reverse;
    static logic_op and_inverted;
    static logic_op or_reverse;
    static logic_op or_inverted;
};
logic_op logic_op::clear=logic_op::generate(GL_CLEAR);
logic_op logic_op::set=logic_op::generate(GL_SET);
logic_op logic_op::copy=logic_op::generate(GL_COPY);
logic_op logic_op::copy_inverted=logic_op::generate(GL_COPY_INVERTED);
logic_op logic_op::no_op=logic_op::generate(GL_NOOP);
logic_op logic_op::invert=logic_op::generate(GL_INVERT);
logic_op logic_op::op_and=logic_op::generate(GL_AND);
logic_op logic_op::op_nand=logic_op::generate(GL_NAND);
logic_op logic_op::op_or=logic_op::generate(GL_OR);
logic_op logic_op::op_nor=logic_op::generate(GL_NOR);
logic_op logic_op::op_xor=logic_op::generate(GL_XOR);
logic_op logic_op::op_nxor=logic_op::generate(GL_EQUIV);
logic_op logic_op::and_reverse=logic_op::generate(GL_AND_REVERSE);
logic_op logic_op::and_inverted=logic_op::generate(GL_AND_INVERTED);
logic_op logic_op::or_reverse=logic_op::generate(GL_OR_REVERSE);
logic_op logic_op::or_inverted=logic_op::generate(GL_OR_INVERTED);



class blend_function {
    blend_function() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static blend_function generate(GLuint targ) { blend_function res; res.data=targ; return res; }
    
    static blend_function zero;
    static blend_function one;
    static blend_function src_color;
    static blend_function one_minus_src_color;
    static blend_function dst_color;
    static blend_function one_minus_dst_color;
    static blend_function src_alpha;
    static blend_function one_minus_src_alpha;
    static blend_function dst_alpha;
    static blend_function one_minus_dst_alpha;
    static blend_function constant_color;
    static blend_function one_minus_constant_color;
    static blend_function constant_alpha;
    static blend_function one_minus_constant_alpha;
    static blend_function src_alpha_saturate;
};
blend_function blend_function::zero=blend_function::generate(GL_ZERO);
blend_function blend_function::one=blend_function::generate(GL_ONE);
blend_function blend_function::src_color=blend_function::generate(GL_SRC_COLOR);
blend_function blend_function::one_minus_src_color=blend_function::generate(GL_ONE_MINUS_SRC_COLOR);
blend_function blend_function::dst_color=blend_function::generate(GL_DST_COLOR);
blend_function blend_function::one_minus_dst_color=blend_function::generate(GL_ONE_MINUS_DST_COLOR);
blend_function blend_function::src_alpha=blend_function::generate(GL_SRC_ALPHA);
blend_function blend_function::one_minus_src_alpha=blend_function::generate(GL_ONE_MINUS_SRC_ALPHA);
blend_function blend_function::dst_alpha=blend_function::generate(GL_DST_ALPHA);
blend_function blend_function::one_minus_dst_alpha=blend_function::generate(GL_ONE_MINUS_DST_ALPHA);
blend_function blend_function::constant_color=blend_function::generate(GL_CONSTANT_COLOR);
blend_function blend_function::one_minus_constant_color=blend_function::generate(GL_ONE_MINUS_CONSTANT_COLOR);
blend_function blend_function::constant_alpha=blend_function::generate(GL_CONSTANT_ALPHA);
blend_function blend_function::one_minus_constant_alpha=blend_function::generate(GL_ONE_MINUS_CONSTANT_ALPHA);
blend_function blend_function::src_alpha_saturate=blend_function::generate(GL_SRC_ALPHA_SATURATE);


class blend_equation {
    blend_equation() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static blend_equation generate(GLuint targ) { blend_equation res; res.data=targ; return res; }
    
    static blend_equation add;
    static blend_equation subtract;
    static blend_equation reverse_subtract;
    static blend_equation min;
    static blend_equation max;
};
blend_equation blend_equation::add=blend_equation::generate(GL_FUNC_ADD);
blend_equation blend_equation::subtract=blend_equation::generate(GL_FUNC_SUBTRACT);
blend_equation blend_equation::reverse_subtract=blend_equation::generate(GL_FUNC_REVERSE_SUBTRACT);
blend_equation blend_equation::min=blend_equation::generate(GL_MIN);
blend_equation blend_equation::max=blend_equation::generate(GL_MAX);



class depth_function {
    depth_function() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static depth_function generate(GLuint targ) { depth_function res; res.data=targ; return res; }
    
    static depth_function always_false;
    static depth_function always_true;
    static depth_function incoming_less;
    static depth_function incoming_less_or_equal;
    static depth_function incoming_greater;
    static depth_function incoming_greater_or_equal;
    static depth_function equal;
    static depth_function not_equal;
};
depth_function depth_function::always_false=depth_function::generate(GL_NEVER);
depth_function depth_function::always_true=depth_function::generate(GL_ALWAYS);
depth_function depth_function::incoming_less=depth_function::generate(GL_LESS);
depth_function depth_function::incoming_less_or_equal=depth_function::generate(GL_LEQUAL);
depth_function depth_function::incoming_greater=depth_function::generate(GL_GREATER);
depth_function depth_function::incoming_greater_or_equal=depth_function::generate(GL_GEQUAL);
depth_function depth_function::equal=depth_function::generate(GL_EQUAL);
depth_function depth_function::not_equal=depth_function::generate(GL_NOTEQUAL);



class primitive_type {
    primitive_type() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static primitive_type generate(GLuint targ) { primitive_type res; res.data=targ; return res; }
    
    static primitive_type points;
    static primitive_type line_strip;
    static primitive_type line_loop;
    static primitive_type lines;
    static primitive_type line_strip_adjacency;
    static primitive_type lines_adjacency;
    static primitive_type triangle_strip;
    static primitive_type triangle_fan;
    static primitive_type triangles;
    static primitive_type triangle_strip_adjacency;
    static primitive_type triangles_adjacency;
};
primitive_type primitive_type::points=primitive_type::generate(GL_POINTS);
primitive_type primitive_type::line_strip=primitive_type::generate(GL_LINE_STRIP);
primitive_type primitive_type::line_loop=primitive_type::generate(GL_LINE_LOOP);
primitive_type primitive_type::lines=primitive_type::generate(GL_LINES);
primitive_type primitive_type::line_strip_adjacency=primitive_type::generate(GL_LINE_STRIP_ADJACENCY);
primitive_type primitive_type::lines_adjacency=primitive_type::generate(GL_LINES_ADJACENCY);
primitive_type primitive_type::triangle_strip=primitive_type::generate(GL_TRIANGLE_STRIP);
primitive_type primitive_type::triangle_fan=primitive_type::generate(GL_TRIANGLE_FAN);
primitive_type primitive_type::triangles=primitive_type::generate(GL_TRIANGLES);
primitive_type primitive_type::triangle_strip_adjacency=primitive_type::generate(GL_TRIANGLE_STRIP_ADJACENCY);
primitive_type primitive_type::triangles_adjacency=primitive_type::generate(GL_TRIANGLES_ADJACENCY);



class min_filter {
    min_filter() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static min_filter generate(GLuint targ) { min_filter res; res.data=targ; return res; }
    
    static min_filter nearest;
    static min_filter linear;
    static min_filter nearest_mipmap_nearest;
    static min_filter nearest_mipmap_linear;
    static min_filter linear_mipmap_nearest;
    static min_filter linear_mipmap_linear;
};
min_filter min_filter::nearest=min_filter::generate(GL_NEAREST);
min_filter min_filter::linear=min_filter::generate(GL_LINEAR);
min_filter min_filter::nearest_mipmap_nearest=min_filter::generate(GL_NEAREST_MIPMAP_NEAREST);
min_filter min_filter::nearest_mipmap_linear=min_filter::generate(GL_NEAREST_MIPMAP_LINEAR);
min_filter min_filter::linear_mipmap_nearest=min_filter::generate(GL_LINEAR_MIPMAP_NEAREST);
min_filter min_filter::linear_mipmap_linear=min_filter::generate(GL_LINEAR_MIPMAP_LINEAR);



class mag_filter {
    mag_filter() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static mag_filter generate(GLuint targ) { mag_filter res; res.data=targ; return res; }
    
    static mag_filter nearest;
    static mag_filter linear;
};
mag_filter mag_filter::nearest=mag_filter::generate(GL_NEAREST);
mag_filter mag_filter::linear=mag_filter::generate(GL_LINEAR);



class wrap_mode {
    wrap_mode() {}
    GLuint data;
    
    public:
    GLuint internal() const { return data; }
    static wrap_mode generate(GLuint targ) { wrap_mode res; res.data=targ; return res; }
    
    static wrap_mode clamp_to_edge;
    static wrap_mode mirrored_repeat;
    static wrap_mode repeat;
};
wrap_mode wrap_mode::clamp_to_edge=wrap_mode::generate(GL_CLAMP_TO_EDGE);
wrap_mode wrap_mode::mirrored_repeat=wrap_mode::generate(GL_MIRRORED_REPEAT);
wrap_mode wrap_mode::repeat=wrap_mode::generate(GL_REPEAT);



}

#endif
