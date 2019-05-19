namespace history {


class character;
class file;

struct line {
    safe_list<character>::iterator iter;
    
    int cached_line_index;
    
    string cached_value; //starts with a line break if valid. if empty, invalid
    vector<character*> cached_value_characters;
    
    vector<s8> cached_tokens;
    tokenizer::line_state cached_line_state;
    
    //will look at following line first, but only if its net brackets is nonnegative
    //if that doesn't work, look at previous line and use its leading space count plus net brackets times tab width
    int cached_empty_line_leading_spaces;
    
    search::line_index::line_data search_line_data;
    
    safe_list<line*>::iterator uncached_lines_iter;
    
    line() : cached_line_index(-1), cached_empty_line_leading_spaces(-1) {}
};

class character : disable_copy {
    int d_character_id;
    char d_value;
    
    //note: if a character is deleted, its operations are merged with an adjacent character, so the ids
    // in these operations may not match this character's id
    //vector<int /*log index, sorted*/> d_operations; <== disabled, takes quadratic time to delete stuff
    //(to fix time complexity issue, make the array not sorted and when doing a merge, make sure to insert
    // the smaller array into the bigger array. could also use a linked list instead of an array). can
    // have a sorted flag if it really needs to be sorted. could also use a red-black tree
    vector<int> d_merged_character_ids; //not sorted
    
    int d_last_modified_log_index;
    int d_last_viewed_log_index;
    
    public:
    safe_list<character>::iterator iter;
    safe_list<line>::iterator line_iter; //null if not a newline character
    file* parent;
    
    character(int t_character_id, char t_value, int initial_operation) : d_character_id(t_character_id), d_value(t_value) {
        /*if (initial_operation!=-1) {
            d_operations.push_back(initial_operation);
        }*/
        
        d_last_modified_log_index=initial_operation;
        d_last_viewed_log_index=initial_operation;
    }
    
    int character_id() const {
        assert(d_character_id!=-1);
        return d_character_id;
    }
    
    char value() const {
        assert(d_character_id!=-1);
        return d_value;
    }
    
    int last_modified_log_index() const {
        return d_last_modified_log_index;
    }
    
    int last_viewed_log_index() const {
        return d_last_viewed_log_index;
    }
    
    /*const vector<int>& operations() const {
        assert(d_character_id!=-1);
        return d_operations;
    }*/
    
    const vector<int>& merged_character_ids() const {
        assert(d_character_id!=-1);
        return d_merged_character_ids;
    }
    
    void add_operation(int log_index, bool is_write) {
        d_last_viewed_log_index=log_index;
        
        if (is_write) {
            d_last_modified_log_index=log_index;
        }
        /*assert(d_character_id!=-1);
        d_operations.push_back(log_index);*/
    }
    
    void merge(character& source) {
        /*int initial_size=d_operations.size();
        d_operations.insert(d_operations.end(), source.d_operations.begin(), source.d_operations.end());
        inplace_merge(d_operations.begin(), d_operations.begin()+initial_size, d_operations.end());*/
        
        d_last_modified_log_index=max(d_last_modified_log_index, source.d_last_modified_log_index);
        d_last_viewed_log_index=max(d_last_viewed_log_index, source.d_last_viewed_log_index);
        
        d_merged_character_ids.insert(d_merged_character_ids.end(), source.d_merged_character_ids.begin(), source.d_merged_character_ids.end());
        d_merged_character_ids.push_back(source.d_character_id);
        
        source.d_character_id=-1;
        source.d_value=-1;
        source.d_last_modified_log_index=-1;
        source.d_last_viewed_log_index=-1;
        //source.d_operations.clear();
        source.d_merged_character_ids.clear();
    }
};

int get_character_id(const character* c) {
    return c->character_id();
}

char get_character_value(const character* c) {
    return c->value();
}

class file : disable_copy {
    int d_character_id;
    safe_list<character> d_characters;
    safe_list<line> d_lines;
    int d_lines_size;
    
    search::line_index* d_search_index;
    safe_list<line*>* d_uncached_lines;
    
    vector<safe_list<line>::iterator> d_lines_vector; //empty if invalid
    
    int d_last_modified_log_index;
    int d_last_viewed_log_index;
    
    void dirty_line_cached_value(line& l) {
        l.cached_value.clear();
        l.cached_value_characters.clear();
        l.cached_tokens.clear();
        l.cached_empty_line_leading_spaces=-1;
        d_search_index->clear_line(l.search_line_data);
        //do not clear the line state; need to check if it changed or not
        
        //line can be dirtied multiple times
        if (l.uncached_lines_iter==d_uncached_lines->end()) {
            d_uncached_lines->push_front(&l);
            l.uncached_lines_iter=d_uncached_lines->begin();
        }
    }
    
    safe_list<line>::iterator get_cached_line(int t_line_index) {
        update_lines_vector();
        
        if (!d_lines_vector.at(t_line_index)->cached_value.empty()) {
            return d_lines_vector[t_line_index];
        }
        
        auto tokenize_line=[&](int index) {
            auto& l=*d_lines_vector[index];
            
            assert(l.cached_value.empty());
            assert(l.cached_value_characters.empty());
            
            for (auto i=l.iter;i!=d_characters.end();++i) {
                if (i!=l.iter && i->value()=='\n') {
                    break;
                }
                
                l.cached_value+=i->value();
                l.cached_value_characters.push_back(&*i);
            }
            
            auto tokens=tokenizer::tokenize((index==0)? tokenizer::line_state() : d_lines_vector[index-1]->cached_line_state, l.cached_value);
            
            l.cached_tokens=move(tokens.first);
            l.cached_line_state=tokens.second;
            
            d_search_index->set_line(l.search_line_data, l.cached_value_characters, l.cached_tokens, search_max_identifier_length);
            
            assert(l.uncached_lines_iter!=d_uncached_lines->end());
            d_uncached_lines->erase(l.uncached_lines_iter);
            l.uncached_lines_iter=d_uncached_lines->end();
        };
        
        //recalculate any empty lines or uncached lines before the current line. end with a non-empty line if possible
        int start_tokenize_line_index=t_line_index-1;
        for (;start_tokenize_line_index>=0;--start_tokenize_line_index) {
            auto& l=*d_lines_vector[start_tokenize_line_index];
            if (l.cached_value.size()>=2) {
                break;
            }
        }
        if (start_tokenize_line_index==-1) {
            start_tokenize_line_index=0;
        }
        
        for (int index=start_tokenize_line_index;index<=t_line_index-1;++index) {
            dirty_line_cached_value(*(d_lines_vector[index]));
            tokenize_line(index);
        }
        
        //recalculate requested line
        {
            auto& l=*d_lines_vector[t_line_index];
            dirty_line_cached_value(l);
            tokenize_line(t_line_index);
        }
        
        //recalculate any empty lines or lines where the previous line token changed after the current line. end with a non empty line if possible
        int end_tokenize_line_index=t_line_index+1;
        for (;end_tokenize_line_index<d_lines_vector.size();++end_tokenize_line_index) {
            auto& l=*d_lines_vector[end_tokenize_line_index];
            
            s8 old_token=l.cached_line_state.token;
            
            dirty_line_cached_value(l);
            tokenize_line(end_tokenize_line_index);
            
            s8 new_token=l.cached_line_state.token;
            
            if (new_token==old_token && l.cached_value.size()>=2) {
                break;
            }
        }
        if (end_tokenize_line_index==d_lines_vector.size()) {
            end_tokenize_line_index=d_lines_vector.size()-1;
        }
        
        //for any recalculated empty lines, figure out how many leading spaces they should have
        {
            //mark all recalculated empty lines as unknown (-2)
            for (int index=start_tokenize_line_index;index<=end_tokenize_line_index;++index) {
                auto& l=*d_lines_vector[index];
                assert(l.cached_value.size()!=0);
                
                if (l.cached_value.size()>=2) {
                    l.cached_empty_line_leading_spaces=-1;
                } else {
                    l.cached_empty_line_leading_spaces=-2;
                }
            }
            
            //for any groups of empty lines before a non-empty line with no closing brackets, use the non-empty line's leading spaces
            /*for (int index=end_tokenize_line_index;index>=start_tokenize_line_index;) {
                auto& l=*d_lines_vector[index];
                if (l.cached_value.size()<2 || l.cached_line_state.net_brackets<0) {
                    --index;
                    continue;
                }
                
                int leading_spaces=l.cached_line_state.num_spaces_at_start;
                
                int empty_index=index-1;
                for (;empty_index>=start_tokenize_line_index;--empty_index) {
                    auto& empty_l=*d_lines_vector[empty_index];
                    if (empty_l.cached_value.size()>=2) {
                        break;
                    }
                    
                    assert(empty_l.cached_empty_line_leading_spaces==-2);
                    empty_l.cached_empty_line_leading_spaces=leading_spaces;
                }
                
                index=empty_index;
            }*/
            
            //if that didn't work, look for a non-empty line before the empty line and take the number of opening brackets into account
            for (int index=start_tokenize_line_index;index<=end_tokenize_line_index;) {
                auto& l=*d_lines_vector[index];
                if (l.cached_value.size()<2) {
                    ++index;
                    continue;
                }
                
                int leading_spaces=
                    l.cached_line_state.num_spaces_at_start+
                    tab_width*clamp(l.cached_line_state.net_brackets+l.cached_line_state.num_closing_brackets_before_first_opening_bracket, 0, 1)
                ;
                
                int empty_index=index+1;
                for (;empty_index<=end_tokenize_line_index;++empty_index) {
                    auto& empty_l=*d_lines_vector[empty_index];
                    if (empty_l.cached_value.size()>=2) {
                        break;
                    }
                    
                    if (empty_l.cached_empty_line_leading_spaces==-2) {
                        empty_l.cached_empty_line_leading_spaces=leading_spaces;
                    }
                }
                
                index=empty_index;
            }
            
            //if the file only has empty lines, use 0 for the number of leading spaces
            for (int index=start_tokenize_line_index;index<=end_tokenize_line_index;++index) {
                auto& l=*d_lines_vector[index];
                if (l.cached_empty_line_leading_spaces==-2) {
                    assert(l.cached_value.size()==1);
                    l.cached_empty_line_leading_spaces=0;
                }
            }
        }
        
        return d_lines_vector[t_line_index];
    }
    
    void dirty_lines_vector() {
        d_lines_vector.clear();
    }
    
    safe_list<line>::iterator find_line(const character& target, int* column_index=nullptr) const {
        if (column_index!=nullptr) {
            *column_index=0;
        }
        
        auto i=target.iter;
        while (i->value()!='\n') {
            assert(i!=d_characters.begin());
            --i;
            
            if (column_index!=nullptr) {
                ++*column_index;
            }
        }
        
        assert(i->line_iter!=d_lines.end());
        return i->line_iter;
    }
    
    public:
    string name;
    weak_ptr<file> d_ptr;
    
    explicit file(int t_character_id, int operation, search::line_index& t_search_index, safe_list<line*>& t_uncached_lines) : d_lines_size(0) {
        d_search_index=&t_search_index;
        d_uncached_lines=&t_uncached_lines;
        
        d_character_id=t_character_id;
        d_characters.emplace_front(t_character_id, '\n', -1);
        
        d_lines.emplace_front(); ++d_lines_size;
        d_lines.front().iter=d_characters.begin();
        d_uncached_lines->push_front(&(d_lines.front()));
        d_lines.front().uncached_lines_iter=d_uncached_lines->begin();
        
        d_characters.front().iter=d_characters.begin();
        d_characters.front().line_iter=d_lines.begin();
        d_characters.front().parent=this;
        
        d_last_modified_log_index=operation;
        d_last_viewed_log_index=operation;
    }
    
    void update_lines_vector() {
        if (!d_lines_vector.empty()) {
            return;
        }
        
        for (auto c=d_lines.begin();c!=d_lines.end();++c) {
            d_lines_vector.push_back(c);
            c->cached_line_index=d_lines_vector.size()-1;
        }
        
        assert(d_lines_vector.size()==d_lines_size);
    }
    
    int num_lines() const {
        return d_lines_size;
    }
    
    int character_id() const {
        return d_character_id;
    }
    
    int line_index(const character& c, int* column_index=nullptr) {
        update_lines_vector();
        int res=find_line(c, column_index)->cached_line_index;
        assert(res!=-1);
        return res;
    }
    
    const line& get_line(const character& c) const {
        return *find_line(c);
    }
    
    safe_list<character>::const_iterator begin(const line& t_line) const {
        return t_line.iter;
    }
    
    safe_list<character>::const_iterator end(const line& t_line) const {
        auto i=t_line.iter->line_iter;
        ++i;
        if (i==d_lines.end()) {
            return d_characters.end();
        } else {
            return i->iter;
        }
    }
    
    int last_modified_log_index() const {
        return d_last_modified_log_index;
    }
    
    int last_viewed_log_index() const {
        return d_last_viewed_log_index;
    }
    
    const line& get_line(int t_line_index) {
        return *get_cached_line(t_line_index);
    }
    
    const safe_list<character>& characters() const {
        return d_characters;
    }
    
    //column index 0 is the newline character. there is an invisible newline character at the start of the file
    /*const character& get_character(int line_index, int column_index) const {
        return *(get_cached_line(line_index)->cached_value_characters.at(column_index));
    }*/
    
    ivec2 get_position(const character& c) {
        update_lines_vector();
        
        int column;
        auto& l=*find_line(c, &column);
        return ivec2(column, l.cached_line_index);
    }
    
    character& insert_character(const character& previous, int t_character_id, char t_value, int initial_operation) {
        //reset cache for line containing previous
        safe_list<line>::iterator previous_line=find_line(previous);
        dirty_line_cached_value(*previous_line);
        
        auto next_iter=previous.iter;
        ++next_iter;
        auto c_iter=d_characters.emplace(next_iter, t_character_id, t_value, initial_operation);
        c_iter->iter=c_iter;
        c_iter->line_iter=d_lines.end();
        c_iter->parent=this;
        
        //if target is a newline character, update lines list and reset cache
        if (c_iter->value()=='\n') {
            auto i=previous_line;
            ++i;
            c_iter->line_iter=d_lines.emplace(i); ++d_lines_size;
            c_iter->line_iter->iter=c_iter;
            
            d_uncached_lines->push_front(&*(c_iter->line_iter));
            c_iter->line_iter->uncached_lines_iter=d_uncached_lines->begin();
            
            dirty_lines_vector();
        }
        
        d_last_modified_log_index=initial_operation;
        d_last_viewed_log_index=initial_operation;
        return *c_iter;
    }
    
    void delete_character(const character& delete_target, const character& merge_target, int operation) {
        assert(delete_target.iter!=d_characters.begin());
        safe_list<line>::iterator delete_line=find_line(delete_target);
        dirty_line_cached_value(*delete_line);
        
        //if delete_target is a newline character, mark previous line as dirty, update lines list, and reset cache
        if (delete_target.value()=='\n') {
            auto i=delete_target.iter;
            --i;
            auto i_line=find_line(*i);
            dirty_line_cached_value(*i_line);
            
            assert(delete_target.line_iter!=d_lines.end());
            
            if (delete_target.line_iter->uncached_lines_iter!=d_uncached_lines->end()) {
                d_uncached_lines->erase(delete_target.line_iter->uncached_lines_iter);
                delete_target.line_iter->uncached_lines_iter=d_uncached_lines->end();
            }
            
            d_lines.erase(delete_target.line_iter); --d_lines_size;
            
            dirty_lines_vector();
        }
        
        merge_target.iter->merge(*delete_target.iter);
        d_characters.erase(delete_target.iter);
        
        d_last_modified_log_index=operation;
        d_last_viewed_log_index=operation;
    }
    
    void select_character(const character& target, int operation) {
        d_last_viewed_log_index=operation;
    }
    
    void remove_uncached_lines() {
        for (auto c=d_lines.begin();c!=d_lines.end();++c) {
            if (c->uncached_lines_iter!=d_uncached_lines->end()) {
                d_uncached_lines->erase(c->uncached_lines_iter);
                c->uncached_lines_iter=d_uncached_lines->end();
            }
        }
    }
};

class state : disable_copy {
    int d_next_character_id;
    persistent_log* d_log_entries;
    map<int /*character id*/, shared_ptr<file>> d_files;
    map<string, int /*character id*/> d_files_by_name;
    deque<character*> d_characters_by_id; //if the character was deleted, will point to the character containing its log entries, if any
    search::line_index d_search_index;
    safe_list<line*> d_uncached_lines;
    
    int d_last_modified_log_index;
    int d_last_viewed_log_index;
    bool allow_generate_id;
    int override_generate_id;
    int override_log_index;
    
    void assign_character_by_id(int id, character* new_value, character* old_value=nullptr) {
        if (d_characters_by_id.size()<=id) {
            d_characters_by_id.resize(id+1);
        }
        
        character*& c=d_characters_by_id.at(id);
        assert(c==old_value);
        c=new_value;
    }
    
    void set_override_generate_id(int id) {
        assert(override_generate_id==-1);
        override_generate_id=id;
    }
    
    int generate_id() {
        int res;
        if (allow_generate_id) {
            res=d_next_character_id;
            ++d_next_character_id;
        } else {
            assert(override_generate_id!=-1);
            res=override_generate_id;
            d_next_character_id=max(res+1, d_next_character_id);
            override_generate_id=-1;
        }
        return res;
    }
    
    public:
    state(persistent_log* t_log_entries) :
        d_log_entries(t_log_entries),
        d_next_character_id(1),
        d_last_modified_log_index(0),
        d_last_viewed_log_index(0),
        allow_generate_id(false),
        override_generate_id(-1),
        override_log_index(-1)
    {
        d_log_entries->set_read_only(true);
        
        auto& c_log_entries=d_log_entries->log_entries();
        int c_index=0;
        for (auto c=c_log_entries.begin();c!=c_log_entries.end();++c,++c_index) { //can make this multithreaded (by sorting by file) and run concurrent with log read
            override_log_index=c_index;
            
            auto type=c->type();
            
            if (type==log_entry_type_insert) {
                set_override_generate_id(c->character_id());
                insert_character(*character_by_id(c->previous_character_id()), c->value());
            } else
            if (type==log_entry_type_delete) {
                delete_character(*character_by_id(c->character_id()));
            } else
            if (type==log_entry_type_create_file) {
                set_override_generate_id(c->character_id());
                create_file(c->name());
            } else
            if (type==log_entry_type_move_file) {
                move_file(get_file(c->character_id()), c->name());
            } else
            if (type==log_entry_type_delete_file) {
                delete_file(get_file(c->character_id()));
            } else
            if (type==log_entry_type_select) {
                select_character(*character_by_id(c->character_id()));
            } else {
                assert(false);
            }
            
            assert(override_log_index==-1);
        }
        
        allow_generate_id=true;
        
        d_log_entries->set_read_only(false);
    }
    
    const search::line_index& search_index() const {
        return d_search_index;
    }
    
    int last_modified_log_index() const {
        return d_last_modified_log_index;
    }
    
    int last_viewed_log_index() const {
        return d_last_viewed_log_index;
    }
    
    const deque<log_entry>& log_entries() const {
        return d_log_entries->log_entries();
    }
    
    const character* character_by_id(int id) const {
        if (id<0 || id>=d_characters_by_id.size()) {
            return nullptr;
        } else {
            return d_characters_by_id[id];
        }
    }
    
    void cache_all_lines() {
        line* previous_line=nullptr;
        
        while (!d_uncached_lines.empty()) {
            auto c=d_uncached_lines.begin();
            auto& c_line=**c;
            auto& c_file=*(c_line.iter->parent);
            
            assert(&c_line!=previous_line); //prevent infinite loops if there is a bug
            previous_line=&c_line;
            
            if (c_line.cached_line_index==-1) {
                c_file.update_lines_vector();
                assert(c_line.cached_line_index!=-1);
            }
            c_file.get_line(c_line.cached_line_index); //invalidates c
            assert(!c_line.cached_value.empty());
        }
    }
    
    weak_ptr<file> create_file(const string& name) {
        int id=generate_id();
        assert(d_files_by_name.insert(make_pair(name, id)).second);
        
        int log_index=(override_log_index!=-1)? override_log_index : d_log_entries->allocate(log_entry_create_file(id, name));
        override_log_index=-1;
        
        auto res_ptr=make_shared<file>(id, log_index, d_search_index, d_uncached_lines);
        res_ptr->d_ptr=res_ptr;
        weak_ptr<file> res=res_ptr;
        
        assert(d_files.insert(make_pair(id, res_ptr)).second);
        assign_character_by_id( id, &*(res_ptr->characters().front().iter) );
        
        res_ptr->name=name;
        
        d_last_modified_log_index=log_index;
        d_last_viewed_log_index=log_index;
        return res;
    }
    
    void move_file(weak_ptr<file> target_weak, const string& new_name) {
        auto target=target_weak.lock();
        
        auto i=d_files_by_name.find(target->name);
        assert(i!=d_files_by_name.end() && i->second==target->character_id());
        d_files_by_name.erase(i);
        
        assert(d_files_by_name.insert(make_pair(new_name, target->character_id())).second);
        
        int log_index=(override_log_index!=-1)? override_log_index : d_log_entries->allocate(log_entry_move_file(target->character_id(), new_name));
        override_log_index=-1;
        
        target->name=new_name;
        
        d_last_modified_log_index=log_index;
        d_last_viewed_log_index=log_index;
    }
    
    void delete_file(weak_ptr<file> target_weak) {
        auto target=target_weak.lock();
        
        auto i=d_files_by_name.find(target->name);
        assert(i!=d_files_by_name.end() && i->second==target->character_id());
        d_files_by_name.erase(i);
        
        int log_index=(override_log_index!=-1)? override_log_index : d_log_entries->allocate(log_entry_delete_file(target->character_id()));
        override_log_index=-1;
        
        target->remove_uncached_lines();
        
        for (auto c=target->characters().begin();c!=target->characters().end();++c) {
            assign_character_by_id(c->character_id(), nullptr, &*(c->iter));
        }
        
        assert(d_files.erase(target->character_id()));
        
        d_last_modified_log_index=log_index;
        d_last_viewed_log_index=log_index;
    }
    
    weak_ptr<file> get_file(int character_id) const {
        return d_files.at(character_id);
    }
    
    weak_ptr<file> find_file(int character_id) const {
        auto i=d_files.find(character_id);
        if (i==d_files.end()) {
            return weak_ptr<file>();
        } else {
            return i->second;
        }
    }
    
    const map<string, int>& files_by_name() const {
        return d_files_by_name;
    }
    
    const character& insert_character(const character& previous, char value) {
        int id=generate_id();
        
        int log_index=(override_log_index!=-1)? override_log_index : d_log_entries->allocate(log_entry_insert(id, previous.character_id(), value));
        override_log_index=-1;
        
        auto& res=previous.parent->insert_character(previous, id, value, log_index);
        assign_character_by_id(id, &res);
        
        d_last_modified_log_index=log_index;
        d_last_viewed_log_index=log_index;
        return res;
    }
    
    void delete_character(const character& delete_target) {
        int log_index=(override_log_index!=-1)? override_log_index : d_log_entries->allocate(log_entry_delete(delete_target.character_id()));
        override_log_index=-1;
        
        delete_target.iter->add_operation(log_index, true);
        
        auto merge_target_iter=delete_target.iter;
        assert(merge_target_iter!=delete_target.parent->characters().begin());
        --merge_target_iter;
        
        auto& merge_target=*merge_target_iter;
        
        for (auto c=delete_target.merged_character_ids().begin();c!=delete_target.merged_character_ids().end();++c) {
            assign_character_by_id(*c, &*(merge_target.iter), &*(delete_target.iter));
        }
        assign_character_by_id(delete_target.character_id(), &*(merge_target.iter), &*(delete_target.iter));
        
        delete_target.parent->delete_character(delete_target, merge_target, log_index);
        
        d_last_modified_log_index=log_index;
        d_last_viewed_log_index=log_index;
    }
    
    void select_character(const character& target) {
        int log_index=(override_log_index!=-1)? override_log_index : d_log_entries->allocate(log_entry_select(target.character_id()));
        override_log_index=-1;
        
        target.parent->select_character(target, log_index);
        target.iter->add_operation(log_index, false);
        
        d_last_viewed_log_index=log_index;
    }
};


}
