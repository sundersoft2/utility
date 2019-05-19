const int scroll_page_common_lines=2;
const int scroll_wheel_scale=3;
const int control_cursor_move_scale=5;

struct syntax_highlighting {
    map<s8, char_format> token_colors; //bg color only used if it replaces bg_color_body
    
    rgba fg_color_selected;
    rgba bg_color_selected;
    rgba bg_color_indentation_1;
    rgba bg_color_indentation_2; //will alternate between 1 and 2
    rgba bg_color_trailing_whitespace;
    rgba bg_color_body_current_line; //code that is after indentation and before the end of the line
    rgba bg_color_trailing_whitespace_current_line;
    
    rgba bg_color_highlighted;
    rgba bg_color_highlighted_selected;
    
    set<s8> age_tokens; //foreground color replaced with age color
    double age_multiplier=1; //applied to log of age before color lookup
    vector<rgba> age_colors;
    
    char_format get_colors(
        rgba* out_cursor_color=nullptr,
        s8 token=history::tokenizer::token_type_invalid,
        int leading_space_index=-1,
        bool is_current_line=false,
        bool is_selected=false,
        bool is_highlighted=false,
        int age=-1
    ) const {
        if (token<0) {
            token=-token;
        }
        
        auto f=token_colors.at(token);
        
        if (is_selected) {
            f.bg_color=(is_highlighted)? bg_color_highlighted_selected : bg_color_selected;
        } else
        if (is_highlighted) {
            f.bg_color=bg_color_highlighted;
        } else
        if (leading_space_index!=-1 && token!=history::tokenizer::token_type_group_indentation_current) {
            f.bg_color=((leading_space_index/tab_width)%2==0)? bg_color_indentation_1 : bg_color_indentation_2;
        } else
        if (token==history::tokenizer::token_type_invalid) {
            f.bg_color=(is_current_line)? bg_color_trailing_whitespace_current_line : bg_color_trailing_whitespace;
        } else
        if (is_current_line && token!=history::tokenizer::token_type_group_indentation_current) {
            f.bg_color=bg_color_body_current_line;
        }
        
        if (age_tokens.count(token) && age>=0) {
            double log_age=log10((age==0)? 1 : age)*age_multiplier;
            int floor_log_age=(int)log_age;
            double frac_log_age=log_age-floor_log_age;
            assert(floor_log_age>=0 && frac_log_age>=0);
            
            if (floor_log_age+1<age_colors.size()) {
                f.fg_color=rgba::create(age_colors[floor_log_age].cast()*(1-frac_log_age)+age_colors[floor_log_age+1].cast()*frac_log_age);
            }
        }
        
        if (is_selected) {
            f.fg_color=fg_color_selected;
        }
        
        if (out_cursor_color!=nullptr) {
            *out_cursor_color=rgba(255-f.bg_color.r, 255-f.bg_color.g, 255-f.bg_color.b);
        }
        
        return f;
    }
};

class file_editor : public ui_element {
    file_reference* file;
    const texture* d_font;
    const syntax_highlighting* d_syntax_highlighting;
    
    ivec2 d_size;
    int d_scroll_y;
    ivec2 d_cursor_start;
    ivec2 d_cursor_end;
    
    bool d_clicked=false;
    
    vector<vector<pair<char, char_format>>> screen_buffer;
    
    void get_ordered_selection(ivec2& start, ivec2& end) {
        start=d_cursor_start;
        end=d_cursor_end;
        
        if (start.y>end.y || (start.y==end.y && start.x>end.x)) {
            swap(start, end);
        }
    }
    
    ivec2 get_selection_x_range(ivec2 start, ivec2 end, int y) {
        int selection_x_start=-1;
        int selection_x_end=-1;
        
        if (y==start.y) {
            selection_x_start=start.x;
            selection_x_end=(end.y==start.y)? end.x : int_max;
        } else
        if (y==end.y && end.y!=start.y) {
            selection_x_start=0;
            selection_x_end=end.x;
        } else
        if (y>start.y && y<end.y) {
            selection_x_start=0;
            selection_x_end=int_max;
        }
        
        return ivec2(selection_x_start, selection_x_end);
    }
    
