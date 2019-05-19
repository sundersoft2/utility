#ifndef ILYA_HEADER_OGL_RENDER
#define ILYA_HEADER_OGL_RENDER

#include <gl/gl.h>
#include <gl/glext.h>
#include <glex/GLEX.c>

#include <string>
#include <cctype>
#include <vector>
#include <memory>

#define MAIN_OLD main

#include <sdl/sdl.h>

#ifdef main
    #undef main
    #define main MAIN_OLD
#endif

#include <shared/graphics.h>
#include <shared/math_linear.h>

#include "ogl_enumeration.h"
#include "ogl_state.h"

#define opengl_main(...) \
    opengl_main_inner(int argc, char** argv); \
    int main(int argc, char** argv) { \
        try {\
            SDL_Init(0);\
            SDL_InitSubSystem(SDL_INIT_VIDEO);\
            SDL_SetVideoMode(100, 100, 32, SDL_OPENGLBLIT);\
            GLEX_init();\
            int ret=opengl_main_inner(argc, argv);\
            SDL_Quit();\
            return ret;\
        } catch (const shader::errors& targ) {\
            cout << "Shader error: " << targ.data;\
            char *f=0; *f=1;\
        } catch (const program::errors& targ) {\
            cout << "Program error: " << targ.data;\
            char *f=0; *f=1;\
        }\
    }\
    int opengl_main_inner(int argc, char** argv)

namespace ogl {

using std::string;
using std::isspace;
using std::move;
using std::swap;
using std::vector;
using math::matrix_across_base;
using math::matrix_down_base;
using math::vec_base;

class render_command {
    public:
    virtual void run_internal() const=0;
    virtual ~render_command() {}
};



class clearing_setup : public render_command {
    bool should_clear_depth;
    bool should_clear_color;
    GLdouble clearing_depth;
    vec_base<GLdouble, 4> clearing_color;
    
    public:
    template<class vec_type> clearing_setup(
        bool t_clear_color,
        bool t_clear_depth,
        const vec_base<vec_type, 4> value_color,
        double value_depth=1.0
    ) : should_clear_depth(0), should_clear_color(0) {
        if (t_clear_color) clear_color(value_color);
        if (t_clear_depth) clear_depth(value_depth);
    }
    clearing_setup(bool t_clear_color=0, bool t_clear_depth=0) : should_clear_depth(0), should_clear_color(0) {
        if (t_clear_color) clear_color();
        if (t_clear_depth) clear_depth();
    }
    
    void clear_depth(double value=1.0) {
        should_clear_depth=1;
        clearing_depth=value;
    }
    
    void keep_depth() {
        should_clear_depth=0;
    }
    
    template<class vec_type> void clear_color(const vec_base<vec_type, 4>& value) {
        should_clear_color=1;
        clearing_color.x=value.x;
        clearing_color.y=value.y;
        clearing_color.z=value.z;
        clearing_color.w=value.w;
    }
    void clear_color() {
        should_clear_color=1;
        clearing_color.x=0;
        clearing_color.y=0;
        clearing_color.z=0;
        clearing_color.w=0;
    }
    
    void keep_color() {
        should_clear_color=0;
    }
    
    void run_internal() const {
        if (should_clear_depth) glClearDepth(clearing_depth);
        if (should_clear_color) glClearColor(clearing_color.x, clearing_color.y, clearing_color.z, clearing_color.w);
        GLuint glclear_arg=(should_clear_depth? GL_DEPTH_BUFFER_BIT : 0) | (should_clear_color? GL_COLOR_BUFFER_BIT : 0);
        if (glclear_arg) glClear(glclear_arg);
    }
    static void run_default() {}
};



class depth_setup : public render_command {
    GLuint depth_func;
    GLboolean depth_mask;
    
    public:
    depth_setup(depth_function t_func=depth_function::incoming_less, bool t_mask=1) {
        function(t_func);
        allow_writing(t_mask);
    }
    
    void function(depth_function targ) {
        depth_func=targ.internal();
    }
    
    void allow_writing(bool t_depth_mask) {
        depth_mask=t_depth_mask? GL_TRUE : GL_FALSE;
    }
    
    void run_internal() const {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depth_func);
        glDepthMask(depth_mask);
    }
    static void run_default() {
        glDisable(GL_DEPTH_TEST);
    }
};



