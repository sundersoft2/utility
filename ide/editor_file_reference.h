struct file_reference {
    virtual int num_lines() const=0;
    virtual const string& get_line(
        int y,
        const vector<s8>** out_tokens,
        int* out_num_leading_spaces,
        history::tokenizer::line_state* out_line_state,
        vector<int>* out_character_ages
    )=0; //y must be in range
    virtual const vector<bool>* get_line_highlighting(int y) { return nullptr; }
    
    virtual bool is_read_only() const { return true; }
    virtual void input_character_after_position(ivec2 p, char c) { assert(false); } //no constraints on p
    virtual void erase_characters_in_buffer() { assert(false); }
    virtual bool add_character_at_position_to_erase_buffer(ivec2 p) { assert(false); } //no constraints on p
    
    virtual void notify_line_selected(int y) {} //called whenever a line changes except when editor::move_cursor_to_line is called with the callback disabled
    virtual void notify_user_selected_character(ivec2 pos) {} //only called when the user selects a different character with the mouse, arrow keys, or home/end buttons
    
    virtual ~file_reference() {}
    
    protected:
    mutable vector<string> empty_line_cache;
    mutable vector<vector<s8>> empty_line_token_cache;
    
    const string& get_empty_line(
        int num_spaces,
        const vector<s8>** out_tokens,
        int* out_num_leading_spaces,
        history::tokenizer::line_state* out_line_state,
        vector<int>* out_character_ages
    ) const {
        while (empty_line_cache.size()<=num_spaces) {
            string s="\n";
            s.insert(s.end(), empty_line_cache.size(), ' ');
            
            vector<s8> s_tokens(empty_line_cache.size()+1, -history::tokenizer::token_type_space);
            s_tokens.at(0)=history::tokenizer::token_type_space;
            
            empty_line_cache.push_back(move(s));
            empty_line_token_cache.push_back(move(s_tokens));
        }
        
        if (out_tokens!=nullptr) {
            *out_tokens=&(empty_line_token_cache.at(num_spaces));
        }
        
        if (out_num_leading_spaces!=nullptr) {
            *out_num_leading_spaces=num_spaces;
        }
        
        if (out_line_state!=nullptr) {
            *out_line_state=history::tokenizer::line_state();
        }
        
        if (out_character_ages!=nullptr) {
            out_character_ages->clear();
        }
        
        return empty_line_cache.at(num_spaces);
    }
};

class temporary_file_reference : public file_reference {
    vector<ivec2> erase_buffer;
    
    int next_group_id;
    
    int current_group_id;
    int current_group_line_index;
    
    int num_leading_spaces(const string& s) {
        assert(s.size()>=1);
        return find_if(s.begin()+1, s.end(), [&](char c){ return c!=' '; })-(s.begin()+1);;
    }
    
    public:
    bool read_only;
    bool highlight_current;
    
    struct line_data {
        string value;
        vector<s8> tokens;
        int group_id;
        function<void()> callback;
        vector<bool> highlighting;
        
        line_data() : group_id(-1) {}
    };
    
    vector<line_data> lines;
    
    temporary_file_reference() : read_only(true), highlight_current(true), next_group_id(1), current_group_id(-1), current_group_line_index(-1) {}
    
    pair<int, int> get_current_group() {
        if (current_group_line_index==-1) {
            return make_pair(-1, -1);
        }
        
        int start=current_group_line_index;
        for (;start>=0 && lines[start].group_id==current_group_id;--start);
        ++start;
        
        int end=current_group_line_index;
        for (;end<lines.size() && lines[end].group_id==current_group_id;++end);
        --end;
        
        return make_pair(start, end);
    }
    