    bool erase_selection() {
        if (d_cursor_start==d_cursor_end) {
            return false;
        }
        
        if (file->is_read_only()) {
            return false;
        }
        
        if (is_command) {
            command_copy_to_current();
        }
        
        ivec2 start;
        ivec2 end;
        get_ordered_selection(start, end);
        
        if (file->is_read_only()) {
            seek_cursor(start);
            return true;
        }
        
        for (int y=start.y;y<=end.y;++y) {
            ivec2 selection_x=get_selection_x_range(start, end, y);
            for (int x=selection_x.x;x<selection_x.y;++x) {
                if (!file->add_character_at_position_to_erase_buffer(ivec2(x+1, y))) {
                    break;
                }
            }
            
            if (y!=start.y) {
                file->add_character_at_position_to_erase_buffer(ivec2(0, y)); //newline
            }
        }
        
        file->erase_characters_in_buffer();
        seek_cursor(start);
        
        set_redraw();
        
        return true;
    }
    
    string get_selected_text() {
        string res;
        
        if (d_cursor_start==d_cursor_end) {
            return res;
        }
        
        ivec2 start;
        ivec2 end;
        get_ordered_selection(start, end);
        
        for (int y=start.y;y<=end.y;++y) {
            auto line=get_line(y);
            
            ivec2 selection_x=get_selection_x_range(start, end, y);
            for (int x=selection_x.x;x<selection_x.y;++x) {
                if (line==nullptr || x+1>=line->size()) {
                    break;
                }
                
                res+=(*line)[x+1];
            }
            
            if (y!=end.y) {
                res+='\n';
            }
        }
        
        return res;
    }
    
    void seek_cursor_impl(ivec2& cursor_pos, ivec2 p) {
        if (p.x<0) {
            p.x=0;
        }
        
        if (p.y<0) {
            p.y=0;
        }
        
        if (p.y>file->num_lines()-1) {
            p.y=file->num_lines()-1;
        }
        
        if (p.y<d_scroll_y) {
            set_scroll(p.y);
        } else
        if (p.y>d_scroll_y+d_size.y-1) {
            set_scroll(p.y-(d_size.y-1));
        }
        
        if (cursor_pos!=p) {
            cursor_pos=p;
            set_redraw();
        }
    }
    
    void seek_cursor(ivec2 p_start, ivec2 p_end, bool allow_callback=true) {
        if (is_command) {
            p_start.y=p_end.y;
        }
        
        seek_cursor_impl(d_cursor_start, p_start);
        seek_cursor_impl(d_cursor_end, p_end);
        
        if (allow_callback) {
            file->notify_line_selected(d_cursor_end.y);
        }
    }
    
    void seek_cursor(ivec2 p) {
        seek_cursor(p, p);
    }
    
    void set_scroll(int t) {
        if (t<0) {
            t=0;
        }
        
        int max_scroll=max(file->num_lines()-d_size.y, 0);
        if (t>max_scroll) {
            t=max_scroll;
        }
        
        if (d_scroll_y!=t) {
            d_scroll_y=t;
            set_redraw();
        }
    }
    
    void resize(ivec2 t_size) {
        d_size=t_size;
        screen_buffer.clear();
        screen_buffer.resize(d_size.y, vector<pair<char, char_format>>(d_size.x));
        set_redraw();
    }
    
    const string* get_line(
        int y,
        const vector<s8>** out_tokens=nullptr,
        int* out_num_leading_spaces=nullptr,
        history::tokenizer::line_state* out_line_state=nullptr,
        const vector<bool>** out_highlighting=nullptr,
        vector<int>* out_character_ages=nullptr
    ) {
        if (out_tokens!=nullptr) {
            *out_tokens=nullptr;
        }
        
        if (out_num_leading_spaces!=nullptr) {
            *out_num_leading_spaces=-1;
        }
        
        if (out_line_state!=nullptr) {
            *out_line_state=history::tokenizer::line_state();
        }
        
        if (out_highlighting!=nullptr) {
            *out_highlighting=nullptr;
        }
        
        if (out_character_ages!=nullptr) {
            out_character_ages->clear();
        }
        
        if (y<0 || y>=file->num_lines()) {
            return nullptr;
        } else {
            if (out_highlighting!=nullptr) {
                *out_highlighting=file->get_line_highlighting(y);
            }
            
            return &(file->get_line(y, out_tokens, out_num_leading_spaces, out_line_state, out_character_ages));
        }
    }
    