class color_setup : public render_command {
    GLboolean mask_r;
    GLboolean mask_g;
    GLboolean mask_b;
    GLboolean mask_a;
    
    virtual void run_remainder() const {
        glDisable(GL_BLEND);
        glDisable(GL_COLOR_LOGIC_OP);
    }
    
    public:
    color_setup(bool r=1, bool g=1, bool b=1, bool a=1) {
        allow_writing(r, g, b, a);
    }
    
    void allow_writing(bool r, bool g, bool b, bool a) {
        mask_r=r? GL_TRUE : GL_FALSE;
        mask_g=g? GL_TRUE : GL_FALSE;
        mask_b=b? GL_TRUE : GL_FALSE;
        mask_a=a? GL_TRUE : GL_FALSE;
    }
    
    virtual void run_internal() const {
        glColorMask(mask_r, mask_g, mask_b, mask_a);
        run_remainder();
    }
    static void run_default() {
        glDisable(GL_BLEND);
        glDisable(GL_COLOR_LOGIC_OP);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
};



class color_setup_logic : public color_setup {
    GLuint c_op;
    
    virtual void run_remainder() const {
        glDisable(GL_BLEND);
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(c_op);
    }
    
    public:
    color_setup_logic(logic_op targ=logic_op::copy, bool r=1, bool g=1, bool b=1, bool a=1) : color_setup(r, g, b, a) {
        function(targ);
    }
    
    void function(logic_op targ) {
        c_op=targ.internal();
    }
};



class color_setup_blend : public color_setup {
    vec_base<GLdouble, 4> blending_color;
    GLuint color_func;
    GLuint alpha_func;
    GLuint source_color;
    GLuint source_alpha;
    GLuint dest_color;
    GLuint dest_alpha;
    
    virtual void run_remainder() const {
        glEnable(GL_BLEND);
        glDisable(GL_COLOR_LOGIC_OP);
        glBlendFuncSeparate(source_color, dest_color, source_alpha, dest_alpha);
        glBlendEquationSeparate(color_func, alpha_func);
        glBlendColor(blending_color.x, blending_color.y, blending_color.z, blending_color.w);
    }
    
    public:
    color_setup_blend() : color_func(GL_FUNC_ADD), alpha_func(GL_FUNC_ADD), source_color(GL_ONE), source_alpha(GL_ONE), dest_color(GL_ZERO), dest_alpha(GL_ZERO) {}
    
    void equation(blend_equation color, blend_equation alpha) {
        color_func=color.internal();
        alpha_func=alpha.internal();
    }
    void equation(blend_equation targ) { equation(targ, targ); }
    
    void source(blend_function color, blend_function alpha) {
        source_color=color.internal();
        source_alpha=alpha.internal();
        
    }
    void source(blend_function targ) { source(targ, targ); }
    
    void destination(blend_function color, blend_function alpha) {
        dest_color=color.internal();
        dest_alpha=alpha.internal();
    }
    void destination(blend_function targ) { destination(targ, targ); }
    
    void color(const vec_base<double, 4>& targ) {
        blending_color.x=targ.x;
        blending_color.y=targ.y;
        blending_color.z=targ.z;
        blending_color.w=targ.w;
    }
};



class culling_setup : public render_command {
    GLuint front_face;
    bool use_culling;
    GLuint cull_face;
    
    public:
    culling_setup() : front_face(GL_CCW), use_culling(0) {}
    
    void clockwise(bool yes=1) {
        front_face=yes? GL_CW : GL_CCW;
    }
    
    void counterclockwise(bool yes=1) {
        front_face=yes? GL_CCW : GL_CW;
    }
    
    void cull(bool front=0, bool back=1) {
        use_culling=front || back;
        cull_face=front? (back? GL_FRONT_AND_BACK : GL_FRONT) : GL_BACK;
    }
    
    void run_internal() const {
        if (use_culling) {
            glEnable(GL_CULL_FACE);
            glCullFace(cull_face);
        } else {
            glDisable(GL_CULL_FACE);
        }
        glFrontFace(front_face);
    }
    static void run_default() {
        glDisable(GL_CULL_FACE);
        glFrontFace(GL_FRONT);
    }
};



class selection_setup {
    public:
    
    virtual void run_internal() const=0;
    virtual ~selection_setup() {}
};



class selection_batch : public selection_setup {
    vector<const selection_setup*> targs;
    