    void seek_current_group(int delta) {
        assert(delta==-1 || delta==1);
        
        int old_current_group_id=current_group_id;
        int old_current_group_line_index=current_group_line_index;
        
        if (current_group_line_index==-1) {
            assert(current_group_id==-1);
            
            if (delta==-1) {
                current_group_line_index=lines.size()-1;
            } else {
                current_group_line_index=0;
            }
        } else {
            assert(current_group_id!=-1);
        }
        
        for (;current_group_line_index>=0 && current_group_line_index<lines.size();current_group_line_index+=delta) {
            if (lines[current_group_line_index].group_id!=-1 && lines[current_group_line_index].group_id!=current_group_id) {
                break;
            }
        }
        
        if (current_group_line_index==-1 || current_group_line_index==lines.size()) {
            current_group_id=old_current_group_id;
            current_group_line_index=old_current_group_line_index;
        } else {
            current_group_id=lines[current_group_line_index].group_id;
            
            if (lines[current_group_line_index].callback) {
                lines[current_group_line_index].callback();
            }
            
            assert(current_group_id!=old_current_group_id);
            assert(current_group_line_index!=old_current_group_line_index);
        }
    }
    
    void clear() {
        lines.clear();
        next_group_id=1;
        current_group_id=-1;
        current_group_line_index=-1;
    }
    
    void push_back(const string& value, const vector<s8>& tokens=vector<s8>()) {
        assert(value.at(0)=='\n');
        
        line_data l;
        l.value=value;
        l.tokens=tokens;
        
        lines.push_back(move(l));
    }
    
    struct push_back_data {
        string value;
        vector<s8> tokens;
        vector<bool> highlighting;
        bool is_code;
        
        push_back_data() : is_code(false) {}
        push_back_data(const string& t_value) : value(t_value), is_code(false) {}
        push_back_data(const string& t_value, const vector<s8>& t_tokens, const vector<bool>& t_highlighting=vector<bool>()) :
            value(t_value), tokens(t_tokens), highlighting(t_highlighting), is_code(true)
        {}
    };
    
    void push_back_group(vector<push_back_data> data, function<void()> callback=function<void()>()) {
        int first_line=lines.size();
        
        for (auto c=data.begin();c!=data.end();++c) {
            auto& res=*c;
            if (res.value.empty()) {
                break;
            }
            
            assert(res.value[0]=='\n');
            
            if (res.is_code) {
                assert(res.tokens.empty() || res.tokens.size()==res.value.size());
                if (res.tokens.empty()) {
                    res.tokens.push_back(history::tokenizer::token_type_space);
                }
            } else {
                assert(res.tokens.empty());
            }
            
            line_data l;
            l.value=move(res.value);
            l.tokens=move(res.tokens);
            l.callback=callback;
            l.group_id=next_group_id;
            l.highlighting=move(res.highlighting);
            
            lines.push_back(move(l));
        }
        
        int min_leading_spaces=-1;
        
        for (int x=first_line;x<lines.size();++x) {
            auto& c=lines[x];
            
            if (c.tokens.empty()) {
                //header
                c.tokens.resize(c.value.size(), -history::tokenizer::token_type_group_header_default);
                c.tokens.at(0)=history::tokenizer::token_type_group_header_default;
            } else {
                //code
                int leading_spaces=num_leading_spaces(c.value);
                if (min_leading_spaces==-1 || min_leading_spaces>leading_spaces) {
                    leading_spaces=min_leading_spaces;
                }
                
                auto last_token=c.tokens.back();
                if (last_token>0) {
                    last_token=-last_token;
                }
                
                while (c.tokens.size()<c.value.size()) {
                    c.tokens.push_back(last_token);
                }
            }
        }
        
        if (min_leading_spaces==-1) {
            min_leading_spaces=0;
        }
        
        for (int x=first_line;x<lines.size();++x) {
            auto& c=lines[x];
            
            assert(!c.tokens.empty());
            if (c.tokens.front()==history::tokenizer::token_type_group_header_default) {
                continue;
            }
            
            assert(c.value.size()>=min_leading_spaces+1);
            assert(c.tokens.size()==c.value.size());
            
            bool has_highlighting=!c.highlighting.empty();
            
            c.value.erase(c.value.begin()+1, c.value.begin()+min_leading_spaces+1);
            c.tokens.erase(c.tokens.begin()+1, c.tokens.begin()+min_leading_spaces+1);
            if (has_highlighting) {
                c.highlighting.erase(c.highlighting.begin()+1, c.highlighting.begin()+min(min_leading_spaces+1, (int)c.highlighting.size()));
            }
            
            c.value.insert(c.value.begin()+1, tab_width, ' ');
            c.tokens.insert(c.tokens.begin()+1, tab_width, -history::tokenizer::token_type_group_indentation_default);
            c.tokens.at(1)=history::tokenizer::token_type_group_indentation_default;
            
            if (has_highlighting) {
                c.highlighting.insert(c.highlighting.begin()+1, tab_width, false);
            }
            
            if (c.tokens.size()>=tab_width+2 && c.tokens[tab_width+1]<0) {
                c.tokens[tab_width+1]=-c.tokens[tab_width+1];
            }
        }
        
        ++next_group_id;
    }
    