    int height() {
        return d_size.y;
    }
    
    void input_character(char c, bool auto_indent=true) {
        if (c=='\n') {
            input_newline(auto_indent);
            return;
        }
        
        if (is_command) {
            command_copy_to_current();
        }
        
        assert(is_ascii_printable(c) || c==' ');
        
        if (file->is_read_only()) {
            return;
        }
        
        history::tokenizer::line_state old_line_state;
        if (d_cursor_end.y>=0 && d_cursor_end.y<file->num_lines()) {
            assert(get_line(d_cursor_end.y, nullptr, nullptr, &old_line_state)!=nullptr);
        }
        
        erase_selection();
        file->input_character_after_position(d_cursor_end, c);
        seek_cursor(ivec2(d_cursor_end.x+1, d_cursor_end.y));
        
        history::tokenizer::line_state new_line_state;
        assert(get_line(d_cursor_end.y, nullptr, nullptr, &new_line_state)!=nullptr);
        
        if (
            old_line_state.num_closing_brackets_before_first_opening_bracket==0 &&
            new_line_state.num_closing_brackets_before_first_opening_bracket==1
        ) {
            indent_selected_lines(-tab_width, false);
        }
        
        set_redraw();
    }
    
    void command_erase_current() {
        auto& target_line=*get_line(file->num_lines()-1);
        for (int x=1;x<target_line.size();++x) {
            file->add_character_at_position_to_erase_buffer(ivec2(x, file->num_lines()-1));
        }
        file->erase_characters_in_buffer();
    }
    
    void command_copy_to_current() {
        assert(is_command);
        
        if (d_cursor_end.y==file->num_lines()-1) {
            return;
        }
        
        command_erase_current();
        
        int source_line_size=get_line(d_cursor_end.y)->size();
        for (int x=1;x<source_line_size;++x) {
            file->input_character_after_position(ivec2(x-1, file->num_lines()-1), (*get_line(d_cursor_end.y))[x]);
        }
        
        seek_cursor(ivec2(d_cursor_start.x, file->num_lines()-1), ivec2(d_cursor_end.x, file->num_lines()-1));
    }
    
    void command_create_new() {
        assert(is_command);
        if (get_line(file->num_lines()-1)->size()!=1) {
            file->input_character_after_position(ivec2(get_line(file->num_lines()-1)->size(), file->num_lines()-1), '\n');
        }
        seek_cursor(ivec2(0, file->num_lines()-1));
    }
    
    void command_execute_current() {
        assert(is_command);
        
        command_callback(get_line(file->num_lines()-1)->substr(1));
        command_create_new();
    }
    
    void input_newline(bool auto_indent=true) {
        if (file->is_read_only()) {
            return;
        }
        
        if (is_command) {
            command_copy_to_current();
            command_execute_current();
        } else {
            erase_selection();
            
            file->input_character_after_position(d_cursor_end, '\n');
            
            int new_line_indentation=0;
            if (auto_indent && d_cursor_end.y>=0 && d_cursor_end.y<file->num_lines()) {
                int old_line_num_leading_spaces;
                history::tokenizer::line_state old_line_state;
                assert(get_line(d_cursor_end.y, nullptr, &old_line_num_leading_spaces, &old_line_state)!=nullptr);
                
                new_line_indentation=
                    old_line_num_leading_spaces+
                    tab_width*clamp(old_line_state.net_brackets+old_line_state.num_closing_brackets_before_first_opening_bracket, 0, 1)
                ;
            }
            
            seek_cursor(ivec2(0, d_cursor_end.y+1));
            
            indent_selected_lines(new_line_indentation, false);
        }
        
        set_redraw();
    }
    