    public:
    selection_batch() {}
    selection_batch(const vector<const selection_setup*>& t_targs) : targs(t_targs) {}
    selection_batch(vector<const selection_setup*>&& t_targs) : targs(move(t_targs)) {}
    
    void bind(const selection_setup* targ) {
        targs.push_back(targ);
    }
    void bind(selection_setup& targ) { bind(&targ); }
    
    void run_internal() const {
        for (int x=0;x<targs.size();++x) {
            if (targs[x]) targs[x]->run_internal();
        }
    }
};



class selection_interval : public selection_setup {
    int min;
    int count;
    GLenum mode;
    
    public:
    selection_interval(int t_max=0) : min(0), count(t_max), mode(GL_TRIANGLES) {}
    selection_interval(int t_min, int t_max) : min(t_min), count(t_max-t_min), mode(GL_TRIANGLES) {}
    selection_interval(primitive_type t_mode, int t_max=0) : min(0), count(t_max), mode(t_mode.internal()) {}
    selection_interval(primitive_type t_mode, int t_min, int t_max) : min(t_min), count(t_max-t_min), mode(t_mode.internal()) {}
    
    void run_internal() const {
        glDrawArrays(mode, min, count);
    }
};



class selection_indexed : public selection_setup {
    GLenum type;
    GLenum mode;
    int num;
    const void* data;
    
    public:
    selection_indexed(int t_num, const unsigned int* t_data) : num(t_num), type(GL_UNSIGNED_INT), data(t_data), mode(GL_TRIANGLES) {}
    selection_indexed(int t_num, const unsigned short* t_data) : num(t_num), type(GL_UNSIGNED_SHORT), data(t_data), mode(GL_TRIANGLES) {}
    selection_indexed(int t_num, const unsigned char* t_data) : num(t_num), type(GL_UNSIGNED_BYTE), data(t_data), mode(GL_TRIANGLES) {}
    selection_indexed(primitive_type t_mode, int t_num, const unsigned int* t_data) : num(t_num), type(GL_UNSIGNED_INT), data(t_data), mode(t_mode.internal()) {}
    selection_indexed(primitive_type t_mode, int t_num, const unsigned short* t_data) : num(t_num), type(GL_UNSIGNED_SHORT), data(t_data), mode(t_mode.internal()) {}
    selection_indexed(primitive_type t_mode, int t_num, const unsigned char* t_data) : num(t_num), type(GL_UNSIGNED_BYTE), data(t_data), mode(t_mode.internal()) {}
    
    void run_internal() const {
        glDrawElements(mode, num, type, data);
    }
};



class input_setup : public render_command {
    struct attrib_info {
        attribute targ;
        GLuint type;
        int dimension;
        bool normalize;
        int stride;
        const void* data;
        attrib_info() : targ(attribute::generate(0)) {}
    };
    vector<attrib_info> attrib_infos;
    
    void generic_bind_attribute(const attribute& targ, int dimension, int stride, const float* data) {
        attrib_info res;
        res.targ=targ;
        res.type=GL_FLOAT;
        res.dimension=dimension;
        res.normalize=0;
        res.stride=stride;
        res.data=data;
        attrib_infos.push_back(res);
    }
    
    const selection_setup* command;
    
    public:
    input_setup() : command(0) {}
    
    void bind(const selection_setup* targ) { command=targ; }
    void bind(selection_setup& targ) { bind(&targ); }
    
    void bind(const attribute& targ, const float* data, int stride=sizeof(float)) {
        generic_bind_attribute(targ, 1, stride, data);
    }
    template<int size> void bind(const attribute& targ, const vec_base<float, size>* data, int stride=sizeof(vec_base<float, size>)) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_attribute(targ, size, stride, data[0].data);
    }
    
    void run_internal() const {
        for (auto c=attrib_infos.begin();c!=attrib_infos.end();++c) {
            glEnableVertexAttribArray(c->targ.internal());
            glVertexAttribPointer(c->targ.internal(), c->dimension, c->type, (c->normalize)? GL_TRUE : GL_FALSE, c->stride, c->data);
        }
        if (command) command->run_internal();
        for (auto c=attrib_infos.begin();c!=attrib_infos.end();++c) {
            glDisableVertexAttribArray(c->targ.internal());
        }
    }
};