    void notify_line_selected(int y) override {
        if (y>=0 && y<lines.size() && lines[y].group_id!=-1) {
            auto& l=lines[y];
            
            if (current_group_id!=l.group_id) {
                current_group_id=l.group_id;
                current_group_line_index=y;
                
                if (l.callback) {
                    l.callback();
                }
            }
        }
    }
    
    int num_lines() const override { return lines.size(); }
    const string& get_line(
        int y,
        const vector<s8>** out_tokens,
        int* out_num_leading_spaces,
        history::tokenizer::line_state* out_line_state,
        vector<int>* out_character_ages
    ) override {
        auto& res=lines[y];
        
        bool is_current=(current_group_id!=-1 && res.group_id==current_group_id);
        
        if (out_tokens!=nullptr) {
            if (res.tokens.empty()) {
                get_empty_line(res.value.size()-1, out_tokens, nullptr, nullptr, nullptr);
            } else {
                if (highlight_current) {
                    for (auto c=res.tokens.begin();c!=res.tokens.end();++c) {
                        bool negative=(*c<0);
                        int v=(negative? -*c : *c);
                        if (v==history::tokenizer::token_type_group_header_current || v==history::tokenizer::token_type_group_header_default) {
                            *c=(negative? -1 : 1)*(is_current? history::tokenizer::token_type_group_header_current : history::tokenizer::token_type_group_header_default);
                        } else
                        if (v==history::tokenizer::token_type_group_indentation_current || v==history::tokenizer::token_type_group_indentation_default) {
                            *c=(negative? -1 : 1)*(is_current? history::tokenizer::token_type_group_indentation_current : history::tokenizer::token_type_group_indentation_default);
                        }
                    }
                }
                
                *out_tokens=&(res.tokens);
            }
        }
        
        if (out_num_leading_spaces!=nullptr) {
            *out_num_leading_spaces=num_leading_spaces(res.value);
        }
        
        if (out_line_state!=nullptr) {
            *out_line_state=history::tokenizer::line_state(); //not supported
        }
        
        if (out_character_ages!=nullptr) {
            out_character_ages->clear();
        }
        
        return res.value;
    }
    
    const vector<bool>* get_line_highlighting(int y) {
        return &(lines[y].highlighting);
    }
    
    bool is_read_only() const override { return read_only; }
    
    void input_character_after_position(ivec2 p, char c) override {
        assert(!read_only);
        assert(p.x>=0 && p.y>=0);
        while (p.y>=lines.size()) {
            push_back("\n");
        }
        
        auto& c_line=lines[p.y];
        while (p.x>=c_line.value.size()) {
            c_line.value+=' ';
        }
        
        c_line.tokens.clear();
        
        if (c=='\n') {
            line_data next_line;
            next_line.value="\n"+ c_line.value.substr(p.x+1);
            c_line.value.resize(p.x+1);
            lines.insert(lines.begin()+(p.y+1), move(next_line));
        } else {
            c_line.value.insert(p.x+1, 1, c);
        }
    }
    