    void indent_selected_lines(int net_spaces, bool fix_misformatted_lines) {
        if (net_spaces==0) {
            return;
        }
        
        ivec2 start;
        ivec2 end;
        get_ordered_selection(start, end);
        
        int y_start=start.y;
        int y_end=end.y;
        if (y_start>y_end) {
            swap(y_start, y_end);
        }
        
        int abs_net_spaces=(net_spaces<0)? -net_spaces : net_spaces;
        bool is_backwards=(net_spaces<0);
        
        int seek_cursor_spaces=abs_net_spaces;
        
        for (int y=y_start;y<=y_end;++y) {
            int indent_spaces=abs_net_spaces;
            int num_leading_spaces;
            assert(get_line(y, nullptr, &num_leading_spaces)!=nullptr);
            
            if (fix_misformatted_lines && num_leading_spaces%tab_width!=0) {
                assert(abs_net_spaces==tab_width);
                if (is_backwards) {
                    indent_spaces=(num_leading_spaces%tab_width);
                } else {
                    indent_spaces=tab_width-(num_leading_spaces%tab_width);
                }
            }
            
            if (y_start==y_end) {
                seek_cursor_spaces=indent_spaces;
            }
            
            for (int x=0;x<indent_spaces;++x) {
                if (is_backwards) {
                    auto v=get_line(y);
                    if (v!=nullptr && v->size()>=x+2 && (*v)[x+1]==' ') {
                        file->add_character_at_position_to_erase_buffer(ivec2(x+1, y));
                    }
                } else {
                    file->input_character_after_position(ivec2(0, y), ' ');
                }
            }
        }
        
        file->erase_characters_in_buffer();
        seek_cursor(
            ivec2(start.x+(is_backwards? -1 : 1)*seek_cursor_spaces, start.y),
            ivec2(end.x  +(is_backwards? -1 : 1)*seek_cursor_spaces, end.y)
        );
        
        set_redraw();
    }
    
    void input_tab(bool is_backwards) {
        if (file->is_read_only()) {
            return;
        }
        
        if (is_command) {
            return;
        }
        
        bool indent=false;
        if (d_cursor_end.y>=0 && d_cursor_end.y<file->num_lines()) {
            int num_leading_spaces;
            assert(get_line(d_cursor_end.y, nullptr, &num_leading_spaces)!=nullptr);
            indent=(d_cursor_end.x<=num_leading_spaces);
        }
        
        if (d_cursor_start==d_cursor_end && !is_backwards && !indent) {
            for (int x=0;x<tab_width;++x) {
                input_character(' ');
                if (d_cursor_end.x%tab_width==0) {
                    break;
                }
            }
        } else {
            indent_selected_lines(is_backwards? -tab_width : tab_width, true);
        }
    }
    
    void erase_character(int direction) {
        assert(direction==-1 || direction==1);
        
        if (is_command) {
            command_copy_to_current();
        }
        
        if (erase_selection()) {
            return;
        }
        
        if (file->is_read_only()) {
            return;
        }
        
        auto line=get_line(d_cursor_end.y);
        
        ivec2 erase_pos;
        if ( direction==1 && (line==nullptr || d_cursor_end.x>=(line->size()-1)) ) {
            erase_pos=ivec2(0, d_cursor_end.y+1);
        } else {
            erase_pos=ivec2(d_cursor_end.x+((direction==-1)? 0 : 1), d_cursor_end.y);
        }
        
        if (is_command && erase_pos.x==0) {
            return;
        }
        
        bool erasing_leading_whitespace=false;
        if (erase_pos.y>=0 && erase_pos.y<file->num_lines()) {
            int num_leading_spaces;
            assert(get_line(erase_pos.y, nullptr, &num_leading_spaces)!=nullptr);
            erasing_leading_whitespace=(erase_pos.x>0 && erase_pos.x<=num_leading_spaces);
        }
        
        if (erasing_leading_whitespace) {
            indent_selected_lines(-tab_width, true);
        } else {
            file->add_character_at_position_to_erase_buffer(erase_pos);
            
            file->erase_characters_in_buffer();
            if (direction==-1) {
                if (d_cursor_end.x==0) {
                    move_cursor_relative(ivec2(half_int_max, -1), false, true, false);
                } else {
                    move_cursor_relative(ivec2(-1, 0), false, false, false);
                }
            }
            
            set_redraw();
        }
    }
    
