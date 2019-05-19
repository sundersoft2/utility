void do_sdl(int res) {
    assert(res==0);
}

image_rgba read_bmp(string path) {
    ifstream in(path.c_str(), ios::binary);
    return read_bmp(in);
}

image_rgba postprocess_font(image_rgba&& font) {
    for (int y=0;y<font.height();++y) {
        for (int x=0;x<font.width();++x) {
            rgba& c=font[y][x];
            if (c.r==0) {
                c=rgba(0, 0, 0, 0);
            } else {
                c=rgba(255, 255, 255, 255);
            }
        }
    }
    return move(font);
}

class texture;

static safe_list<texture*> all_textures;
static SDL_Renderer* sdl_renderer;

class texture : disable_copy {
    image_rgba cpu;
    SDL_Texture* gpu;
    safe_list<texture*>::iterator iter;
    
    void free_gpu() {
        assert(gpu!=nullptr);
        SDL_DestroyTexture(gpu);
        gpu=nullptr;
    }
    
    void sync_gpu() {
        assert(gpu==nullptr);
        gpu=SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, cpu.width(), cpu.height());
        assert(gpu!=nullptr);
        do_sdl(SDL_UpdateTexture(gpu, nullptr, &(cpu[0][0]), cpu.width()*sizeof(rgba)));
    }
    
    void refresh() {
        free_gpu();
        sync_gpu();
    }
    
    public:
    texture(image_rgba&& t_cpu) {
        cpu=move(t_cpu);
        gpu=nullptr;
        all_textures.push_front(this);
        iter=all_textures.begin();
        sync_gpu();
    }
    
    static void refresh_all() {
        for (auto c=all_textures.begin();c!=all_textures.end();++c) {
            (*c)->refresh();
        }
    }
    
    const image_rgba* operator->() const {
        return &cpu;
    }
    
    const image_rgba& operator*() const {
        return cpu;
    }
    
    ~texture() {
        all_textures.erase(iter);
        free_gpu();
    }
    
    void render(ivec2 write_start, ivec2 read_start, ivec2 size, rgba color) const {
        SDL_Rect read;
        read.x=read_start.x;
        read.y=read_start.y;
        read.w=size.x;
        read.h=size.y;
        
        SDL_Rect write;
        write.x=write_start.x;
        write.y=write_start.y;
        write.w=size.x;
        write.h=size.y;
        
        do_sdl(SDL_SetTextureBlendMode(gpu, SDL_BLENDMODE_BLEND));
        do_sdl(SDL_SetTextureColorMod(gpu, color.r, color.g, color.b));
        do_sdl(SDL_SetTextureAlphaMod(gpu, color.a));
        
        do_sdl(SDL_RenderCopy(sdl_renderer, gpu, &read, &write));
    }
};

void render_solid(ivec2 write_start, ivec2 size, rgba color) {
    SDL_Rect write;
    write.x=write_start.x;
    write.y=write_start.y;
    write.w=size.x;
    write.h=size.y;
    
    do_sdl(SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND));
    do_sdl(SDL_SetRenderDrawColor(sdl_renderer, color.r, color.g, color.b, color.a));
    
    do_sdl(SDL_RenderFillRect(sdl_renderer, &write));
}

void render_clear(rgba color) {
    do_sdl(SDL_SetRenderDrawColor(sdl_renderer, color.r, color.g, color.b, color.a));
    
    do_sdl(SDL_RenderClear(sdl_renderer));
}

struct char_format {
    bool bold;
    bool italic;
    
    rgba fg_color;
    rgba bg_color;
    
    char_format() {}
    char_format(bool t_bold, bool t_italic, rgba t_fg_color, rgba t_bg_color=rgba(0, 0, 0)) {
        bold=t_bold;
        italic=t_italic;
        fg_color=t_fg_color;
        bg_color=t_bg_color;
    }
};

ivec2 font_character_size(const image_rgba& font) {
    assert(font.width()%(94*3)==0);
    assert(font.height()%4==0);
    return ivec2(font.width()/(94*3), font.height()/4);
}

bool font_image_start(ivec2 char_size, char c, char_format f, ivec2& out_image_start) {
    if (!is_ascii_printable(c)) {
        return false;
    }
    
    int y_mask=(f.bold? 1 : 0) | (f.italic? 2 : 0);
    out_image_start=ivec2((c-33)*(char_size.x*3), y_mask*char_size.y);
    
    return true;
}

bool rectangle_contains(ivec2 origin, ivec2 size, ivec2 p) {
    return
        p.x>=origin.x && p.x<origin.x+size.x &&
        p.y>=origin.y && p.y<origin.y+size.y
    ;
}

ivec2 scale(ivec2 a, ivec2 b) {
    return ivec2(a.x*b.x, a.y*b.y);
}