    void erase_characters_in_buffer() override {
        assert(!read_only);
        
        set<int> modified_lines;
        
        //just set the deleted characters to null first
        for (auto c=erase_buffer.begin();c!=erase_buffer.end();++c) {
            auto& c_line=lines[c->y];
            
            c_line.value[c->x]=0;
            c_line.tokens.clear();
            
            modified_lines.insert(c->y);
        }
        
        vector<pair<int /*previous line, retained*/, int /*removed line*/>> line_merge_buffer; //sorted by removed line
        
        //first pass: remove deleted characters and figure out which lines need to be merged
        for (auto c=modified_lines.begin();c!=modified_lines.end();++c) {
            auto& c_line=lines[*c];
            
            if (c_line.value[0]=='\0') {
                assert(*c!=0);
                line_merge_buffer.emplace_back(*c-1, *c);
            }
            
            //get rid of any null character
            c_line.value.erase(remove_if(c_line.value.begin(), c_line.value.end(), [&](char v){ return v==0; }), c_line.value.end());
        }
        
        //second pass: merge lines. need to go backwards in case multiple adjacent lines were removed
        for (auto c=line_merge_buffer.rbegin();c!=line_merge_buffer.rend();++c) {
            lines[c->first].value+=move(lines[c->second].value);
            lines[c->second].value.clear();
        }
        
        //third pass: remove merged line sources
        lines.erase(remove_if(lines.begin(), lines.end(), [&](const line_data& c){ return c.value.empty(); }), lines.end());
        
        erase_buffer.clear();
    }
    
    bool add_character_at_position_to_erase_buffer(ivec2 p) override {
        assert(!read_only);
        assert(p.x>=0 && p.y>=0);
        
        if (p.x==0 && p.y==0) {
            return false;
        }
        
        if (p.y>=lines.size()) {
            return false;
        }
        
        if (p.x>=lines[p.y].value.size()) {
            return false;
        }
        
        erase_buffer.push_back(p);
        
        return true;
    }
    
    ~temporary_file_reference() override {}
};

class history_file_reference : public file_reference {
    history::state* d_state;
    weak_ptr<history::file> d_file_weak;
    highlighting* d_highlighting=nullptr;
    shared_ptr<highlighting::file_cache_entry> d_highlighting_entry;
    
    set<const history::character*> erase_buffer;
    
    const history::line* get_history_line(int y) const {
        auto d_file=d_file_weak.lock();
        
        if (d_file!=nullptr && y>=0 && y<d_file->num_lines()) {
            return &(d_file->get_line(y));
        } else {
            return nullptr;
        }
    }
    
    const history::character* get_character_at_position(ivec2 p) const {
        auto l=get_history_line(p.y);
        if (l!=nullptr && p.x>=0 && p.x<l->cached_value.size()) {
            return l->cached_value_characters[p.x];
        } else {
            return nullptr;
        }
    }
    
    public:
    history_file_reference(history::state& t_state) : d_state(&t_state) {}
    
    weak_ptr<history::file> file() {
        return d_file_weak;
    }
    
    void bind(weak_ptr<history::file> t_file) {
        d_file_weak=t_file;
    }
    
    void bind(highlighting* t_highlighting) {
        d_highlighting=t_highlighting;
        
        if (d_highlighting!=nullptr) {
            d_highlighting_entry=d_highlighting->lookup_file(d_file_weak);
        } else {
            d_highlighting_entry=nullptr;
        }
    }
    
    int num_lines() const {
        auto d_file=d_file_weak.lock();
        return (d_file==nullptr)? 0 : d_file->num_lines();
    }
    
    const string& get_line(
        int y,
        const vector<s8>** out_tokens,
        int* out_num_leading_spaces,
        history::tokenizer::line_state* out_line_state,
        vector<int>* out_character_ages
    ) {
        auto& res=*get_history_line(y);
        
        if (res.cached_empty_line_leading_spaces!=-1) {
            return get_empty_line(res.cached_empty_line_leading_spaces, out_tokens, out_num_leading_spaces, out_line_state, out_character_ages);
        } else {
            if (out_tokens!=nullptr) {
                *out_tokens=&(res.cached_tokens);
            }
            
            if (out_num_leading_spaces!=nullptr) {
                *out_num_leading_spaces=res.cached_line_state.num_spaces_at_start;
            }
            
            if (out_line_state!=nullptr) {
                *out_line_state=res.cached_line_state;
            }
            
            if (out_character_ages!=nullptr) {
                out_character_ages->clear();
                int current_log_index=d_state->last_viewed_log_index();
                for (auto c=res.cached_value_characters.begin();c!=res.cached_value_characters.end();++c) {
                    out_character_ages->push_back(current_log_index - (*c)->last_viewed_log_index());
                }
            }
            
            return res.cached_value;
        }
    }
    
