namespace history {
    class character;
    
    int get_character_id(const character* c);
    char get_character_value(const character* c);
}

namespace search {


//not case sensitive
//note: having a really long identifier will fuck things up because the storage
//space grows quadratically with the identifier size. can add some limit to how long
//an identifier can be before adding it in case there is some thousand character line
//with one identifier on it for some god forsaken reason. won't bother with this for now
//could partition long identifiers into smaller ones but can't be fucked; waste of time
template<class data_type> class identifier_index {
    public:
    class entry;
    
    private:
    struct index_value {
        safe_list<shared_ptr<entry>> entries;
    };
    
    struct index_reference {
        typename map<string, index_value>::iterator map_iterator;
        typename safe_list<shared_ptr<entry>>::iterator list_iterator;
    };
    
    public:
    class entry : disable_copy {
        friend class identifier_index;
        bool d_valid;
        string d_value;
        data_type d_data;
        vector<index_reference> d_index_references;
        typename map<string, shared_ptr<entry>>::iterator d_entries_reference;
        mutable bool mark=false;
        
        public:
        const string& value() const {
            return d_value;
        }
        
        data_type& data() {
            return d_data;
        }
        
        const data_type& data() const {
            return d_data;
        }
    };
    
    private:
    map<string, index_value> index;
    map<string, shared_ptr<entry>> entries;
    
    public:
    pair<shared_ptr<entry>, bool> add(string& value, int max_length=-1) {
        convert_string(value);
        if (value.size()>=max_length) {
            value.resize(max_length);
        }
        
        {
            auto existing=entries.find(value);
            if (existing!=entries.end()) {
                return make_pair(existing->second, false);
            }
        }
        
        auto res=make_shared<entry>();
        res->d_value=value;
        
        for (int pos=0;pos<value.size();++pos) {
            auto map_iter=index.insert(make_pair(value.substr(pos), index_value())).first;
            auto& e=map_iter->second.entries;
            e.push_front(res);
            
            index_reference r;
            r.map_iterator=map_iter;
            r.list_iterator=e.begin();
            res->d_index_references.push_back(r);
        }
        
        {
            auto e=entries.insert(make_pair(value, res));
            assert(e.second);
            res->d_entries_reference=e.first;
        }
        
        res->d_valid=true;
        return make_pair(res, true);
    }
    
    static bool valid_char(char c) {
        return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_';
    }
    
    static void convert_string(string& s) {
        assert(s.size()>=1);
        
        tolower(s);
        for (auto d=s.begin();d!=s.end();++d) {
            assert(valid_char(*d));
        }
    }
    
    //expects no other references to the entry
    void remove(shared_ptr<entry>& e) {
        assert(e->d_valid);
        
        for (auto c=e->d_index_references.begin();c!=e->d_index_references.end();++c) {
            c->map_iterator->second.entries.erase(c->list_iterator);
            if (c->map_iterator->second.entries.empty()) {
                index.erase(c->map_iterator);
            }
        }
        
        entries.erase(e->d_entries_reference);
        
        e->d_valid=false;
        assert(e.unique());
    }
    
    vector<shared_ptr<entry>> lookup(vector<string>& fragments, bool wildcard_start, bool wildcard_end, int max_results=-1, int max_iterations=-1) const {
        assert(fragments.size()>=1);
        
        for (auto c=fragments.begin();c!=fragments.end();++c) {
            convert_string(*c);
        }
        
        boost::regex r; {
            string r_str="^";
            if (wildcard_start) {
                r_str+=".*";
            }
            
            for (int fragment_index=0;fragment_index<fragments.size();++fragment_index) {
                auto& c=fragments[fragment_index];
                
                if (fragment_index!=0) {
                    r_str+=".*";
                }
                r_str+=c;
            }
            
            if (wildcard_end) {
                r_str+=".*";
            }
            r_str+="$";
            
            //cerr << r_str << "\n";
            r=boost::regex(r_str);
        }
        
        int biggest_fragment_index=-1;
        int biggest_fragment_size=-1;
        for (int fragment_index=0;fragment_index<fragments.size();++fragment_index) {
            auto& c=fragments[fragment_index];
            if (biggest_fragment_index==-1 || c.size()>biggest_fragment_size) {
                biggest_fragment_index=fragment_index;
                biggest_fragment_size=c.size();
            }
        }
        
        //cerr << biggest_fragment_index << "\n";
        auto& biggest_fragment=fragments[biggest_fragment_index];
        
        typename map<string, index_value>::const_iterator biggest_fragment_start;
        typename map<string, index_value>::const_iterator biggest_fragment_end;
        if (biggest_fragment_index==fragments.size()-1 && !wildcard_end) {
            biggest_fragment_start=index.find(biggest_fragment);
            biggest_fragment_end=biggest_fragment_start;
            if (biggest_fragment_end!=index.end()) {
                ++biggest_fragment_end;
            }
        } else {
            biggest_fragment_start=index.lower_bound(biggest_fragment);
            
            auto& last_char=biggest_fragment.at(biggest_fragment.size()-1);
            assert(last_char!=numeric_limits<char>::max());
            ++last_char;
            
            biggest_fragment_end=index.lower_bound(biggest_fragment);
            
            --last_char;
        }
        
        vector<shared_ptr<entry>> res;
        
        int num_iterations=0;
        bool done=false;
        for (auto map_i=biggest_fragment_start;map_i!=biggest_fragment_end;++map_i) {
            for (auto entry_i=map_i->second.entries.begin();entry_i!=map_i->second.entries.end();++entry_i) {
                //cerr << (*entry_i)->d_value << "\n";
                if (!(*entry_i)->mark && boost::regex_match((*entry_i)->d_value, r)) {
                    (*entry_i)->mark=true; //can have duplicates if the biggest fragment is found multiple times in a keyword
                    res.push_back(*entry_i);
                    if (max_results!=-1 && res.size()>=max_results) {
                        done=true;
                        break;
                    }
                }
                
                ++num_iterations;
                if (max_iterations!=-1 && num_iterations>=max_iterations) {
                    done=true;
                    break;
                }
            }
            
            if (done) {
                break;
            }
        }
        
        for (auto c=res.begin();c!=res.end();++c) {
            (*c)->mark=false;
        }
        
        //doesn't need to be sorted
        //sort(res.begin(), res.end(), [&](shared_ptr<entry>& a, shared_ptr<entry>& b){ return a->value()<b->value(); });
        
        return res;
    }
};

class line_index {
    struct token_data_identifier {
        int start_character_id=-1;
        int num_characters=-1;
    };
    
