namespace history {


const char log_entry_type_insert=1;
const char log_entry_type_delete=2;
const char log_entry_type_create_file=3;
const char log_entry_type_move_file=4;
const char log_entry_type_delete_file=5;
const char log_entry_type_select=6;

struct log_entry_insert {
    int timestamp;
    int character_id;
    int previous_character_id;
    char value;
    
    log_entry_insert() {}
    log_entry_insert(int t_character_id, int t_previous_character_id, char t_value) : timestamp(time(nullptr)), character_id(t_character_id), previous_character_id(t_previous_character_id), value(t_value) {}
};

struct log_entry_delete {
    int timestamp;
    int character_id;
    
    log_entry_delete() {}
    explicit log_entry_delete(int t_character_id) : timestamp(time(nullptr)), character_id(t_character_id) {}
};

struct log_entry_create_file {
    int timestamp;
    int character_id;
    string name;
    
    log_entry_create_file() {}
    log_entry_create_file(int t_character_id, const string& t_name) : timestamp(time(nullptr)), character_id(t_character_id), name(t_name) {}
};

struct log_entry_move_file {
    int timestamp;
    int character_id;
    string name;
    
    log_entry_move_file() {}
    log_entry_move_file(int t_character_id, const string& t_name) : timestamp(time(nullptr)), character_id(t_character_id), name(t_name) {}
};

struct log_entry_delete_file {
    int timestamp;
    int character_id;
    
    log_entry_delete_file() {}
    log_entry_delete_file(int t_character_id) : timestamp(time(nullptr)), character_id(t_character_id) {}
};

struct log_entry_select {
    int timestamp;
    int character_id;
    
    log_entry_select() {}
    explicit log_entry_select(int t_character_id) : timestamp(time(nullptr)), character_id(t_character_id) {}
};

class log_entry {
    char d_type;
    int int_1;
    int int_2;
    int int_3;
    char char_1;
    string string_1;
    
    template<class type> static void read(type& out, const vector<char>& data, int& pos) {
        assert(pos+sizeof(type)<=data.size());
        out=*(const type*)&(data[pos]);
        pos+=sizeof(type);
    }
    
    template<class type> static void write(vector<char>& data, type in) {
        data.insert(data.end(), (char*)&in, ((char*)&in)+sizeof(in));
    }
    
    public:
    log_entry() : d_type(0), int_1(0), int_2(0), int_3(0), char_1(0) {}
    log_entry(const log_entry_insert& t) : d_type(log_entry_type_insert), int_1(t.timestamp), int_2(t.character_id), int_3(t.previous_character_id), char_1(t.value) {}
    log_entry(const log_entry_delete& t) : d_type(log_entry_type_delete), int_1(t.timestamp), int_2(t.character_id), int_3(0), char_1(0) {}
    log_entry(const log_entry_create_file& t) : d_type(log_entry_type_create_file), int_1(t.timestamp), int_2(t.character_id), int_3(0), char_1(0), string_1(t.name) {}
    log_entry(const log_entry_move_file& t) : d_type(log_entry_type_move_file), int_1(t.timestamp), int_2(t.character_id), int_3(0), char_1(0), string_1(t.name) {}
    log_entry(const log_entry_delete_file& t) : d_type(log_entry_type_delete_file), int_1(t.timestamp), int_2(t.character_id), int_3(0), char_1(0) {}
    log_entry(const log_entry_select& t) : d_type(log_entry_type_select), int_1(t.timestamp), int_2(t.character_id), int_3(0), char_1(0) {}
    
