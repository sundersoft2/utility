/*class monitor_file {
    boost::filesystem::path path;
    string d_data;
    time_t last_write_time;
    bool d_valid;
    
    public:
    history_compile_log(const boost::filesystem::path& t_path) : path(t_path), last_write_time(0), d_valid(false) {}
    
    bool valid() const {
        return d_valid;
    }
    
    const string& data() const {
        return d_data;
    }
    
    //returns true if anything changed
    bool update() {
        boost::system::error_code e;
        int t_last_write_time=boost::filesystem::last_write_time(path, &e);
        bool was_valid=d_valid;
        
        if (e.value()!=0) {
            assert(e.value()==boost::system::errc::no_such_file_or_directory);
            d_data=string();
            last_write_time=0;
            d_valid=false;
            
            return was_valid;
        } else
        if (t_last_write_time!=last_write_time) {
            last_write_time=t_last_write_time;
            d_valid=true;
            
            string new_data=getfile(path.string(), true);
            if (was_valid && d_data==new_data) {
                return false;
            } else {
                d_data=new_data;
                return true;
            }
        }
    }
};*/

int clamp(int v, int v_min, int v_max) {
    return min(max(v, v_min), v_max);
}

class timer {
    u32 start_time;
    bool enable;
    
    public:
    timer(bool t_enable) {
        enable=t_enable;
        
        if (!enable) {
            return;
        }
        
        start_time=SDL_GetTicks();
    }
    
    void signal(const char* file, int line) {
        if (!enable) {
            return;
        }
        
        u32 end_time=SDL_GetTicks();
        u32 duration=(end_time-start_time);
        
        cerr << "Timer " << file << ":" << line << " took " << duration << " ms.\n";
        
        start_time=SDL_GetTicks();
    }
    
    ~timer() {
        if (!enable) {
            return;
        }
        
        cerr << "\n";
    }
};

#define timer_begin() timer c_timer(true);
#define timer_begin_disable() timer c_timer(false);
#define timer_signal() c_timer.signal(__FILE__, __LINE__)

template<class type> class list {};

template<class type> class safe_list : public std::list<type> {
    public:
    void size() const {} //c++ language designers were on crack, list::size runs in linear time
};

typedef vec_base<int, 2> ivec2;
const int int_max=numeric_limits<int>::max();
const int half_int_max=int_max/2;

bool is_ascii_printable(char c) {
    return c>=33 && c<=126;
}

void tolower(string& s) {
    for (auto c=s.begin();c!=s.end();++c) {
        *c=tolower(*c);
    }
}

class disable_copy {
    disable_copy(const disable_copy&);
    disable_copy(disable_copy&&);
    void operator=(const disable_copy&);
    void operator=(disable_copy&&);
    
    public:
    disable_copy() {}
};

struct sha1 {
    static const int digest_size=5;
    u32 digest[digest_size];
    
    sha1() { fill_n(digest, digest_size, 0); }
    template<class func> sha1(func f) { assign(f); }
    sha1(const string& s) {
        bool valid=true;
        assign([&]() {
            if (valid && s.size()!=0) {
                valid=false;
                return make_pair(&(s[0]), (int)s.size());
            } else {
                return make_pair((const char*)nullptr, 0);
            }
        });
    }
    
    template<class func> void assign(func f) {
        boost::uuids::detail::sha1 s;
        while (true) {
            pair<const char*, int> range=f();
            if (range.first!=nullptr) {
                s.process_bytes(range.first, range.second);
            } else {
                break;
            }
        }
        s.get_digest(digest);
    }
    
    string to_string() {
        ostringstream out;
        out << hex;
        for (int x=0;x<digest_size;++x) {
            out << digest[x] << " ";
        }
        return out.str();
    }
    
    bool operator==(const sha1& t) const {
        return equal(digest, digest+digest_size, t.digest);
    }
    
    bool operator!=(const sha1& t) const {
        return !((*this)==t);
    }
    
    bool operator<(const sha1& t) const {
        return lexicographical_compare(digest, digest+digest_size, t.digest, t.digest+digest_size);
    }
};
