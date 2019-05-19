namespace history { namespace sync {


struct disk_scan_settings {
    string root_path;
    set<string> valid_extensions;
    set<string> ignored_directory_names; //does not include path
};

struct state_common {
    map<string, sha1> files;
    map<sha1, set<string>> files_by_hash;
    
    void remove_hash(const string& name, const sha1& hash) {
        auto i_hash=files_by_hash.find(hash);
        assert(i_hash!=files_by_hash.end());
        assert(i_hash->second.erase(name));
        if (i_hash->second.empty()) {
            files_by_hash.erase((map<sha1, set<string>>::const_iterator)i_hash);
        }
    }
    
    bool remove(const string& name) {
        auto i=files.find(name);
        if (i!=files.end()) {
            remove_hash(name, i->second);
            files.erase(i);
            return true;
        } else {
            return false;
        }
    }
    
    //returns old hash if file existed. if file is new, returns new hash
    sha1 add(const string& name, const sha1& hash, bool allow_new, bool *added=nullptr) {
        sha1 res;
        
        if (added!=nullptr) {
            *added=false;
        }
        
        auto i=files.find(name);
        if (i!=files.end()) {
            remove_hash(name, i->second);
            res=i->second;
            i->second=hash;
        } else {
            assert(allow_new);
            
            if (added!=nullptr) {
                *added=true;
            }
            
            assert(files.insert(make_pair(name, hash)).second);
            res=hash;
        }
        
        assert(files_by_hash[hash].insert(name).second);
        
        return res;
    }
    
    void clear() {
        files.clear();
        files_by_hash.clear();
    }
};

struct memory_state : public state_common {
    int last_modified_log_index;
    map<string, sha1> expected_disk_files;
    
    memory_state() : last_modified_log_index(-1) {}
};

struct disk_state : public state_common {
    map<string /*new*/, string /*old*/> moved_files;
};

const char state_delta_mode_invalid=0; //used if conflicts were partially resolved
const char state_delta_mode_disk_only=1;
const char state_delta_mode_memory_only=2;
const char state_delta_mode_changed=3;
const char state_delta_mode_stale=4; //used to indicate that a disk file can be safely overwritten by a memory file
struct state_delta {
    char mode;
    string name;
    sha1 disk_hash;
    sha1 memory_hash;
    string moved_name; //if mode is disk_only, this is the old name of the file (in memory). if mode is memory_only, this is the new name (on disk).
    
    state_delta() : mode(state_delta_mode_invalid) {}
    state_delta(char t_mode, const string& t_name, sha1 t_disk_hash, sha1 t_memory_hash, const string& t_moved_name=string()) : mode(t_mode), name(t_name), disk_hash(t_disk_hash), memory_hash(t_memory_hash), moved_name(t_moved_name) {}
};

//if files is empty, will scan every file and returns true
//otherwise, returns false if it scanned every file because some of them were deleted
bool scan_disk(const disk_scan_settings& settings, disk_state& data, const vector<string>& files) {
    disk_state old_data;
    
    string file_buffer;
    
    boost::filesystem::path root_path=settings.root_path;
    
    auto scan_file=[&](const boost::filesystem::path& c) {
        string extension=c.extension().string();
        if (extension.empty()) {
            extension=c.filename().string(); //e.g. SConstruct
        } else {
            extension=extension.substr(1);
        }
        
        if ( settings.valid_extensions.count(extension) ) {
            boost::filesystem::path relative_path; {
                auto r=mismatch(c.begin(), c.end(), root_path.begin());
                assert(r.second==root_path.end() || r.second->string()==".");
                for (;r.first!=c.end();++r.first) {
                    relative_path/=*(r.first);
                }
            }
            assert(relative_path.is_relative());
            
            ifstream in(c.string(), ios::binary);
            getstream(in, 1024, &file_buffer);
            
            auto new_hash=sha1(file_buffer);
            
            //cerr << "scan_disk " << relative_path.string() << " || " << new_hash.to_string() << "\n";
            
            bool added;
            data.add(relative_path.string(), new_hash, files.empty(), &added); //can only have new files if files input is empty
            if (added) {
                auto i=old_data.files_by_hash.find(new_hash);
                if (i!=old_data.files_by_hash.end() && !i->second.empty()) {
                    string old_file=*(i->second.begin()); //if there are multiple files with the same hash, pick one of them
                    
                    auto double_moved_i=old_data.moved_files.find(old_file); //if a file got moved multiple times between scans, use the oldest name
                    if (double_moved_i!=old_data.moved_files.end()) {
                        old_file=(double_moved_i->second);
                        old_data.moved_files.erase(double_moved_i);
                    }
                    
                    if (relative_path.string()!=old_file) { //possible to move a file, scan disk, and then move it back, scan disk, then import disk -> memory
                        assert(data.moved_files.insert(make_pair(relative_path.string(), old_file)).second);
                    }
                    
                    i->second.erase(i->second.begin()); //make sure the same file isn't used as a source for two move operations
                }
            }
        }
    };
    
    function<void(const boost::filesystem::path& path)> scan_directory=[&](const boost::filesystem::path& path) {
        for (auto c=boost::filesystem::directory_iterator(path);c!=boost::filesystem::directory_iterator();++c) {
            assert(c->path().has_filename());
            
            if (is_directory(c->status())) {
                if ( !settings.ignored_directory_names.count(c->path().filename().string()) ) {
                    scan_directory(c->path());
                }
            } else
            if (is_regular_file(c->status())) {
                scan_file(c->path());
            }
        }
    };
    
    if (files.empty()) {
        old_data=move(data);
        data.clear(); //get rid of deleted files
        data.moved_files.clear();
        scan_directory(root_path);
        
        for (auto c=old_data.moved_files.begin();c!=old_data.moved_files.end();++c) {
            //if a file was moved twice, it already got removed from this map
            //if a file was moved and then deleted, need to skip it
            //if a file was moved and then either changed or left alone, need to add it to data.moved_files
            
            if (data.files.count(c->first)) {
                assert(data.moved_files.insert(make_pair(c->first, c->second)).second);
            }
        }
        
        return true;
    } else {
        for (auto c=files.begin();c!=files.end();++c) {
            boost::filesystem::path path=root_path;
            path/=*c;
            if (!boost::filesystem::exists(path)) {
                return false;
            }
        }
        
        for (auto c=files.begin();c!=files.end();++c) {
            boost::filesystem::path path=root_path;
            path/=*c;
            scan_file(path);
        }
        
        return true;
    }
}

void scan_memory(const history::state& s, memory_state& data, bool allow_new=false) {
    //make sure there are no deleted files, since files cannot be deleted from memory without updating memory_state
    for (auto c=data.files.begin();c!=data.files.end();++c) {
        assert(s.files_by_name().count(c->first));
    }
    
    //find all changed files. there can't be any new files except during startup
    for (auto c=s.files_by_name().begin();c!=s.files_by_name().end();++c) {
        auto c_file=s.get_file(c->second).lock();
        if (c_file->last_modified_log_index()<data.last_modified_log_index) {
            continue; //can't be new and couldn't have changed
        }
        
        int line=0;
        sha1 new_hash([&]() -> pair<const char*, int> {
            if (line<c_file->num_lines()) {
                auto& str=c_file->get_line(line).cached_value;
                auto res=make_pair( &(str[(line==0)? 1 : 0]), ((int)str.size()) - ((line==0)? 1 : 0) );
                ++line;
                return res;
            } else {
                return make_pair(nullptr, 0);
            }
        });
        
        //cerr << "scan_memory " << c->first << " || " << new_hash.to_string() << "\n";
        
        bool added;
        data.add(c->first, new_hash, allow_new, &added);
        if (added) {
            assert(data.expected_disk_files.insert(make_pair(c->first, new_hash)).second);
        }
    }
    
    data.last_modified_log_index=s.last_modified_log_index();
}

//will create a new file if necessary
void write_file_disk(
    const history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    disk_state& disk_data,
    const string& file_name
) {
    shared_ptr<const file> c_file; {
        auto i=s.files_by_name().find(file_name);
        assert(i!=s.files_by_name().end());
        c_file=s.get_file(i->second).lock();
    }
    
    {
        boost::filesystem::path file_path=disk_settings.root_path +"/"+ file_name;
        boost::filesystem::create_directories(file_path.parent_path());
    }
    
    ofstream out((disk_settings.root_path +"/"+ file_name).c_str(), ios::binary|ios::trunc);
    
    bool is_first=true;
    for (auto c=c_file->characters().begin();c!=c_file->characters().end();++c) {
        if (!is_first) {
            out.put(c->value());
        }
        is_first=false;
    }
    
    assert(out.good());
    
    {
        auto i=memory_data.files.find(file_name);
        assert(i!=memory_data.files.end());
        disk_data.add(file_name, i->second, true);
        
        auto expected_i=memory_data.expected_disk_files.find(file_name);
        assert(expected_i!=memory_data.expected_disk_files.end());
        expected_i->second=i->second;
    }
}

void delete_file_disk(
    const history::state& s,
    const disk_scan_settings& disk_settings,
    const memory_state& memory_data,
    disk_state& disk_data,
    const string& file_name
) {
    assert(boost::filesystem::remove(disk_settings.root_path +"/"+ file_name));
    assert(disk_data.remove(file_name));
}

//will create a new file if necessary
void write_file_memory(
    history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    const disk_state& disk_data,
    const string& file_name
) {
    {
        auto i=s.files_by_name().find(file_name);
        if (i!=s.files_by_name().end()) {
            s.delete_file(s.get_file(i->second));
        }
    }
    
    //todo:
    //also need to sanitize this stuff:
    //-any tabs are converted to spaces using the editor tab width (make it global). right now tabs are deleted from the file
    //-any newlines at the end of the file after the first one are removed. a newline is added if the file does not end with a newline. right now nothing happens
    //if the file was sanitized, don't overwrite the disk version since that could cause data loss. just let the user sync memory to disk for now
    
    auto c_file=s.create_file(file_name).lock();
    
    string file_data=getfile(disk_settings.root_path +"/"+ file_name);
    
    int next_newline_index=-1;
    int last_non_space_index=-1;
    for (int x=0;x<file_data.size();++x) {
        if (next_newline_index==-1 || next_newline_index==x) {
            assert(next_newline_index==-1 || file_data[x]=='\n');
            
            for (next_newline_index=x+1;next_newline_index<file_data.size();++next_newline_index) {
                if (file_data[next_newline_index]=='\n') {
                    break;
                }
            }
            
            //get rid of trailing spaces
            for (last_non_space_index=next_newline_index-1;last_non_space_index>x;--last_non_space_index) {
                if (is_ascii_printable(file_data[last_non_space_index])) {
                    break;
                }
            }
        }
        
        assert(next_newline_index>x);
        
        char c=file_data[x];
        if ((c=='\n' || c==' '|| is_ascii_printable(c)) && x<=last_non_space_index) {
            s.insert_character(c_file->characters().back(), c);
        }
    }
    
    {
        auto i=disk_data.files.find(file_name);
        assert(i!=disk_data.files.end());
        memory_data.add(file_name, i->second, true);
        
        memory_data.expected_disk_files[file_name]=i->second;
    }
}

void delete_file_memory(
    history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    const disk_state& disk_data,
    const string& file_name
) {
    auto i=s.files_by_name().find(file_name);
    assert(i!=s.files_by_name().end());
    s.delete_file(s.get_file(i->second));
    
    assert(memory_data.remove(file_name));
    assert(memory_data.expected_disk_files.erase(file_name));
}

void move_file_memory(
    history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    const disk_state& disk_data,
    const string& old_file_name,
    const string& new_file_name
) {
    assert(old_file_name!=new_file_name);
    
    {
        auto i=s.files_by_name().find(old_file_name);
        assert(i!=s.files_by_name().end());
        s.move_file(s.get_file(i->second), new_file_name);
    }
    
    {
        auto hash=memory_data.files.at(old_file_name);
        assert(memory_data.remove(old_file_name));
        
        bool added;
        memory_data.add(new_file_name, hash, true, &added);
        assert(added);
        
        auto expected_hash=memory_data.expected_disk_files.at(old_file_name);
        assert(memory_data.expected_disk_files.erase(old_file_name));
        
        assert(memory_data.expected_disk_files.insert(make_pair(new_file_name, expected_hash)).second);
    }
}

//returns conflicts
vector<state_delta> scan(
    const history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    disk_state& disk_data,
    bool& only_scan_dirty_memory_files //this will get set to false if a full scan had to be done because files were deleted
) {
    scan_memory(s, memory_data);
    
    map<string, sha1 /*old hash*/> changed_memory_files;
    assert(memory_data.expected_disk_files.size()==memory_data.files.size());
    for (auto c=memory_data.expected_disk_files.begin();c!=memory_data.expected_disk_files.end();++c) {
        auto i=memory_data.files.find(c->first);
        assert(i!=memory_data.files.end());
        if (i->second!=c->second) {
            assert(changed_memory_files.insert(make_pair(c->first, i->second)).second);
        }
    }
    
    vector<string> scan_disk_files;
    if (only_scan_dirty_memory_files) {
        for (auto c=changed_memory_files.begin();c!=changed_memory_files.end();++c) {
            scan_disk_files.push_back(c->first);
        }
    }
    
    if (!scan_disk(disk_settings, disk_data, scan_disk_files)) {
        assert(only_scan_dirty_memory_files && !scan_disk_files.empty());
        only_scan_dirty_memory_files=false;
        scan_disk_files.clear();
        assert(scan_disk(disk_settings, disk_data, scan_disk_files));
    }
    
    auto& disk_moved_files_new_to_old=disk_data.moved_files;
    map<string, string> disk_moved_files_old_to_new;
    
    if (!only_scan_dirty_memory_files) {
        for (auto c=disk_moved_files_new_to_old.begin();c!=disk_moved_files_new_to_old.end();++c) {
            assert(disk_moved_files_old_to_new.insert(make_pair(c->second, c->first)).second);
        }
    }
    
    vector<state_delta> res;
    
    for (auto c_disk=disk_data.files.begin();c_disk!=disk_data.files.end();++c_disk) {
        auto c_memory=memory_data.files.find(c_disk->first);
        
        if (c_memory==memory_data.files.end()) {
            string old_moved_name;
            
            if (!only_scan_dirty_memory_files) {
                auto moved_old_i=disk_moved_files_new_to_old.find(c_disk->first);
                if (moved_old_i!=disk_moved_files_new_to_old.end()) {
                    auto memory_old_i=memory_data.files.find(moved_old_i->second);
                    if (memory_old_i!=memory_data.files.end() && memory_old_i->second==c_disk->second) {
                        old_moved_name=moved_old_i->second;
                    }
                }
            }
            
            res.emplace_back(state_delta_mode_disk_only, c_disk->first, c_disk->second, sha1(), old_moved_name);
        } else
        if (c_memory->second!=c_disk->second) {
            auto old_hash=changed_memory_files.find(c_disk->first);
            if (old_hash!=changed_memory_files.end() && old_hash->second==c_disk->second) {
                res.emplace_back(state_delta_mode_stale, c_disk->first, c_disk->second, c_memory->second);
            } else {
                res.emplace_back(state_delta_mode_changed, c_disk->first, c_disk->second, c_memory->second);
            }
            //it's possible for the user to swap two files on disk, which wouldn't get detected as a move. fuck it
        }
    }
    
    for (auto c_memory=memory_data.files.begin();c_memory!=memory_data.files.end();++c_memory) {
        auto c_disk=disk_data.files.find(c_memory->first);
        
        if (c_disk==disk_data.files.end()) {
            string new_moved_name;
            
            if (!only_scan_dirty_memory_files) {
                auto moved_new_i=disk_moved_files_old_to_new.find(c_memory->first);
                if (moved_new_i!=disk_moved_files_old_to_new.end()) {
                    auto disk_new_i=disk_data.files.find(moved_new_i->second);
                    if (disk_new_i!=disk_data.files.end() && disk_new_i->second==c_memory->second) {
                        new_moved_name=moved_new_i->second;
                    }
                }
            }
            
            res.emplace_back(state_delta_mode_memory_only, c_memory->first, sha1(), c_memory->second, new_moved_name);
        }
    }
    
    return move(res);
}

const char resolve_log_entry_mode_invalid=0;
const char resolve_log_entry_mode_delete=1;
const char resolve_log_entry_mode_create=2;
const char resolve_log_entry_mode_overwrite=3;
const char resolve_log_entry_mode_move=4;
struct resolve_log_entry {
    char mode;
    string name;
    bool is_memory;
    string old_name; //for moved files
    
    resolve_log_entry() : mode(resolve_log_entry_mode_invalid), is_memory(false) {}
    resolve_log_entry(char t_mode, const string& t_name, bool t_is_memory, const string& t_old_name=string()) : mode(t_mode), name(t_name), is_memory(t_is_memory), old_name(t_old_name) {}
    
    string to_string(bool dry_run) const {
        string res;
        if (dry_run) {
            res+="OK to ";
        }
        
        if (mode==resolve_log_entry_mode_delete) {
            res+=(dry_run? "delete '" : "Deleted '");
        } else
        if (mode==resolve_log_entry_mode_create) {
            res+=(dry_run? "create '" : "Created '");
        } else
        if (mode==resolve_log_entry_mode_overwrite) {
            res+=(dry_run? "overwrite '" : "Overwrote '");
        } else
        if (mode==resolve_log_entry_mode_move) {
            res+=(dry_run? "move '" : "Moved '")+ old_name +"' to '";
        } else {
            assert(false);
        }
        
        res+=name;
        
        if (dry_run) {
            res+="' ?";
        } else {
            res+="' .";
        }
        
        return res;
    }
};

//returns false if there are unresolved conflicts
pair<vector<resolve_log_entry>, bool> resolve_safe(
    const history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    disk_state& disk_data,
    vector<state_delta>& conflicts
) {
    bool resolved_all=true;
    vector<resolve_log_entry> log;
    
    for (auto c=conflicts.begin();c!=conflicts.end();++c) {
        if (c->mode==state_delta_mode_stale) {
            log.emplace_back(resolve_log_entry_mode_overwrite, c->name, false);
            
            write_file_disk(s, disk_settings, memory_data, disk_data, c->name);
            *c=state_delta();
        } else {
            resolved_all=false;
        }
    }
    
    return make_pair(move(log), resolved_all);
}

vector<resolve_log_entry> resolve_using_disk(
    history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    const disk_state& disk_data,
    vector<state_delta>& conflicts,
    bool dry_run
) {
    vector<resolve_log_entry> log;
    
    for (auto c=conflicts.begin();c!=conflicts.end();++c) {
        assert(c->mode!=state_delta_mode_stale);
        
        if (c->mode==state_delta_mode_invalid) {
            continue;
        }
        
        if (c->mode==state_delta_mode_disk_only) {
            if (c->moved_name.empty()) {
                log.emplace_back(resolve_log_entry_mode_create, c->name, true);
                
                if (!dry_run) {
                    write_file_memory(s, disk_settings, memory_data, disk_data, c->name);
                    *c=state_delta();
                }
            } else {
                log.emplace_back(resolve_log_entry_mode_move, c->name, true, c->moved_name);
                
                if (!dry_run) {
                    move_file_memory(s, disk_settings, memory_data, disk_data, c->moved_name, c->name);
                    *c=state_delta();
                }
            }
        } else
        if (c->mode==state_delta_mode_memory_only) {
            if (c->moved_name.empty()) { //skip old file for moved files
                log.emplace_back(resolve_log_entry_mode_delete, c->name, true);
                
                if (!dry_run) {
                    delete_file_memory(s, disk_settings, memory_data, disk_data, c->name);
                    *c=state_delta();
                }
            }
        } else
        if (c->mode==state_delta_mode_changed) {
            log.emplace_back(resolve_log_entry_mode_overwrite, c->name, true);
            
            if (!dry_run) {
                write_file_memory(s, disk_settings, memory_data, disk_data, c->name);
                *c=state_delta();
            }
        } else {
            assert(false);
        }
    }
    
    return move(log);
}

vector<resolve_log_entry> resolve_using_memory(
    const history::state& s,
    const disk_scan_settings& disk_settings,
    memory_state& memory_data,
    disk_state& disk_data,
    vector<state_delta>& conflicts,
    bool dry_run
) {
    vector<resolve_log_entry> log;
    
    for (auto c=conflicts.begin();c!=conflicts.end();++c) {
        assert(c->mode!=state_delta_mode_stale);
        
        if (c->mode==state_delta_mode_invalid) {
            continue;
        }
        
        if (c->mode==state_delta_mode_disk_only) {
            log.emplace_back(resolve_log_entry_mode_delete, c->name, false);
            
            if (!dry_run) {
                delete_file_disk(s, disk_settings, memory_data, disk_data, c->name);
                *c=state_delta();
            }
        } else
        if (c->mode==state_delta_mode_memory_only) {
            log.emplace_back(resolve_log_entry_mode_create, c->name, false);
            
            if (!dry_run) {
                write_file_disk(s, disk_settings, memory_data, disk_data, c->name);
                *c=state_delta();
            }
        } else
        if (c->mode==state_delta_mode_changed) {
            log.emplace_back(resolve_log_entry_mode_overwrite, c->name, false);
            
            if (!dry_run) {
                write_file_disk(s, disk_settings, memory_data, disk_data, c->name);
                *c=state_delta();
            }
        } else {
            assert(false);
        }
    }
    
    return move(log);
}


}}