class command_list : public render_command {
    vector<const render_command*> targs;
    
    public:
    command_list() {}
    command_list(const vector<const render_command*>& t_targs) : targs(t_targs) {}
    command_list(vector<const render_command*>&& t_targs) : targs(move(t_targs)) {}
    
    void bind(const render_command* targ) {
        targs.push_back(targ);
    }
    void bind(render_command& targ) { bind(&targ); }
    
    void run_internal() const {
        for (int x=0;x<targs.size();++x) {
            if (targs[x]) targs[x]->run_internal();
        }
    }
};



class render {
    const clearing_setup* active_clearing_setup;
    const depth_setup* active_depth_setup;
    const color_setup* active_color_setup;
    const culling_setup* active_culling_setup;
    const program* active_program;
    const framebuffer* active_framebuffer;
    const render_command* generic_command;
    
    public:
    render() :
            active_clearing_setup(0),
            active_depth_setup(0),
            active_color_setup(0),
            active_culling_setup(0),
            active_program(0),
            active_framebuffer(0),
            generic_command(0)
    {}
    
    void bind(const clearing_setup* targ) { active_clearing_setup=targ; }
    void bind(const depth_setup* targ) { active_depth_setup=targ; }
    void bind(const color_setup* targ) { active_color_setup=targ; }
    void bind(const culling_setup* targ) { active_culling_setup=targ; }
    void bind(const program* targ) { active_program=targ; }
    void bind(const framebuffer* targ) { active_framebuffer=targ; }
    //
    void bind(clearing_setup& targ) { bind(&targ); }
    void bind(depth_setup& targ) { bind(&targ); }
    void bind(color_setup& targ) { bind(&targ); }
    void bind(culling_setup& targ) { bind(&targ); }
    void bind(program& targ) { bind(&targ); }
    void bind(framebuffer& targ) { bind(&targ); }
    
    void bind_generic(const render_command* targ) { generic_command=targ; }
    void bind_generic(render_command& targ) { bind_generic(&targ); }
    
    void run() {
        glDisable(GL_DITHER);
        glUseProgram(active_program? active_program->internal() : 0);
        glBindFramebuffer(GL_FRAMEBUFFER, active_framebuffer? active_framebuffer->internal() : 0);
        if (active_framebuffer) {
            glViewport(0, 0, active_framebuffer->width(), active_framebuffer->height());
        } else {
            std::cout << "Program won't work.\n";
        }
        if (active_program) {
            int current_texture_unit=0;
            const auto& sampler_map=active_program->internal_get_sampler_map();
            for (auto c=sampler_map.begin();c!=sampler_map.end();++c) {
                glUniform1i(c->first, current_texture_unit);
                glActiveTexture(GL_TEXTURE0 + current_texture_unit);
                if (c->second) {
                    c->second->internal_bind_for_render();
                } else {
                    c->second->internal_unbind_for_render();
                }
                ++current_texture_unit;
            }
        }
        if (active_depth_setup) active_depth_setup->run_internal(); else depth_setup::run_default();
        if (active_color_setup) active_color_setup->run_internal(); else color_setup::run_default();
        if (active_culling_setup) active_culling_setup->run_internal(); else culling_setup::run_default();
        if (active_clearing_setup) active_clearing_setup->run_internal(); else clearing_setup::run_default();
        if (generic_command) generic_command->run_internal();
    }
};

