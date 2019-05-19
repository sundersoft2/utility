namespace history {


struct analysis_group {
    int character_id; //of the newline in the first line
    int num_lines;
    int age; //minimum
    
    //internal data, reset before analysis_group is returned
    file* c_file;
    int start_line;
    void clear_internal() {
        c_file=nullptr;
        start_line=-1;
    }
    
    analysis_group() : character_id(-1), num_lines(0), age(half_int_max), c_file(nullptr), start_line(-1) {}
};

struct analysis_results {
    vector<analysis_group> groups;
    int last_log_index;
    bool is_files;
    
    analysis_results() : last_log_index(-1), is_files(false) {}
};

void visualize(const state& s, const analysis_results& a, temporary_file_reference& res, function<void(int, int)> select_characters, bool clear=true) {
    if (clear) {
        res.clear();
    }
    
    file* previous_file=nullptr;
    
    for (auto c=a.groups.begin();c!=a.groups.end();++c) {
        auto c_character=s.character_by_id(c->character_id);
        
        if (c_character==nullptr) {
            continue;
        }
        
        auto& c_file=*(c_character->parent);
        int start_line_index=c_file.line_index(*c_character);
        
        vector<temporary_file_reference::push_back_data> group_data;
        
        if (a.is_files) {
            group_data.emplace_back("\n"+ c_file.name);
        } else
        if (previous_file!=&c_file) {
            group_data.emplace_back("\n============ "+ c_file.name +" :: "+ to_string(start_line_index+1)+ " ============");
        } else {
            group_data.emplace_back("\n--   --   -- "+ c_file.name +" :: "+ to_string(start_line_index+1)+ " --   --   --");
        }
        
        for (int line_offset=0;line_offset<c->num_lines;++line_offset) {
            int line_index=start_line_index+line_offset;
            if (line_index<c_file.num_lines()) {
                auto& l=c_file.get_line(line_index);
                group_data.emplace_back(l.cached_value, l.cached_tokens);
            }
        }
        
        int id_start=c->character_id;
        int id_end;
        if (start_line_index+c->num_lines>=c_file.num_lines()) {
            id_end=c_file.characters().rbegin()->character_id();
        } else {
            id_end=c_file.get_line(start_line_index+c->num_lines).iter->character_id();
        }
        
        res.push_back_group(move(group_data), [id_start, id_end, select_characters](){ select_characters(id_end, id_start); });
        
        previous_file=&c_file;
    }
}

analysis_results analyze_files(const state& s, bool measure_views) {
    analysis_results res;
    res.last_log_index=(measure_views)? s.last_viewed_log_index() : s.last_modified_log_index();
    res.is_files=true;
    
    for (auto c=s.files_by_name().begin();c!=s.files_by_name().end();++c) {
        auto c_file=s.get_file(c->second).lock();
        
        analysis_group g;
        g.character_id=c->second;
        g.num_lines=0;
        g.age=s.last_viewed_log_index()-((measure_views)? c_file->last_viewed_log_index() : c_file->last_modified_log_index());
        
        res.groups.push_back(g);
    }
    
    sort(res.groups.begin(), res.groups.end(), [&](const analysis_group& a, const analysis_group& b) { return a.age<b.age; });
    
    return move(res);
}

struct analyze_line_state {
    int age;
    bool is_padding;
    
    analyze_line_state() : age(half_int_max), is_padding(false) {}
};

analysis_results generate_analysis_results(
    const state& s,
    function<int(int /*index*/)> generator /*returns character id; return -1 to stop, -2 to skip*/,
    int last_log_index=-1
) {
    const int max_groups=100;
    const int min_group_padding=2; //added to each side of a line
    const int max_group_size=12;
    const double max_group_age_difference=200;
    
    map<file*, map<int, analyze_line_state>> line_states;
    
    //figure out the line ages
    int n_adjusted=0;
    for (int n_raw=0;;++n_raw) {
        int c_character_id=generator(n_raw);
        
        if (c_character_id==-1) {
            break;
        } else
        if (c_character_id==-2) {
            continue;
        }
        
        auto c_character=s.character_by_id(c_character_id);
        if (c_character==nullptr) {
            continue;
        }
        
        auto previous_age=line_states[c_character->parent].insert(make_pair( c_character->parent->line_index(*c_character), analyze_line_state() ));
        if (previous_age.second) {
            previous_age.first->second.age=n_adjusted;
        }
        //if the line was already in the map, its age is less than n_adjusted, so don't do anything
        
        ++n_adjusted;
    }
    
    //add padding
    for (auto c_file=line_states.begin();c_file!=line_states.end();++c_file) {
        for (auto c_line=c_file->second.begin();c_line!=c_file->second.end();++c_line) {
            if (c_line->second.is_padding) {
                continue;
            }
            
            int y_center=c_line->first;
            int y_start=max(y_center-min_group_padding, 0);
            int y_end=min(y_center+min_group_padding, c_file->first->num_lines());
            
            for (int y=y_start;y<y_end;++y) {
                if (y==y_center) {
                    continue;
                }
                
                auto i=c_file->second.insert(make_pair( y, analyze_line_state() ));
                if (i.second) {
                    i.first->second.age=c_line->second.age;
                    i.first->second.is_padding=true;
                }
            }
        }
    }
    
    analysis_results res;
    res.last_log_index=last_log_index;
    res.is_files=false;
    
    //generate groups
    for (auto c_file=line_states.begin();c_file!=line_states.end();++c_file) {
        bool current_group_valid=false;
        analysis_group current_group;
        
        auto finish_group=[&]() {
            if (current_group_valid) {
                current_group.c_file=c_file->first;
                res.groups.push_back(current_group);
            }
            
            current_group_valid=false;
            current_group=analysis_group();
        };
        
        for (auto c_line=c_file->second.begin();c_line!=c_file->second.end();++c_line) {
            
            if (
                !current_group_valid ||
                c_line->first!=(current_group.start_line+current_group.num_lines) ||
                current_group.num_lines>=max_group_size ||
                abs(double(current_group.age)-double(c_line->second.age))>max_group_age_difference
            ) {
                finish_group();
                current_group_valid=true;
                current_group.start_line=c_line->first;
            }
            
            ++current_group.num_lines;
            current_group.age=min(current_group.age, c_line->second.age);
        }
        
        finish_group();
    }
    
    //assign character_id for the groups. also sort and reduce size of output if necessary
    for (auto c=res.groups.begin();c!=res.groups.end();++c) {
        c->character_id=c->c_file->get_line(c->start_line).iter->character_id();
        c->clear_internal();
    }
    
    sort(res.groups.begin(), res.groups.end(), [&](const analysis_group& a, const analysis_group& b) { return a.age<b.age; });
    if (res.groups.size()>max_groups) {
        res.groups.resize(max_groups);
    }
    
    return move(res);
}

analysis_results analyze_lines(const state& s, bool measure_views) {
    const int max_log_entries=2000;
    
    auto& log_entries=s.log_entries();
    
    return generate_analysis_results(s, [&](int n_raw) {
        if (n_raw>=log_entries.size() || n_raw>=max_log_entries) {
            return -1;
        }
        
        auto& c_log_entry=log_entries[log_entries.size()-1-n_raw];
        
        if ((measure_views || c_log_entry.is_write()) && !c_log_entry.is_file_level()) {
            return c_log_entry.character_id();
        } else {
            return -2;
        }
    }, (measure_views)? s.last_viewed_log_index() : s.last_modified_log_index());
}


}
