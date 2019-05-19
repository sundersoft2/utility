#ifndef ILYA_HEADER_OGL_STATE
#define ILYA_HEADER_OGL_STATE

#include <gl/gl.h>
#include <gl/glext.h>
#include <glex/GLEX.c>

#include <string>
#include <cctype>
#include <vector>
#include <memory>
#include <map>

#include <shared/generic.h>
#include <shared/graphics.h>
#include <shared/math_linear.h>

#include "ogl_enumeration.h"

namespace ogl {

using std::string;
using std::isspace;
using std::move;
using std::swap;
using std::map;
using std::vector;
using std::pair;
using std::make_pair;
using graphics::image_base;
using graphics::rgba;
using math::matrix_across_base;
using math::matrix_down_base;
using math::vec_base;

bool ogl_should_swap_rgba_rows=true;

void swap_image_rgba_rows(image_base<rgba>& targ) {
    if (!ogl_should_swap_rgba_rows) {
        return;
    }
    
    if (targ.begin()!=targ.end()) {
        auto c=targ.begin();
        auto d=targ.end()-1;
        for (;c!=d && d+1!=c;++c, --d) {
            auto e=c.begin();
            auto f=d.begin();
            for (;e!=c.end();++e, ++f) {
                swap(*e, *f);
            }
        }
    }
}



class renderbuffer {
    GLuint handle;
    int d_width;
    int d_height;
    renderbuffer(const renderbuffer&) { assert(false); }
    void operator=(const renderbuffer&) { assert(false); }
    
    public:
    renderbuffer(renderbuffer&& targ) : handle(move(targ.handle)), d_width(move(targ.d_width)), d_height(move(targ.d_height)) {
        targ.handle=0;
    }
    
    renderbuffer& operator=(renderbuffer&& targ) {
        swap(handle, targ.handle);
        swap(d_width, targ.d_width);
        swap(d_height, targ.d_height);
        return *this;
    }
    
    GLuint internal() const { return handle; }
    int width() const { return d_width; }
    int height() const { return d_height; }
    
    renderbuffer(image_format t_format, int t_width, int t_height) : d_width(t_width), d_height(t_height) {
        glGenRenderbuffers(1, &handle);
        glBindRenderbuffer(GL_RENDERBUFFER, handle);
        glRenderbufferStorage(GL_RENDERBUFFER, t_format.internal(), t_width, t_height);
    }
    
    ~renderbuffer() {
        glDeleteRenderbuffers(1, &handle);
    }
};



class texture {
    GLuint handle;
    int d_width;
    int d_height;
    image_format d_format;
    bool d_has_mipmaps;
    int d_num_levels;
    GLenum d_mag_filter;
    GLenum d_min_filter;
    GLenum d_wrap_u;
    GLenum d_wrap_v;
    texture(const texture&) : d_format(image_format::rgba8) { assert(false); }
    void operator=(const texture&) { assert(false); }
    
    pair<int, int> mipmap_size(int level) const {
        int x=d_width>>level;
        if (x==0) x=1;
        int y=d_height>>level;
        if (y==0) y=1;
        return make_pair(x, y);
    }
    
    public:
    texture() : handle(0), d_format(image_format::rgba8) {}
    texture(texture&& targ) : handle(0), d_format(image_format::rgba8) { *this=move(targ); }
    
    texture(image_format t_format, int t_width, int t_height) :
        d_width(t_width),
        d_height(t_height),
        d_format(t_format),
        d_has_mipmaps(0),
        d_num_levels(1),
        d_min_filter(GL_NEAREST),
        d_mag_filter(GL_NEAREST),
        d_wrap_u(GL_REPEAT),
        d_wrap_v(GL_REPEAT)
    {
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexImage2D(GL_TEXTURE_2D, 0, t_format.internal(), t_width, t_height, 0, t_format.is_depth()? GL_DEPTH_COMPONENT : GL_RED, GL_BYTE, 0);
    }
    