    struct identifier_data {
        int reference_count=0;
        safe_list<weak_ptr<identifier_index<identifier_data>::entry>>::iterator unused_keywords_iterator;
        safe_list<token_data_identifier> tokens;
    };
    
    struct token_data_line : public token_data_identifier {
        weak_ptr<identifier_index<identifier_data>::entry> index_entry;
        safe_list<token_data_identifier>::iterator tokens_iterator;
    };
    
    public:
    class line_data : disable_copy {
        friend class line_index;
        vector<token_data_line> d_tokens;
    };
    
    private:
    identifier_index<identifier_data> index;
    safe_list<weak_ptr<identifier_index<identifier_data>::entry>> unused_keywords;
    int num_unused_keywords=0;
    
    void purge_unused_keywords() {
        while (num_unused_keywords>max_unused_keywords) {
            assert(!unused_keywords.empty());
            auto c=unused_keywords.front().lock();
            assert(c->data().reference_count==0);
            assert(c->data().unused_keywords_iterator==unused_keywords.begin());
            assert(c->data().tokens.empty());
            
            index.remove(c);
            
            unused_keywords.erase(unused_keywords.begin());
            --num_unused_keywords;
        }
    }
    
    public:
    const int max_unused_keywords=10; //set to a low number for more testing; could be set higher if performance is an issue
    
    void clear_line(line_data& data) {
        for (auto c=data.d_tokens.begin();c!=data.d_tokens.end();++c) {
            auto v=c->index_entry.lock(); //shouldn't be null
            
            v->data().tokens.erase(c->tokens_iterator);
            
            --v->data().reference_count;
            assert(v->data().reference_count>=0);
            if (v->data().reference_count==0) {
                unused_keywords.push_front(v);
                ++num_unused_keywords;
                assert(v->data().unused_keywords_iterator==unused_keywords.end());
                v->data().unused_keywords_iterator=unused_keywords.begin();
            }
        }
        data.d_tokens.clear();
        
        purge_unused_keywords();
    }
    
