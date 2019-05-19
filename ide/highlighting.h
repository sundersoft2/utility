class highlighting {
    public:
    class file_cache_entry {
        friend class highlighting;
        map<int /*line number*/, vector<bool>> lines;
        weak_ptr<history::file> parent;
    };
    
    private:
    vector<pair<int /*start character id*/, int /*num characters*/>> source;
    map<weak_ptr<history::file>, shared_ptr<file_cache_entry>, owner_less<weak_ptr<history::file>>> cache;
    history::state& s;
    int last_modified_log_index=-1;
    
    void purge_cache() {
        for (auto c=cache.begin();c!=cache.end();) {
            auto c_next=c;
            ++c_next;
            
            if (c->first.lock()==nullptr) {
                cache.erase(c);
            } else {
                c->second->lines.clear();
            }
            
            c=c_next;
        }
    }
    
    public:
    highlighting(history::state& t_s) : s(t_s) {}
    
    void bind(vector<pair<int /*start character id*/, int /*num characters*/>>&& t_source) {
        if (&source!=&t_source) { //gcc bug?
            source=move(t_source);
        }
        
        purge_cache();
        
        for (auto c=source.begin();c!=source.end();++c) {
            auto c_character=s.character_by_id(c->first);
            if (c_character==nullptr) {
                continue;
            }
            
            auto& c_file=*(c_character->parent);
            int start_column_index;
            int line_index=c_file.line_index(*c_character, &start_column_index);
            
            assert(c->second>=0);
            int end_column_index=start_column_index+c->second;
            
            auto c_entry=lookup_file(c_file.d_ptr);
            assert(c_entry!=nullptr);
            
            auto& c_characters=c_entry->lines[line_index];
            if (c_characters.size()<end_column_index) {
                c_characters.resize(end_column_index);
            }
            
            for (int x=start_column_index;x<end_column_index;++x) {
                c_characters[x]=true;
            }
        }
        
        last_modified_log_index=s.last_modified_log_index();
    }
    
    void bind(const vector<pair<string, vector<pair<int /*start character id*/, int /*num characters*/>>>>& t_source) {
        source.clear();
        
        for (auto c=t_source.begin();c!=t_source.end();++c) {
            for (auto d=c->second.begin();d!=c->second.end();++d) {
                source.push_back(*d);
            }
        }
        
        bind(move(source));
    }
    
    void clear() {
        source.clear();
        bind(move(source));
    }
    
    void refresh() {
        if (last_modified_log_index!=s.last_modified_log_index()) {
            bind(move(source));
        }
    }
    
    shared_ptr<file_cache_entry> lookup_file(weak_ptr<history::file> f_weak) {
        auto f=f_weak.lock();
        if (f!=nullptr) {
            auto& res=cache[f];
            if (res==nullptr) {
                res=make_shared<file_cache_entry>();
                res->parent=f;
            }
            return res;
        } else {
            return nullptr;
        }
    }
    
    //result invalidated after bind is called
    const vector<bool>* lookup_line(shared_ptr<file_cache_entry> entry, int line_number) {
        if (entry==nullptr) {
            return nullptr;
        }
        
        auto res=entry->lines.find(line_number);
        if (res!=entry->lines.end()) {
            return &(res->second);
        } else {
            return nullptr;
        }
    }
};