    const vector<bool>* get_line_highlighting(int y) {
        if (d_highlighting!=nullptr) {
            return d_highlighting->lookup_line(d_highlighting_entry, y);
        } else {
            return nullptr;
        }
    }
    
    void notify_user_selected_character(ivec2 pos) override {
        auto l=get_history_line(pos.y);
        if (l!=nullptr) {
            int x=clamp(pos.x, 0, l->cached_value_characters.size()-1);
            d_state->select_character(*(l->cached_value_characters[x]));
        }
    };
    
    bool is_read_only() const {
        auto d_file=d_file_weak.lock();
        return d_file==nullptr;
    }
    
    void input_character_after_position(ivec2 p, char c) {
        auto d_file=d_file_weak.lock();
        
        assert(p.x>=0 && p.y>=0);
        assert(d_file!=nullptr);
        
        while (p.y>=d_file->num_lines()) {
            d_state->insert_character(d_file->characters().back(), '\n');
        }
        
        if (c!='\n' && c!=' ') {
            while (true) {
                int line_size=get_history_line(p.y)->cached_value.size();
                if (p.x<line_size) {
                    break;
                }
                
                d_state->insert_character(*get_character_at_position(ivec2(line_size-1, p.y)), ' ');
            }
        }
        
        if (c=='\n') {
            int line_size=get_history_line(p.y)->cached_value.size();
            d_state->insert_character(*get_character_at_position(ivec2(min(p.x, line_size-1), p.y)), c);
        } else
        if (c==' ') {
            int line_size=get_history_line(p.y)->cached_value.size();
            if (p.x<line_size-1) {
                d_state->insert_character(*get_character_at_position(p), c);
            }
        } else {
            d_state->insert_character(*get_character_at_position(p), c);
        }
    }
    
    void erase_characters_in_buffer() {
        auto d_file=d_file_weak.lock();
        
        assert(d_file!=nullptr);
        
        set<const history::line*> modified_lines;
        
        //figure out which lines were effected by maintaining a set of newline characters of effected lines
        //(if a newline character is deleted, remove it from the set)
        for (auto i=erase_buffer.begin();i!=erase_buffer.end();++i) {
            auto& c=**i;
            auto c_line=&(d_file->get_line(c));
            if (c.value()=='\n') {
                modified_lines.erase(c_line);
            } else {
                modified_lines.insert(c_line);
            }
            
            d_state->delete_character(c);
        }
        erase_buffer.clear();
        
        //for lines that were effected, remove trailing spaces
        for (auto i=modified_lines.begin();i!=modified_lines.end();++i) {
            auto& c_line=**i;
            auto c_line_begin=d_file->begin(c_line);
            auto c_line_i=d_file->end(c_line);
            --c_line_i;
            
            for (;c_line_i!=c_line_begin;) {
                auto new_c_line_i=c_line_i;
                --new_c_line_i;
                
                if (c_line_i->value()==' ') {
                    d_state->delete_character(*c_line_i);
                } else {
                    break;
                }
                
                c_line_i=new_c_line_i;
            }
        }
    }
    
    bool add_character_at_position_to_erase_buffer(ivec2 p) {
        auto d_file=d_file_weak.lock();
        
        assert(d_file!=nullptr);
        
        auto v=get_character_at_position(p);
        if (v!=nullptr && v->iter!=d_file->characters().begin()) {
            erase_buffer.insert(v);
            return true;
        } else {
            return false;
        }
    }
    
    ~history_file_reference() override {}
};
