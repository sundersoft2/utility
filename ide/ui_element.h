set<int> pressed_keys;
bool control_pressed;
bool shift_pressed;
bool alt_pressed;
bool left_mouse_button_pressed;
ivec2 mouse_position;

struct ui_element : disable_copy {
    ivec2 origin;
    ivec2 size_pixels;
    bool selected;
    bool redraw;
    
    bool contains(ivec2 p) const {
        return rectangle_contains(origin, size_pixels, p);
    }
    
    void set_redraw() {
        redraw=true;
    }
    
    virtual void input_key(bool is_down, int key) {}
    virtual void input_text(char c) {}
    virtual void input_mouse_button(bool is_down, bool double_click, int button) {}
    virtual void input_mouse_move() {}
    virtual void input_mouse_wheel(int dy) {}
    virtual void render() {}
    
    virtual ~ui_element() {}
};

struct ui_elements {
    vector<ui_element*> elements;
    
    void add(ui_element& t) {
        elements.push_back(&t);
    }
    
    ui_element* locate(ivec2 p) {
        for (auto c=elements.begin();c!=elements.end();++c) {
            if ((*c)->contains(p)) {
                return &**c;
            }
        }
        return nullptr;
    }
    
    void select_at(ivec2 p) {
        auto c=locate(p);
        if (c!=nullptr) {
            select(*c);
        }
    }
    
    bool needs_render() {
        for (auto c=elements.begin();c!=elements.end();++c) {
            if ((*c)->redraw) {
                return true;
            }
        }
        
        return false;
    }
    
    template<class func> void iterate(func f) {
        for (auto c=elements.begin();c!=elements.end();++c) {
            f(**c);
        }
    }
    
    void select(ui_element& t) {
        iterate([&](ui_element& c){ c.selected=(&c==&t); });
    }
    
    void set_redraw() {
        iterate([&](ui_element& c){ c.redraw=true; });
    }
    
    void input_key(bool is_down, int key)  {
        iterate([&](ui_element& c) { if (c.selected) c.input_key(is_down, key); });
    }
    
    void input_text(char ch) {
        iterate([&](ui_element& c) { if (c.selected) c.input_text(ch); });
    }
    
    void input_mouse_button(bool is_down, bool double_click, int button) {
        iterate([&](ui_element& c) { if (c.selected) c.input_mouse_button(is_down, double_click, button); });
    }
    
    void input_mouse_move() {
        iterate([&](ui_element& c) { if (c.selected) c.input_mouse_move(); });
    }
    
    void input_mouse_wheel(int dy) {
        auto hover=locate(mouse_position);
        if (hover!=nullptr) {
            hover->input_mouse_wheel(dy);
        } else {
            iterate([&](ui_element& c) { if (c.selected) c.input_mouse_wheel(dy); });
        }
    }
    
    void render() {
        iterate([&](ui_element& c) {
            c.render();
            c.redraw=false;
        });
    }
};