    /*
    operator log_entry_insert() const { assert(d_type==log_entry_type_insert); log_entry_insert res; res.timestamp=int_1; res.character_id=int_2; res.previous_character_id=int_3; res.value=char_1; return res; }
    operator log_entry_delete() const { assert(d_type==log_entry_type_delete); log_entry_delete res; res.timestamp=int_1; res.character_id=int_2; return res; }
    operator log_entry_create_file() const { assert(d_type==log_entry_type_create_file); log_entry_create_file res; res.timestamp=int_1; res.character_id=int_2; res.name=string_1; return res; }
    operator log_entry_move_file() const { assert(d_type==log_entry_type_move_file); log_entry_move_file res; res.timestamp=int_1; res.character_id=int_2; res.name=string_1; return res; }
    operator log_entry_delete_file() const { assert(d_type==log_entry_type_delete_file); log_entry_delete_file res; res.timestamp=int_1; res.character_id=int_2; return res; }
    */
    
    static log_entry decode(const vector<char>& data, int& pos) {
        log_entry res;
        
        read(res.d_type, data, pos);
        
        assert(
            res.d_type==log_entry_type_insert ||
            res.d_type==log_entry_type_delete ||
            res.d_type==log_entry_type_create_file ||
            res.d_type==log_entry_type_move_file ||
            res.d_type==log_entry_type_delete_file ||
            res.d_type==log_entry_type_select
        );
        
        read(res.int_1, data, pos);
        read(res.int_2, data, pos);
        
        if (res.d_type==log_entry_type_insert) {
            read(res.int_3, data, pos);
            read(res.char_1, data, pos);
        }
        
        if (res.d_type==log_entry_type_create_file || res.d_type==log_entry_type_move_file) {
            int string_1_size;
            read(string_1_size, data, pos);
            
            assert(pos+string_1_size<=data.size());
            res.string_1=string(data.begin()+pos, data.begin()+pos+string_1_size);
            pos+=string_1_size;
        }
        
        return res;
    }
    
    void encode(vector<char>& data) const {
        assert(
            d_type==log_entry_type_insert ||
            d_type==log_entry_type_delete ||
            d_type==log_entry_type_create_file ||
            d_type==log_entry_type_move_file ||
            d_type==log_entry_type_delete_file ||
            d_type==log_entry_type_select
        );
        
        write(data, d_type);
        
        write(data, int_1);
        write(data, int_2);
        
        if (d_type==log_entry_type_insert) {
            write(data, int_3);
            write(data, char_1);
        }
        
        if (d_type==log_entry_type_create_file || d_type==log_entry_type_move_file) {
            write(data, (int)string_1.size());
            data.insert(data.end(), string_1.begin(), string_1.end());
        }
    }
    
    bool is_write() const {
        return
            d_type==log_entry_type_insert ||
            d_type==log_entry_type_delete ||
            d_type==log_entry_type_create_file ||
            d_type==log_entry_type_move_file ||
            d_type==log_entry_type_delete_file
        ;
    }
    
    bool is_file_level() const {
        return
            d_type==log_entry_type_create_file ||
            d_type==log_entry_type_move_file ||
            d_type==log_entry_type_delete_file
        ;
    }
    
    char type() const {
        return d_type;
    }
    
    int timestamp() const {
        assert(
            d_type==log_entry_type_insert ||
            d_type==log_entry_type_delete ||
            d_type==log_entry_type_create_file ||
            d_type==log_entry_type_move_file ||
            d_type==log_entry_type_delete_file ||
            d_type==log_entry_type_select
        );
        return int_1;
    }
    
    int character_id() const {
        assert(
            d_type==log_entry_type_insert ||
            d_type==log_entry_type_delete ||
            d_type==log_entry_type_create_file ||
            d_type==log_entry_type_move_file ||
            d_type==log_entry_type_delete_file ||
            d_type==log_entry_type_select
        );
        return int_2;
    }
    
    int previous_character_id() const {
        assert(d_type==log_entry_type_insert);
        return int_3;
    }
    
    char value() const {
        assert(d_type==log_entry_type_insert);
        return char_1;
    }
    
    const string& name() const {
        assert(d_type==log_entry_type_create_file || d_type==log_entry_type_move_file);
        return string_1;
    }
};

class persistent_log {
    static const int max_block_size_decompressed=1024*1024*1024;
    static const int max_block_size_compressed=LZ4_COMPRESSBOUND(max_block_size_decompressed);
    
