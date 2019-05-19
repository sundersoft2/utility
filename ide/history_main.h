namespace history {


class history_main {
    unique_ptr<state> d_state;
    unique_ptr<persistent_log> d_log;
    sync::disk_scan_settings d_disk_settings;
    sync::memory_state d_memory_state;
    sync::disk_state d_disk_state;
    
    function<void(string&&)> log_info;
    
    void log_conflicts(const vector<sync::resolve_log_entry>& conflicts, bool is_dry_run) {
        for (auto c=conflicts.begin();c!=conflicts.end();++c) {
            log_info(c->to_string(is_dry_run));
        }
    }
    
    public:
    history_main(string root_path, function<void(string&&)> t_log_info) : log_info(t_log_info) {
        const char* data_dir_name="ide_data";
        const char* log_file_name="log";
        
        create_ide_files(root_path, data_dir_name, log_file_name);
        
        d_disk_settings.root_path=root_path;
        d_disk_settings.valid_extensions={"cpp", "h", "c", "hpp", "sconstruct", "sconscript", "cs", "py", "shader", "cginc", "html", "js", "css", "php"};
        d_disk_settings.ignored_directory_names={".hg", ".git", "Library", "library", data_dir_name};
        
        //make sure the data directory and log file exist
        
        d_log=make_unique_ptr<persistent_log>(root_path +"/"+ data_dir_name +"/"+ log_file_name);
        if (d_log->was_corrupted()) {
            log_info("warning: log is corrupted");
        }
        
        d_state=make_unique_ptr<state>(&*d_log);
        
        sync::scan_memory(*d_state, d_memory_state, true);
        sync::scan_disk(d_disk_settings, d_disk_state, vector<string>());
    }
    
    void persist(bool do_full_scan=false) {
        if (!d_log->persist()) {
            log_info("warning: did not save due to corrupted log");
            return;
        }
    }
    
    void sync(bool use_disk, bool use_dry_run, bool only_scan_dirty_memory_files=false) {
        auto conflicts=sync::scan(*d_state, d_disk_settings, d_memory_state, d_disk_state, only_scan_dirty_memory_files);
        auto resolve_safe_res=resolve_safe(*d_state, d_disk_settings, d_memory_state, d_disk_state, conflicts);
        log_conflicts(resolve_safe_res.first, false);
        if (!resolve_safe_res.second) {
            vector<sync::resolve_log_entry> resolve_final_res;
            if (use_disk) {
                resolve_final_res=resolve_using_disk(*d_state, d_disk_settings, d_memory_state, d_disk_state, conflicts, use_dry_run);
            } else {
                resolve_final_res=resolve_using_memory(*d_state, d_disk_settings, d_memory_state, d_disk_state, conflicts, use_dry_run);
            }
            log_conflicts(resolve_final_res, use_dry_run);
        }
    }
    
    state& get_state() {
        return *d_state;
    }
};


}