    void move_cursor_relative(ivec2 delta, bool select, bool truncate, bool scroll) {
        ivec2 new_end=d_cursor_end+delta;
        int old_scroll_y=d_scroll_y;
        
        if (truncate) {
            if (new_end.y<0) {
                new_end.y=0;
            }
            
            if (new_end.y>=file->num_lines()) {
                new_end.y=file->num_lines()-1;
            }
            
            int num_leading_spaces;
            auto line=get_line(new_end.y, nullptr, &num_leading_spaces);
            if (line==nullptr) {
                new_end.x=0;
            } else {
                new_end.x=min(new_end.x, max((int)(line->size()-1), 0));
                if (delta.y==0 && d_cursor_end.x>num_leading_spaces) {
                    new_end.x=max(new_end.x, num_leading_spaces);
                }
            }
        }
        
        if (select) {
            seek_cursor(d_cursor_start, new_end);
        } else {
            seek_cursor(new_end);
        }
        
        if (scroll) {
            set_scroll(old_scroll_y+delta.y);
        }
    }
    
    void move_cursor_absolute(ivec2 mouse_pos, bool select) {
        ivec2 font_size=font_character_size(**d_font);
        int x=(int)(((double)mouse_pos.x)/font_size.x+0.5);
        int y=(int)(((double)mouse_pos.y)/font_size.y);
        y+=d_scroll_y;
        x+=get_min_num_leading_spaces();
        
        if (select) {
            seek_cursor(d_cursor_start, ivec2(x, y));
        } else {
            seek_cursor(ivec2(x, y));
        }
    }
    
    void scroll_relative(int delta_y) {
        set_scroll(d_scroll_y+delta_y);
    }
    
    int get_min_num_leading_spaces() {
        int min_num_leading_spaces=(d_cursor_end.y>=d_scroll_y && d_cursor_end.y<d_scroll_y+d_size.y)? d_cursor_end.x : -1;
        
        for (int screen_y=0;screen_y<d_size.y;++screen_y) {
            int y=screen_y+d_scroll_y;
            
            int line_num_leading_spaces;
            const string* line=get_line(y, nullptr, &line_num_leading_spaces);
            
            if (min_num_leading_spaces==-1 || line_num_leading_spaces<min_num_leading_spaces) {
                min_num_leading_spaces=line_num_leading_spaces;
            }
        }
        
        if (min_num_leading_spaces==-1) {
            min_num_leading_spaces=0;
        }
        
        return min_num_leading_spaces;
    }
    
    void render() override {
        ivec2 start(-1, -1);
        ivec2 end(-1, -1);
        
        int min_num_leading_spaces=get_min_num_leading_spaces();
        
        if (selected) {
            get_ordered_selection(start, end);
        }
        
        rgba cursor_color;
        
        static vector<int> line_character_ages;
        
        for (int screen_y=0;screen_y<d_size.y;++screen_y) {
            int y=screen_y+d_scroll_y;
            
            ivec2 selection_x=get_selection_x_range(start, end, y);
            
            int line_num_leading_spaces=0;
            const vector<s8>* line_tokens=nullptr;
            const vector<bool>* line_highlighting=nullptr;
            const string* line=get_line(y, &line_tokens, &line_num_leading_spaces, nullptr, &line_highlighting, &line_character_ages);
            
            for (int screen_x=0;screen_x<d_size.x;++screen_x) {
                int x=screen_x+min_num_leading_spaces;
                
                bool c_selected=(x>=selection_x.x && x<selection_x.y);
                bool c_highlighted=(line_highlighting!=nullptr && x+1>=0 && x+1<line_highlighting->size() && (*line_highlighting)[x+1]);
                int c_age=(x+1>=0 && x+1<line_character_ages.size())? line_character_ages[x+1] : -1;
                char c=0;
                s8 c_token=history::tokenizer::token_type_invalid;
                
                if (line!=nullptr && x>=0 && x<line->size()-1) {
                    c=(*line)[x+1];
                    c_token=(*line_tokens)[x+1];
                }
                
                auto c_f=d_syntax_highlighting->get_colors(
                    (ivec2(x, y)==d_cursor_end)? &cursor_color : nullptr,
                    c_token,
                    (x<line_num_leading_spaces)? x : -1,
                    y==d_cursor_end.y,
                    c_selected,
                    c_highlighted,
                    c_age
                );
                screen_buffer[screen_y][screen_x]=make_pair(c, c_f);
            }
        }
        
        render_text(*d_font, origin, ivec2(min_num_leading_spaces, d_scroll_y), d_size, [&](ivec2 p) {
            return screen_buffer.at(p.y-d_scroll_y).at(p.x-min_num_leading_spaces);
        }, selected? d_cursor_end : ivec2(-1, -1), cursor_color);
        
        if (!left_mouse_button_pressed) {
            d_clicked=false;
        }
    }
    