    deque<log_entry> d_log_entries;
    
    static const char block_header_mode_default=72;
    struct block_header {
        char mode;
        int first_log_entry_index;
        u32 decompressed_size;
        u32 compressed_size;
        u32 crc32;
    } __attribute__ ((packed));
    
    string file_name;
    unique_ptr<fstream> disk_stream;
    int num_persisted_log_entries;
    
    void load_stream() {
        disk_stream=make_unique_ptr<fstream>(file_name.c_str(), ios::in|ios::out|ios::binary);
    }
    
    bool d_was_corrupted;
    bool d_read_only;
    int last_valid_position;
    
    public:
    bool allow_persist_if_corrupted;
    bool assert_if_corrupted;
    
    //can make this multi-threaded and use async io (send all async read requests first with 1mb request size, then do processing) if it is slow
    persistent_log(const string& t_file_name, bool t_allow_persist_if_corrupted=false, bool t_assert_if_corrupted=true) {
        file_name=t_file_name;
        d_was_corrupted=false;
        last_valid_position=-1;
        allow_persist_if_corrupted=t_allow_persist_if_corrupted;
        assert_if_corrupted=t_assert_if_corrupted;
        
        load_stream();
        
        vector<char> read_buffer(32*1024);
        vector<char> decompressed_buffer(32*1024);
        
        assert(disk_stream->tellg()==0);
        
        while (true) {
            read_buffer.resize(sizeof(block_header));
            disk_stream->read(&(read_buffer[0]), sizeof(block_header));
            
            block_header h; {
                auto h_ptr=(block_header*)&(read_buffer[0]);
                h=*h_ptr;
                h_ptr->crc32=0;
            }
            
            if (
                disk_stream->gcount()!=sizeof(h) ||
                h.mode!=block_header_mode_default ||
                h.first_log_entry_index!=d_log_entries.size() ||
                h.decompressed_size>max_block_size_decompressed ||
                h.compressed_size>max_block_size_compressed
            ) {
                if (disk_stream->gcount()!=0 || !disk_stream->eof()) {
                    d_was_corrupted=true;
                    assert(!assert_if_corrupted);
                }
                break;
            }
            
            read_buffer.resize(sizeof(h)+h.compressed_size);
            decompressed_buffer.resize(h.decompressed_size);
            
            disk_stream->read(&(read_buffer[sizeof(h)]), h.compressed_size);
            
            boost::crc_32_type crc;
            crc.process_bytes(&(read_buffer[0]), read_buffer.size());
            if (crc.checksum()!=h.crc32) {
                d_was_corrupted=true;
                assert(!assert_if_corrupted);
                break;
            }
            
            //since checksum has been verified, can use asserts
            assert(LZ4_decompress_safe(&(read_buffer[sizeof(h)]), &(decompressed_buffer[0]), h.compressed_size, h.decompressed_size)==h.decompressed_size);
            
            int pos=0;
            while (pos<decompressed_buffer.size()) {
                d_log_entries.push_back(log_entry::decode(decompressed_buffer, pos));
            }
            
            if (last_valid_position==-1) {
                last_valid_position=0;
            }
            
            assert(disk_stream->tellg()==last_valid_position+read_buffer.size());
            last_valid_position=disk_stream->tellg();
        }
        
        assert(!disk_stream->bad());
        disk_stream->clear(); //clear eof error
        
        num_persisted_log_entries=d_log_entries.size();
    }
    
    void set_read_only(bool t_read_only) {
        d_read_only=t_read_only;
    }
    
    //calling persist will truncate the corrupted part of the log if this is true
    bool was_corrupted() const {
        return d_was_corrupted;
    }
    
    int allocate(log_entry&& c) {
        assert(!d_read_only);
        
        d_log_entries.push_back(move(c));
        return d_log_entries.size()-1;
    }
    
    const deque<log_entry>& log_entries() const {
        return d_log_entries;
    }
    