    texture& operator=(texture&& targ) {
        swap(handle, targ.handle);
        swap(d_width, targ.d_width);
        swap(d_height, targ.d_height);
        swap(d_has_mipmaps, targ.d_has_mipmaps);
        swap(d_num_levels, targ.d_num_levels);
        swap(d_format, targ.d_format);
        swap(d_mag_filter, targ.d_mag_filter);
        swap(d_min_filter, targ.d_min_filter);
        swap(d_wrap_u, targ.d_wrap_u);
        swap(d_wrap_v, targ.d_wrap_v);
        return *this;
    }
    
    GLuint internal() const { return handle; }
    
    void internal_bind_for_render() const {
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, d_min_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, d_mag_filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, d_wrap_u);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, d_wrap_v);
    }
    
    static void internal_unbind_for_render() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    image_format format() const { return d_format; }
    
    void configure(mag_filter t_mag) { d_mag_filter=t_mag.internal(); }
    void configure(min_filter t_min) { d_min_filter=t_min.internal(); }
    void configure_u(wrap_mode t_wrap) { d_wrap_u=t_wrap.internal(); }
    void configure_v(wrap_mode t_wrap) { d_wrap_v=t_wrap.internal(); }
    void configure(wrap_mode t_wrap) { configure_u(t_wrap); configure_v(t_wrap); }
    
    int width() const { return d_width; }
    int height() const { return d_height; }
    vec_base<int, 2> size() const { return vec_base<int, 2>(d_width, d_height); }
    
    int width(int level) const { return mipmap_size(level).first; }
    int height(int level) const { return mipmap_size(level).second; }
    vec_base<int, 2> size(int level) const {
        auto res=mipmap_size(level);
        return vec_base<int, 2>(res.first, res.second);
    }
    int num_levels() const { return d_num_levels; }
    bool has_mipmaps() const { return d_has_mipmaps; }
    
    bool exists() const { return handle; }
    
    void generate_empty_mipmaps() {
        if (!d_has_mipmaps) {
            glBindTexture(GL_TEXTURE_2D, handle);
            int c_width=d_width;
            int c_height=d_height;
            d_num_levels=0;
            while (c_width>1 || c_height>1) {
                c_width>>=1;
                c_height>>=1;
                if (c_width==0) c_width=1;
                if (c_height==0) c_height=1;
                ++d_num_levels;
                glTexImage2D(GL_TEXTURE_2D, d_num_levels, d_format.internal(), c_width, c_height, 0, d_format.is_depth()? GL_DEPTH_COMPONENT : GL_RED, GL_BYTE, 0);
            }
            ++d_num_levels;
            d_has_mipmaps=1;
        }
    }
    
    struct read_error {};
    vec_base<float, 4> read_last_mipmap() const {
        if (!d_has_mipmaps) throw read_error();
        vec_base<float, 4> res;
        glBindTexture(GL_TEXTURE_2D, handle);
        glGetTexImage(GL_TEXTURE_2D, d_num_levels-1, GL_RGBA, GL_FLOAT, &res);
        return res;
    }
    
    image_base<rgba> download_rgba8(int level=0) const {
        auto t_size=mipmap_size(level);
        image_base<rgba> res(t_size.first, t_size.second, 0);
        glBindTexture(GL_TEXTURE_2D, handle);
        glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_UNSIGNED_BYTE, &*(res.pbegin()));
        if (ogl_should_swap_rgba_rows) {
            swap_image_rgba_rows(res);
        }
        return res;
    }
    
    struct upload_error {};
    
    private:
    void upload_rgba8_impl(const image_base<rgba>& data, int level=0, bool generate_mipmaps=false) const {
        auto t_size=mipmap_size(level);
        if (t_size.first!=data.width() || t_size.second!=data.height()) throw upload_error();
        glBindTexture(GL_TEXTURE_2D, handle);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, d_width, d_height, GL_RGBA, GL_UNSIGNED_BYTE, &*(data.pbegin()));
        
        if (generate_mipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
    
    public:
    void upload_rgba8(image_base<rgba>&& data, int level=0, bool generate_mipmaps=false) const {
        if (ogl_should_swap_rgba_rows) {
            swap_image_rgba_rows(data);
        }
        upload_rgba8_impl(data, level, generate_mipmaps);
    }
    void upload_rgba8(const image_base<rgba>& data, int level=0, bool generate_mipmaps=false) const {
        assert(!ogl_should_swap_rgba_rows);
        upload_rgba8_impl(data, level, generate_mipmaps);
    }
    
    ~texture() {
        if (handle) glDeleteTextures(1, &handle);
    }
};