    void input_key(bool is_down, int key) override {
        if (!is_down) {
            return;
        }
        
        if (!control_pressed && !alt_pressed && key==SDLK_SPACE) {
            input_character(' ');
        } else
        if (!control_pressed && !alt_pressed && key==SDLK_RETURN) {
            input_newline();
        } else
        if (!control_pressed && !alt_pressed && key==SDLK_TAB) {
            input_tab(shift_pressed);
        } else
        if (!control_pressed && !alt_pressed && key==SDLK_BACKSPACE) {
            erase_character(-1);
        } else
        if (!control_pressed && !alt_pressed && key==SDLK_DELETE) {
            erase_character(1);
        } else
        if (!alt_pressed && (key==SDLK_LEFT || key==SDLK_RIGHT || key==SDLK_UP || key==SDLK_DOWN || key==SDLK_PAGEUP || key==SDLK_PAGEDOWN)) {
            ivec2 last_cursor_end=d_cursor_end;
            
            ivec2 delta(0, 0);
            if (key==SDLK_LEFT) delta.x-=1;
            if (key==SDLK_RIGHT) delta.x+=1;
            if (key==SDLK_UP) delta.y-=1;
            if (key==SDLK_DOWN) delta.y+=1;
            
            if (control_pressed) {
                delta*=control_cursor_move_scale;
            }
            
            if (key==SDLK_PAGEUP) delta.y-=height()-scroll_page_common_lines;
            if (key==SDLK_PAGEDOWN) delta.y+=height()-scroll_page_common_lines;
            move_cursor_relative(delta, shift_pressed, false, key==SDLK_PAGEUP || key==SDLK_PAGEDOWN);
            
            if (d_cursor_end!=last_cursor_end) {
                file->notify_user_selected_character(d_cursor_end);
            }
        } else
        if (!alt_pressed && (key==SDLK_HOME || key==SDLK_END)) {
            ivec2 last_cursor_end=d_cursor_end;
            
            int dir=(key==SDLK_HOME)? -1 : 1;
            move_cursor_relative(ivec2(dir*half_int_max, control_pressed? dir*half_int_max : 0), shift_pressed, true, false);
            
            if (d_cursor_end!=last_cursor_end) {
                file->notify_user_selected_character(d_cursor_end);
            }
        } else
        if (control_pressed && !alt_pressed && (key==SDLK_c || key==SDLK_x)) {
            SDL_SetClipboardText(get_selected_text().c_str());
            if (key==SDLK_x) {
                erase_selection();
            }
        } else
        if (control_pressed && !alt_pressed && key==SDLK_v) {
            auto c=SDL_GetClipboardText();
            if (c!=nullptr) {
                erase_selection();
                input_string(c);
            }
        }
    }
    
    void input_text(char c) override {
        input_character(c);
    }
    
    void input_mouse_button(bool is_down, bool double_click, int button) override {
        if (is_down && button==1) {
            d_clicked=true;
            
            ivec2 last_cursor_end=d_cursor_end;
            
            move_cursor_absolute(mouse_position-origin, shift_pressed);
            
            if (d_cursor_end!=last_cursor_end) {
                file->notify_user_selected_character(d_cursor_end);
            }
        } else        
        if (!is_down && button==1) {
            d_clicked=false;
        }
    }
    