    //returns true if everything in the log has been written to the os page cache
    bool persist() {
        assert(!d_read_only);
        
        if (num_persisted_log_entries==d_log_entries.size()) {
            return true;
        }
        
        if (d_was_corrupted) {
            if (!allow_persist_if_corrupted) {
                return false;
            }
            
            //truncate file
            assert(last_valid_position!=-1);
            disk_stream.reset();
            boost::filesystem::resize_file(file_name, last_valid_position);
            load_stream();
            disk_stream->seekg(last_valid_position);
            
            last_valid_position=-1;
            d_was_corrupted=false;
        }
        
        vector<char> write_buffer(32*1024);
        vector<char> compress_buffer(32*1024);
        
        for (int index=num_persisted_log_entries;index<d_log_entries.size();) {
            int first_log_entry_index=index;
            
            compress_buffer.clear();
            while (index<d_log_entries.size()) {
                int size=compress_buffer.size();
                d_log_entries[index].encode(compress_buffer);
                
                //this isn't done because it does not play nice with transactions
                //if this code is used, there needs to be transaction rollback support in case there is corruption or a crash
                //however, every single log entry is written to the same block, no rollback support is required since recovery will
                // always end on a transaction boundary (concurrency is not supported)
                //if (compress_buffer.size()>max_block_size_decompressed) {
                    //compress_buffer.resize(size);
                    //break;
                //}
                
                assert(compress_buffer.size()<=max_block_size_decompressed);
                
                ++index;
            }
            
            assert(compress_buffer.size()>0 && compress_buffer.size()<=max_block_size_decompressed);
            write_buffer.resize(sizeof(block_header)+LZ4_COMPRESSBOUND(compress_buffer.size()));
            
            int compressed_size=LZ4_compress_default(&(compress_buffer[0]), &(write_buffer[sizeof(block_header)]), compress_buffer.size(), LZ4_COMPRESSBOUND(compress_buffer.size()));
            assert(compressed_size>0 && compressed_size<=max_block_size_compressed);
            write_buffer.resize(sizeof(block_header)+compressed_size);
            
            block_header h;
            h.mode=block_header_mode_default;
            h.first_log_entry_index=first_log_entry_index;
            h.decompressed_size=compress_buffer.size();
            h.compressed_size=compressed_size;
            h.crc32=0;
            
            auto h_ptr=(block_header*)&(write_buffer[0]);
            *h_ptr=h;
            
            boost::crc_32_type crc;
            crc.process_bytes(&(write_buffer[0]), write_buffer.size());
            h_ptr->crc32=crc.checksum();
            
            disk_stream->write(&(write_buffer[0]), write_buffer.size());
            assert(!disk_stream->fail());
        }
        
        num_persisted_log_entries=d_log_entries.size();
        disk_stream->flush();
        assert(!disk_stream->fail());
        
        return true;
    }
};

void create_ide_files(const string& root_path_string, const char* data_dir_name, const char* log_file_name) {
    boost::filesystem::path root_path(root_path_string);
    assert(root_path.is_absolute());
    
    auto data_dir_path=root_path / data_dir_name;
    auto log_file_path=data_dir_path / log_file_name;
    
    if (!boost::filesystem::exists(data_dir_path)) {
        assert(boost::filesystem::exists(root_path));
        for (auto c=boost::filesystem::recursive_directory_iterator(root_path);c!=boost::filesystem::recursive_directory_iterator();++c) {
            assert(c->path().has_filename() && c->path().filename()!=data_dir_name);
        }
        
        for (auto c_path=root_path;!c_path.empty();c_path=c_path.remove_filename()) {
            assert(!boost::filesystem::exists(c_path/data_dir_name));
        }
        
        assert(boost::filesystem::create_directory(data_dir_path));
    } else {
        assert(boost::filesystem::is_directory(data_dir_path));
    }
    
    {
        ofstream out(log_file_path.c_str(), ios::binary|ios::app);
        assert(out.good());
    }
}


}