class framebuffer {
    static int screen_width;
    static int screen_height;
    int d_width;
    int d_height;
    int mode;
    static const int generic_not_bound=0;
    static const int generic_bound=1;
    static const int device_buffer=2;
    GLuint handle;
    framebuffer(const framebuffer&) {}
    void operator=(const framebuffer&) {}
    
    framebuffer(int) { assert(false); }
    
    void generic_bind(int t_width, int t_height) {
        if (mode==device_buffer) {
            throw cant_bind_to_device();
        } else
        if (mode==generic_bound) {
            if (d_width!=t_width || d_height!=t_height) {
                throw size_mismatch();
            }
        } else {
            d_width=t_width;
            d_height=t_height;
            mode=generic_bound;
        }
    }
    
    public:
    static void set_screen_dimensions(int width, int height) {
        screen_width=width;
        screen_height=height;
    }
    
    framebuffer(framebuffer&& targ) : handle(move(targ.handle)), d_width(move(targ.d_width)), d_height(move(targ.d_height)), mode(move(targ.mode)) {
        targ.handle=0;
    }
    
    framebuffer& operator=(framebuffer&& targ) {
        swap(handle, targ.handle);
        swap(d_width, targ.d_width);
        swap(d_height, targ.d_height);
        swap(mode, targ.mode);
        return *this;
    }
    
    GLuint internal() const { return handle; }
    
    framebuffer() : mode(generic_not_bound) {
        glGenFramebuffers(1, &handle);
    }
    
    struct size_mismatch {};
    struct cant_bind_to_device {};
    void bind(framebuffer_attachment attachment, renderbuffer& t_renderbuffer) {
        generic_bind(t_renderbuffer.width(), t_renderbuffer.height());
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment.internal(), GL_RENDERBUFFER, t_renderbuffer.internal());
    }
    void bind(framebuffer_attachment attachment, texture& t_texture, int level=0) {
        generic_bind(t_texture.width(), t_texture.height());
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment.internal(), GL_TEXTURE_2D, t_texture.internal(), level);
    }
    void unbind(framebuffer_attachment attachment) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment.internal(), GL_RENDERBUFFER, 0);
    }
    
    void reset() { //should only be called if nothing is attached
        mode=generic_not_bound;
    }
    
    int width() const { return d_width; }
    int height() const { return d_height; }
    
    struct read_error {};
    image_base<rgba> read_rgba8(framebuffer_attachment attachment) const {
        image_base<rgba> res(d_width, d_height, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
        if (attachment.internal()==GL_DEPTH_ATTACHMENT) {
            throw read_error();
        } else {
            glReadBuffer(attachment.internal());
            glReadPixels(0, 0, d_width, d_height, GL_RGBA, GL_UNSIGNED_BYTE, &*(res.pbegin()));
        }
        if (ogl_should_swap_rgba_rows) {
            swap_image_rgba_rows(res);
        }
        return res;
    }
    
    ~framebuffer() {
        if (mode!=device_buffer) {
            glDeleteFramebuffers(1, &handle);
        }
    }
    
    static framebuffer device() {
        if (screen_width==-1 || screen_height==-1) {
            std::cout << "The easter bunny is NOT pleased!\n";
        }
        framebuffer res(0);
        res.handle=0;
        res.mode=device_buffer;
        res.d_width=screen_width;
        res.d_height=screen_height;
        return res;
    }
};
int framebuffer::screen_width=-1;
int framebuffer::screen_height=-1;