/*class render { //todo: break up into smaller classes
    struct attrib_info {
        attribute targ;
        GLuint type;
        int dimension;
        bool normalize;
        int stride;
        const void* data;
        attrib_info() : targ(attribute::generate(0)) {}
    };
    vector<attrib_info> attrib_infos;
    
    struct uniform_info {
        uniform targ;
        union {
            const texture* data_texture;
            const GLfloat* data_float;
            const GLint* data_int;
            const GLuint* data_uint;
        };
        char type; //0wwhhttp, w=width (1-4), h=height (1-4), t=type (int, float, texture, unsigned int), p=transpose
        int num;
        uniform_info() : targ(uniform::generate(0)) {}
    };
    vector<uniform_info> uniform_infos;
    
    bool enable_depth_test;
    GLuint depth_func;
    bool enable_alpha_test;
    GLuint alpha_func;
    float alpha_reference;
    int blend_mode; //0-none, 1-logical, 2-math
    GLuint blend_or_logic_equation;
    GLuint blend_func_source_color;
    GLuint blend_func_source_alpha;
    GLuint blend_func_dest_color;
    GLuint blend_func_dest_alpha;
    vec_base<float, 4> blending_color;
    
    const program* active_program;
    const framebuffer* active_framebuffer;
    
    GLuint clearing;
    vec_base<float, 4> clearing_color;
    double clearing_depth;
    
    void generic_bind_attribute(const attribute& targ, int dimension, int stride, const float* data) {
        attrib_info res;
        res.targ=targ;
        res.type=GL_FLOAT;
        res.dimension=dimension;
        res.normalize=0;
        res.stride=stride;
        res.data=data;
        attrib_infos.push_back(res);
    }
    
    static void assign_uniform_info_data(uniform_info& res, const float* data) {
        res.data_float=data;
    }
    static void assign_uniform_info_data(uniform_info& res, const int* data) {
        res.data_int=data;
    }
    static void assign_uniform_info_data(uniform_info& res, const unsigned int* data) {
        res.data_uint=data;
    }
    
    template<class data_type> void generic_bind_uniform(const uniform& targ, const data_type* data, int num, int type) {
        uniform_info res;
        res.targ=targ;
        assign_uniform_info_data(res, data);
        res.type=type;
        res.num=num;
        uniform_infos.push_back(res);
    }
    
    public:
    render() : active_program(0), active_framebuffer(0), clearing(0) {
        alpha_test(0);
        depth_test(1);
        clear_color(1);
        clear_depth(1);
        disable_blend();
    }
    
    void use(const program& targ) {
        active_program=&targ;
    }
    
    void use(framebuffer& targ) {
        active_framebuffer=&targ;
    }
    
    void clear_color(bool is_enabled, vec_base<float, 4> t_clearing_color=vec_base<float, 4>(0, 0, 0, 0)) {
        clearing=is_enabled? clearing|GL_COLOR_BUFFER_BIT : clearing&(~GL_COLOR_BUFFER_BIT);
        clearing_color=t_clearing_color;
    }
    
    void clear_depth(bool is_enabled, double t_clearing_depth=1) {
        clearing=is_enabled? clearing|GL_DEPTH_BUFFER_BIT : clearing&(~GL_DEPTH_BUFFER_BIT);
        clearing_depth=t_clearing_depth;
    }
    
    void depth_test(bool is_enabled, culling_function func=culling_function::incoming_less) {
        enable_depth_test=is_enabled;
        depth_func=func.internal();
    }
    
    void alpha_test(bool is_enabled, culling_function func=culling_function::always_true, float reference=0) {
        enable_alpha_test=is_enabled;
        alpha_func=func.internal();
        alpha_reference=reference;
    }
    
    void disable_blend() {
        blend_mode=0;
    }
    
    void blend(logic_op targ) {
        blend_mode=1;
        blend_or_logic_equation=targ.internal();
    }
    
    void blend(blend_equation targ, blend_function source, blend_function dest) {
        blend_mode=2;
        blend_or_logic_equation=targ.internal();
        blend_func_source_color=source.internal();
        blend_func_source_alpha=source.internal();
        blend_func_dest_color=dest.internal();
        blend_func_dest_alpha=dest.internal();
    }
    
    void blend(blend_equation targ, blend_function source_color, blend_function source_alpha, blend_function dest_color, blend_function dest_alpha) {
        blend_mode=2;
        blend_or_logic_equation=targ.internal();
        blend_func_source_color=source_color.internal();
        blend_func_source_alpha=source_alpha.internal();
        blend_func_dest_color=dest_color.internal();
        blend_func_dest_alpha=dest_alpha.internal();
    }
    
    void blend_color(vec_base<float, 4> t_color) {
        blending_color=t_color;
    }
    
    void bind(const attribute& targ, const float* data, int stride=sizeof(float)) {
        generic_bind_attribute(targ, 1, stride, data);
    }
    template<int size> void bind(const attribute& targ, const vec_base<float, size>* data, int stride=sizeof(vec_base<float, size>)) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_attribute(targ, size, stride, data[0].data);
    }
    
    template<int width, int height> void bind(const uniform& targ, const matrix_across_base<float, width, height>* data, int num) {
        math::assert_true<width>=1 && width<=4 && height>=1 && height<=4>;
        int t_width=width;
        int t_height=height;
        if (t_height==1) swap(t_width, t_height);
        generic_bind_uniform(targ, data[0].data, num, (t_width-1)<<5 | (t_height-1)<<3 | (1)<<1 | (1));
    }
    template<int width, int height> void bind(const uniform& targ, const matrix_down_base<float, width, height>* data, int num) {
        math::assert_true<width>=1 && width<=4 && height>=1 && height<=4>;
        int t_width=width;
        int t_height=height;
        if (t_height==1) swap(t_width, t_height);
        generic_bind_uniform(targ, data[0].data, num, (t_width-1)<<5 | (t_height-1)<<3 | (1)<<1 | (0));
    }
    template<int size> void bind(const uniform& targ, const vec_base<float, size>* data, int num) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_uniform(targ, data[0].data, num, (0)<<5 | (size-1)<<3 | (1)<<1 | (0));
    }
    template<int size> void bind(const uniform& targ, const vec_base<int, size>* data, int num) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_uniform(targ, data[0].data, num, (0)<<5 | (size-1)<<3 | (0)<<1 | (0));
    }
    template<int size> void bind(const uniform& targ, const vec_base<unsigned int, size>* data, int num) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_uniform(targ, data[0].data, num, (0)<<5 | (size-1)<<3 | (3)<<1 | (0));
    }
    template<int width, int height> void bind(const uniform& targ, matrix_across_base<float, width, height>& data) {
        bind(targ, &data, 1);
    }
    template<int width, int height> void bind(const uniform& targ, matrix_down_base<float, width, height>& data) {
        bind(targ, &data, 1);
    }
    template<int size> void bind(const uniform& targ, vec_base<float, size>& data) {
        bind(targ, &data, 1);
    }
    template<int size> void bind(const uniform& targ, vec_base<int, size>& data) {
        bind(targ, &data, 1);
    }
    template<int size> void bind(const uniform& targ, vec_base<unsigned int, size>& data) {
        bind(targ, &data, 1);
    }

    void bind(const uniform& targ, const float* data, int num) {
        generic_bind_uniform(targ, data, num, (0)<<5 | (0)<<3 | (1)<<1 | (0));
    }
    void bind(const uniform& targ, const int* data, int num) {
        generic_bind_uniform(targ, data, num, (0)<<5 | (0)<<3 | (0)<<1 | (0));
    }
    void bind(const uniform& targ, const unsigned int* data, int num) {
        generic_bind_uniform(targ, data, num, (0)<<5 | (0)<<3 | (3)<<1 | (0));
    }    
    void bind(const uniform& targ, float& data) {
        bind(targ, &data, 1);
    }
    void bind(const uniform& targ, int& data) {
        bind(targ, &data, 1);
    }
    void bind(const uniform& targ, unsigned int& data) {
        bind(targ, &data, 1);
    }
    
    void bind(const uniform& targ, const texture* data, int num) {
        uniform_info res;
        res.targ=targ;
        res.data_texture=data;
        res.type=(0)<<5 | (0)<<3 | (2)<<1 | (0);
        res.num=num;
        uniform_infos.push_back(res);
    }
    void bind(const uniform& targ, texture& data) {
        bind(targ, &data, 1);
    }
    
    void run(int num) {
        //cout << "a";
        if (blend_mode==0) {
            glDisable(GL_BLEND);
            glDisable(GL_COLOR_LOGIC_OP);
        } else
        if (blend_mode==1) {
            glDisable(GL_BLEND);
            glEnable(GL_COLOR_LOGIC_OP);
            glLogicOp(blend_or_logic_equation);
        } else
        if (blend_mode==2) {
            glDisable(GL_COLOR_LOGIC_OP);
            glEnable(GL_BLEND);
            glBlendFuncSeparate(blend_func_source_color, blend_func_source_alpha, blend_func_dest_color, blend_func_dest_alpha);
            glBlendEquation(blend_or_logic_equation);
            glBlendColor(blending_color.x, blending_color.y, blending_color.z, blending_color.w);
        }
        //cout << "b";
        if (clearing&GL_COLOR_BUFFER_BIT) glClearColor(clearing_color.x, clearing_color.y, clearing_color.z, clearing_color.w);
        if (clearing&GL_DEPTH_BUFFER_BIT) glClearDepth(clearing_depth);
        glClear(clearing);
        (enable_depth_test? glEnable : glDisable)(GL_DEPTH_TEST);
        if (enable_depth_test) glDepthFunc(depth_func);
        (enable_alpha_test? glEnable : glDisable)(GL_ALPHA_TEST);
        if (enable_alpha_test) glAlphaFunc(alpha_func, alpha_reference);
        glUseProgram(active_program->internal());
        glBindFramebuffer(GL_FRAMEBUFFER, active_framebuffer? active_framebuffer->internal() : 0);
        //cout << "[" << glCheckFramebufferStatus(active_framebuffer->internal()) << "]";
        //cout << "c";
        glViewport(0, 0, active_framebuffer->width(), active_framebuffer->height());
        for (auto c=attrib_infos.begin();c!=attrib_infos.end();++c) {
            glEnableVertexAttribArray(c->targ.internal());
            glVertexAttribPointer(c->targ.internal(), c->dimension, c->type, (c->normalize)? GL_TRUE : GL_FALSE, c->stride, c->data);
        }
        //cout << "d";
        int current_texture_unit=0;
        for (auto c=uniform_infos.begin();c!=uniform_infos.end();++c) {
            int t=(c->type&0x06)>>1;
            int w=((c->type&0x60)>>5)+1;
            int h=((c->type&0x18)>>3)+1;
            //cout << h << "\n";
            int p=(c->type&0x01);
            if (t==0) { //int
                //cout << "a";
                decltype(glUniform1iv) func;
                if (h==1) func=glUniform1iv;
                if (h==2) func=glUniform2iv;
                if (h==3) func=glUniform3iv;
                if (h==4) func=glUniform4iv;
                func(c->targ.internal(), c->num, c->data_int);
                //cout << "b";
            } else
            if (t==1) { //float
                //cout << "c";
                if (w==1) {
                    decltype(glUniform1fv) func;
                    if (h==1) func=glUniform1fv;
                    if (h==2) func=glUniform2fv;
                    if (h==3) func=glUniform3fv;
                    if (h==4) func=glUniform4fv;
                    func(c->targ.internal(), c->num, c->data_float);
                } else {
                    decltype(glUniformMatrix2fv) func;
                    if (w==2) {
                        if (h==2) func=glUniformMatrix2fv;
                        if (h==3) func=glUniformMatrix2x3fv;
                        if (h==4) func=glUniformMatrix2x4fv;
                    }
                    if (w==3) {
                        if (h==2) func=glUniformMatrix3x2fv;
                        if (h==3) func=glUniformMatrix3fv;
                        if (h==4) func=glUniformMatrix3x4fv;
                    }
                    if (w==4) {
                        if (h==2) func=glUniformMatrix4x2fv;
                        if (h==3) func=glUniformMatrix4x3fv;
                        if (h==4) func=glUniformMatrix4fv;
                    }
                    func(c->targ.internal(), c->num, p, c->data_float);
                }
                //cout << "b";
            } else
            if (t==2) { //texture
                //cout << "d";
                for (int x=0;x<c->num;++x) {
                    //cout << "tex bind\n";
                    glUniform1i(c->targ.internal(), current_texture_unit);
                    glActiveTexture(GL_TEXTURE0 + current_texture_unit);
                    glBindTexture(GL_TEXTURE_2D, c->data_texture[x].internal());
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    ++current_texture_unit;
                }
                //cout << "b";
            } else
            if (t==3) { //unsigned int
                decltype(glUniform1uiv) func;
                if (h==1) func=glUniform1uiv;
                if (h==2) func=glUniform2uiv;
                if (h==3) func=glUniform3uiv;
                if (h==4) func=glUniform4uiv;
                //cout << "[[" << h << ", " << ((void*)func) << ", " << c->targ.internal() << ", " << c->num << ", " << c->data_uint << "]]";
                func(c->targ.internal(), c->num, c->data_uint);
                //cout << "b";
            }
        }
        //cout << "f";
        //cout << "[" << glGetError() << "]\n";
        glDrawArrays(GL_TRIANGLES, 0, num);
        //cout << "[" << glGetError() << "]\n";
        //cout << "g";
        for (auto c=attrib_infos.begin();c!=attrib_infos.end();++c) {
            glDisableVertexAttribArray(c->targ.internal());
        }
    }
};*/

}

#endif