    void set_line(line_data& data, const vector<history::character*>& value, const vector<s8>& tokens, int max_identifier_length=-1) {
        //cerr << "baz!!!\n";
        clear_line(data);
        
        string buffer;
        int start_character_id=-1;
        bool in_identifier=false;
        for (int x=0;x<=value.size();++x) {
            if (in_identifier) {
                if (x==value.size() || tokens[x]!=-history::tokenizer::token_type_identifier) {
                    //cerr << "add " << buffer << "\n";
                    auto index_entry=index.add(buffer, max_identifier_length);
                    ++index_entry.first->data().reference_count;
                    
                    if (index_entry.second) {
                        index_entry.first->data().unused_keywords_iterator=unused_keywords.end();
                    } else {
                        if (index_entry.first->data().unused_keywords_iterator!=unused_keywords.end()) {
                            unused_keywords.erase(index_entry.first->data().unused_keywords_iterator);
                            index_entry.first->data().unused_keywords_iterator=unused_keywords.end();
                            --num_unused_keywords;
                            assert(num_unused_keywords>=0);
                        }
                    }
                    
                    token_data_line d;
                    d.start_character_id=start_character_id;
                    d.num_characters=buffer.size();
                    
                    index_entry.first->data().tokens.push_front(d);
                    
                    d.index_entry=index_entry.first;
                    d.tokens_iterator=index_entry.first->data().tokens.begin();
                    data.d_tokens.push_back(move(d));
                    
                    buffer.clear();
                    start_character_id=-1;
                    in_identifier=false;
                } else {
                    buffer+=history::get_character_value(value[x]);
                }
            } else {
                if (x<value.size() && tokens[x]==history::tokenizer::token_type_identifier) {
                    in_identifier=true;
                    buffer+=history::get_character_value(value[x]);
                    start_character_id=history::get_character_id(value[x]);
                }
            }
        }
        
        assert(buffer.empty() && start_character_id==-1 && !in_identifier);
    }
    
    vector<pair<string, vector<pair<int /*start character id*/, int /*num characters*/>>>> lookup(
        vector<string>& fragments, bool wildcard_start, bool wildcard_end, int max_identifier_results=-1, int max_iterations=-1, int max_token_results=-1
    ) const {
        vector<pair<string, vector<pair<int, int>>>> res;
        
        auto entries=index.lookup(fragments, wildcard_start, wildcard_end, max_identifier_results, max_iterations);
        //cerr << entries.size() << " " << fragments[0] << " " << fragments.size() << " " << wildcard_start << " " << wildcard_end << " FFF\n";
        
        int num_results=0;
        
        bool done=false;
        for (auto c=entries.begin();c!=entries.end();++c) {
            //cerr << (*c)->value() << "\n";
            auto tokens=(*c)->data().tokens;
            
            if (tokens.empty()) {
                continue;
            }
            
            res.push_back(make_pair( (*c)->value(), vector<pair<int, int>>() ));
            
            for (auto d=tokens.begin();d!=tokens.end();++d) {
                res.back().second.push_back(make_pair(d->start_character_id, d->num_characters));
                
                ++num_results;
                if (num_results>=max_token_results) {
                    done=true;
                    break;
                }
            }
            
            if (done) {
                break;
            }
        }
        
        return res;
    }
    
    vector<pair<string, vector<pair<int /*start character id*/, int /*num characters*/>>>> lookup(
        string query, int max_identifier_results=-1, int max_iterations=-1, int max_token_results=-1
    ) const {
        vector<string> fragments;
        bool wildcard_start=false;
        bool wildcard_end=false;
        
        string buffer;
        bool found_non_space=false;
        for (int x=0;x<query.size();++x) {
            char c=query[x];
            if (identifier_index<identifier_data>::valid_char(c)) {
                buffer+=c;
                found_non_space=false;
            } else
            if (!buffer.empty()) {
                fragments.push_back(move(buffer));
                buffer.clear();
                found_non_space=false;
            }
            
            if (!identifier_index<identifier_data>::valid_char(c) && c!=' ') {
                found_non_space=true;
                if (fragments.empty()) {
                    wildcard_start=true;
                }
            }
        }
        
        if (!buffer.empty()) {
            fragments.push_back(move(buffer));
            buffer.clear();
        }
        
        if (found_non_space) {
            wildcard_end=true;
        }
        
        if (fragments.empty()) {
            return vector<pair<string, vector<pair<int, int>>>>();
        } else {
            return lookup(fragments, wildcard_start, wildcard_end, max_identifier_results, max_iterations, max_token_results);
        }
    }
};


}