//font must be fixed-length. image should be of the ascii chars 33 to 126 (total of 94 chars)
//each ascii char should have a space on each side; adjacent chars should have two spaces between them
//image has 4 rows. first row is plain, second bold, third italic, fourth bold and italic
void render_text(
    const texture& font,
    ivec2 origin,
    ivec2 offset,
    ivec2 size,
    function<pair<char, char_format>(ivec2 p)> chars,
    ivec2 cursor_pos=ivec2(-1, -1), //cursor rendered before this char
    rgba cursor_color=rgba(255, 255, 255)
) {
    ivec2 char_size=font_character_size(*font);
    
    //background pass
    for (int x=offset.x;x<offset.x+size.x;++x) {
        for (int y=offset.y;y<offset.y+size.y;++y) {
            ivec2 p(x, y);
            auto c=chars(p);
            render_solid(origin+scale(p-offset, char_size), char_size, c.second.bg_color);
        }
    }
    
    //foreground pass (needs to support overlapping characters)
    for (int x=offset.x;x<offset.x+size.x;++x) {
        for (int y=offset.y;y<offset.y+size.y;++y) {
            ivec2 p(x, y);
            auto c=chars(p);
            ivec2 image_start;
            if (font_image_start(char_size, c.first, c.second, image_start)) {
                font.render(origin+scale(ivec2(x-1, y)-offset, char_size), image_start, ivec2(char_size.x*3, char_size.y), c.second.fg_color);
            }
        }
    }
    
    if (cursor_pos.x!=-1 && rectangle_contains(offset, size, cursor_pos)) {
        render_solid(origin+scale(cursor_pos-offset, char_size)+ivec2(-1, 1), ivec2(2, char_size.y-2), cursor_color);
    }
}

void init_renderer(SDL_Window* sdl_window) {
    if (sdl_renderer!=nullptr) {
        SDL_DestroyRenderer(sdl_renderer);
    }
    
    sdl_renderer=SDL_CreateRenderer(sdl_window, -1,
        //0
        SDL_RENDERER_SOFTWARE
    );
    assert(sdl_renderer!=nullptr);
    
    texture::refresh_all();
}



/*rgba blend_opaque(rgba new_value, rgba old_value) {
    return new_value;
}

rgba blend_alpha(rgba new_value, rgba old_value) {
    if (new_value.a!=0) {
        return new_value;
    } else {
        return old_value;
    }
}

rgba blend_invert(rgba new_value, rgba old_value) {
    return rgba(255-old_value.r, 255-old_value.g, 255-old_value.b, old_value.a);
}

function<rgba(rgba, rgba)> blend_foreground(rgba color) {
    return [=](rgba new_value, rgba old_value) -> rgba {
        if (new_value.r!=0) {
            return color;
        } else {
            return old_value;
        }
    };
}

void render_rectangle(
    image_rgba& target,
    ivec2 origin,
    ivec2 size,
    function<rgba(ivec2 p)> data,
    function<rgba(rgba new_value, rgba old_value)> blend
) {
    origin.x=min(origin.x, target.width());
    origin.y=min(origin.y, target.height());
    
    ivec2 start(max(0, -origin.x), max(0, -origin.y));
    
    size.x=min(target.width(), origin.x+size.x)-origin.x;
    size.y=min(target.height(), origin.y+size.y)-origin.y;
    
    for (int y=start.y;y<size.y;++y) {
        auto y_pixels=target[y+origin.y];
        
        for (int x=start.x;x<size.x;++x) {
            rgba& pixel=y_pixels[x+origin.x];
            rgba old_value=pixel;
            
            rgba new_value=data(ivec2(x, y));
            rgba blended_value=blend(new_value, old_value);
            pixel=blended_value;
        }
    }
}

void render_image(
    image_rgba& target,
    ivec2 origin,
    const image_rgba& image,
    ivec2 start=ivec2(0, 0),
    ivec2 size=ivec2(-1, -1),
    function<rgba(rgba new_value, rgba old_value)> blend=blend_alpha
) {
    if (size.x==-1) {
        size.x=image.width();
    }
    
    if (size.y==-1) {
        size.y=image.height();
    }
    
    assert(start.x>=0);
    assert(start.y>=0);
    assert(size.x>=0);
    assert(size.y>=0);
    assert(start.x+size.x<=image.width());
    assert(start.y+size.y<=image.height());
    
    render_rectangle(target, origin, size, [&](ivec2 p) {
        return image[p.y+start.y][p.x+start.x];
    }, blend);
}

void render_solid(
    image_rgba& target,
    ivec2 origin,
    ivec2 size,
    rgba color,
    function<rgba(rgba new_value, rgba old_value)> blend=blend_opaque
) {
    render_rectangle(target, origin, size, [&](ivec2 p) {
        return color;
    }, blend);
}*/