class shader {
    GLuint handle;
    shader(const shader&) { assert(false); }
    void operator=(const shader&) { assert(false); }
    
    public:
    shader(shader&& targ) : handle(move(targ.handle)) {
        targ.handle=0;
    }
    
    shader& operator=(shader&& targ) {
        swap(handle, targ.handle);
        return *this;
    }
    
    GLuint internal() const { return handle; }
    
    struct errors {
        string data;
    };
    
    shader(shader_type type, const string& source) {
        handle=glCreateShader(type.internal());
        const char* source_str=source.c_str();
        int source_size=source.size();
        glShaderSource(handle, 1, &source_str, &source_size);
        glCompileShader(handle);
        GLint compile_status;
        glGetShaderiv(handle, GL_COMPILE_STATUS, &compile_status);
        if (compile_status==GL_FALSE) {
            errors res;
            int log_size;
            glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_size);
            res.data.resize(log_size);
            glGetShaderInfoLog(handle, log_size, 0, &(res.data[0]));
            while (!res.data.empty() && isspace(res.data[res.data.size()-1])) {
                res.data.resize(res.data.size()-1);
            }
            glDeleteShader(handle);
            throw res;
        }
    }
    
    ~shader() {
        glDeleteShader(handle);
    }
};



class attribute {
    attribute() {}
    GLuint handle;
    
    public:
    static attribute generate(GLuint targ) {
        attribute res;
        res.handle=targ;
        return res;
    }
    
    GLuint internal() const { return handle; }
};



class program;
class uniform {
    uniform() {}
    GLint handle;
    program* parent;
    void use_parent() const;
    
    /*template<class data_type> void generic_bind_uniform(const uniform& targ, const data_type* data, int num, int type) {
        uniform_info res;
        res.targ=targ;
        assign_uniform_info_data(res, data);
        res.type=type;
        res.num=num;
        uniform_infos.push_back(res);
        //
        glUseProgram(parent->internal);
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
    }*/
    
    void generic_bind_uniform(const float* data, int num, int w, int h, bool p) {
        use_parent();
        if (w==1) {
            decltype(glUniform1fv) func;
            if (h==1) func=glUniform1fv;
            if (h==2) func=glUniform2fv;
            if (h==3) func=glUniform3fv;
            if (h==4) func=glUniform4fv;
            func(handle, num, data);
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
            func(handle, num, p? GL_TRUE : GL_FALSE, data);
        }
    }
    
    void generic_bind_uniform(const int* data, int num, int h) {
        use_parent();
        decltype(glUniform1iv) func;
        if (h==1) func=glUniform1iv;
        if (h==2) func=glUniform2iv;
        if (h==3) func=glUniform3iv;
        if (h==4) func=glUniform4iv;
        func(handle, num, data);
    }
    
    void generic_bind_uniform(const unsigned int* data, int num, int h) {
        use_parent();
        decltype(glUniform1uiv) func;
        if (h==1) func=glUniform1uiv;
        if (h==2) func=glUniform2uiv;
        if (h==3) func=glUniform3uiv;
        if (h==4) func=glUniform4uiv;
        func(handle, num, data);
    }
    
    public:
    static uniform generate(GLint t_handle, program* t_parent) {
        uniform res;
        res.handle=t_handle;
        res.parent=t_parent;
        return res;
    }
    
    GLuint internal() const { return handle; }
    program* internal_parent() const { return parent; }
    