    void input_mouse_move() override {
        if (d_clicked) {
            ivec2 last_cursor_end=d_cursor_end;
            
            move_cursor_absolute(mouse_position-origin, true);
            
            if (d_cursor_end!=last_cursor_end) {
                file->notify_user_selected_character(d_cursor_end);
            }
        }
    }
    
    void input_mouse_wheel(int dy) override {
        scroll_relative(dy * (control_pressed? height()-scroll_page_common_lines : scroll_wheel_scale));
    }
    
    public:
    bool is_command;
    function<void(const string&)> command_callback;
    
    file_editor(const syntax_highlighting& t_syntax_highlighting, const texture& t_font, file_reference& t_file) {
        d_font=&t_font;
        d_syntax_highlighting=&t_syntax_highlighting;
        bind(t_file);
        is_command=false;
    }
    
    void bind(file_reference& t_file) {
        file=&t_file;
        d_scroll_y=0;
        d_cursor_start=ivec2(0, 0);
        d_cursor_end=ivec2(0, 0);
        set_redraw();
    }
    
    //returns actual size
    ivec2 resize_pixels(ivec2 t_size, ivec2 t_origin) {
        ivec2 font_size=font_character_size(**d_font);
        int x=t_size.x/font_size.x;
        int y=t_size.y/font_size.y;
        
        origin=t_origin;
        size_pixels=t_size;
        resize(ivec2(x, y));
        
        return scale(ivec2(x, y), font_size);
    }
    
    void input_command_string(const string& s) {
        assert(is_command);
        
        command_copy_to_current();
        command_create_new();
        
        for (auto c=s.begin();c!=s.end();++c) {
            input_character(*c);
        }
    }
    
    int current_line() const {
        return d_cursor_end.y;
    }
    
    string current_identifier(bool require_cursor_at_end) {
        if (require_cursor_at_end && d_cursor_start!=d_cursor_end) {
            return string();
        }
        
        const vector<s8>* tokens;
        const string* characters=get_line(d_cursor_end.y, &tokens);
        
        if (characters==nullptr) {
            return string();
        }
        
        int start_pos;
        int end_pos;
        if (d_cursor_end.x+1>=0 && d_cursor_end.x+1<characters->size() && math::abs((*tokens)[d_cursor_end.x+1])==history::tokenizer::token_type_identifier) {
            start_pos=d_cursor_end.x+1;
            end_pos=d_cursor_end.x+1;
        } else
        if (d_cursor_end.x>=0 && d_cursor_end.x<characters->size() && math::abs((*tokens)[d_cursor_end.x])==history::tokenizer::token_type_identifier) {
            start_pos=d_cursor_end.x;
            end_pos=d_cursor_end.x;
        } else {
            return string();
        }
        
        while (end_pos<characters->size() && math::abs((*tokens)[end_pos])==history::tokenizer::token_type_identifier) {
            ++end_pos;
        }
        
        while (start_pos>=0 && math::abs((*tokens)[start_pos])==history::tokenizer::token_type_identifier) {
            --start_pos;
        }
        ++start_pos;
        
        if (require_cursor_at_end && end_pos!=d_cursor_end.x+1) {
            return string();
        }
        
        return characters->substr(start_pos, end_pos-start_pos);
    }
    
    void input_string(const string& v) {
        for (auto c=v.begin();c!=v.end();++c) {
            if (is_ascii_printable(*c) || *c==' ' || *c=='\n') {
                input_character(*c, false);
            }
        }
    }
    
    void move_cursor_to_line(int y, bool allow_callback=true) {
        seek_cursor(ivec2(0, y), ivec2(0, y), allow_callback);
    }
    
    void move_cursor_to_position(ivec2 p) {
        seek_cursor(p);
    }
    
    void move_cursor_to_range(ivec2 p_start, ivec2 p_end) {
        seek_cursor(p_start, p_end);
    }
    
    void move_cursor_to_end() {
        move_cursor_to_line(file->num_lines()-1);
    }
    
    ~file_editor() override {}
};