    template<int width, int height> void bind(const matrix_across_base<float, width, height>* data, int num) {
        math::assert_true<width>=1 && width<=4 && height>=1 && height<=4>;
        int t_width=width;
        int t_height=height;
        if (t_height==1) swap(t_width, t_height);
        generic_bind_uniform(data[0].data, num, width, height, 1);
    }
    template<int width, int height> void bind(const matrix_down_base<float, width, height>* data, int num) {
        math::assert_true<width>=1 && width<=4 && height>=1 && height<=4>;
        int t_width=width;
        int t_height=height;
        if (t_height==1) swap(t_width, t_height);
        generic_bind_uniform(data[0].data, num, width, height, 0);
    }
    template<int size> void bind(const vec_base<float, size>* data, int num) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_uniform(data[0].data, num, 1, size, 0);
    }
    template<int size> void bind(const vec_base<int, size>* data, int num) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_uniform(data[0].data, num, size);
    }
    template<int size> void bind(const vec_base<unsigned int, size>* data, int num) {
        math::assert_true<size>=1 && size<=4>;
        generic_bind_uniform(data[0].data, num, size);
    }
    template<int width, int height> void bind(const matrix_across_base<float, width, height>& data) {
        bind(&data, 1);
    }
    template<int width, int height> void bind(const matrix_down_base<float, width, height>& data) {
        bind(&data, 1);
    }
    template<int size> void bind(const vec_base<float, size>& data) {
        bind(&data, 1);
    }
    template<int size> void bind(const vec_base<int, size>& data) {
        bind(&data, 1);
    }
    template<int size> void bind(const vec_base<unsigned int, size>& data) {
        bind(&data, 1);
    }

    void bind(const float* data, int num) {
        generic_bind_uniform(data, num, 1, 1, 0);
    }
    void bind(const int* data, int num) {
        generic_bind_uniform(data, num, 1);
    }
    void bind(const unsigned int* data, int num) {
        generic_bind_uniform(data, num, 1);
    }    
    void bind(const float& data) {
        bind(&data, 1);
    }
    void bind(const int& data) {
        bind(&data, 1);
    }
    void bind(const unsigned int& data) {
        bind(&data, 1);
    }
    
    void bind(const texture* data);
    void bind(const texture& data) {
        bind(&data);
    }
    
    void unbind_texture() {
        bind((texture*)0);
    }
};



class program {
    map<GLuint, const texture*> sampler_map;
    GLuint handle;
    program(const program&) { assert(false); }
    void operator=(const program&) { assert(false); }
    
    public:
    program() {
        handle=glCreateProgram();
    }
    program(program&& targ) : handle(move(targ.handle)) {
        targ.handle=0;
    }
    
    program& operator=(program&& targ) {
        swap(handle, targ.handle);
        return *this;
    }
    
    GLuint internal() const { return handle; }
    const decltype(sampler_map)& internal_get_sampler_map() const { return sampler_map; }
    decltype(sampler_map)& internal_get_sampler_map() { return sampler_map; }
    
    struct errors {
        string data;
    };
    
    void attach(const shader& targ) {
        glAttachShader(handle, targ.internal());
    }
    
    void link() {
        glLinkProgram(handle);
        GLint linked=0;
        glGetProgramiv(handle, GL_LINK_STATUS, &linked);
        if (linked==GL_FALSE) {
            errors res;
            int log_size;
            glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_size);
            res.data.resize(log_size);
            glGetProgramInfoLog(handle, log_size, 0, &(res.data[0]));
            while (!res.data.empty() && isspace(res.data[res.data.size()-1])) {
                res.data.resize(res.data.size()-1);
            }
            throw res;
        }
    }
    
    attribute locate_attribute(string name) const {
        GLint res=glGetAttribLocation(handle, name.c_str());
        return attribute::generate(res);
    }
    
    uniform locate_uniform(string name) {
        GLint res=glGetUniformLocation(handle, name.c_str());
        return uniform::generate(res, this);
    }
    
    ~program() {
        glDeleteProgram(handle);
    }
};
void uniform::bind(const texture* data) {
    parent->internal_get_sampler_map()[handle]=data;
}
void uniform::use_parent() const {
    glUseProgram(parent->internal());
}

}

#endif
