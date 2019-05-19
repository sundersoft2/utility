const bool fast_profiling=false;

class profiling_type;
class profiling_mode {
    friend class profiling_type;

    static vector<profiling_mode*> root_modes;
    string name;
    profiling_mode* parent;
    vector<profiling_mode*> children;

    bool used_in_fast_profiling;

    public:
    profiling_mode(bool t_used_in_fast_profiling, string t_name, profiling_mode* t_parent=nullptr) :
        name(t_name), parent(t_parent), used_in_fast_profiling(t_used_in_fast_profiling)
    {

        if (parent==nullptr) {
            root_modes.push_back(this);
        } else {
            parent->children.push_back(this);
        }
    }
};
vector<profiling_mode*> profiling_mode::root_modes=vector<profiling_mode*>();

struct auto_timer {
    win::timer* t=nullptr;

    auto_timer() {}
    auto_timer(auto_timer&& targ) { *this=move(targ); }
    auto_timer& operator=(auto_timer&& targ) {
        stop();
        t=targ.t;
        targ.t=nullptr;
        return *this;
    }

    void stop() {
        if (t!=nullptr) {
            t->stop();
        }
        t=nullptr;
    }

    ~auto_timer() {
        stop();
    }
};

class profiling_type {
    struct key {
        int width;
        int height;
        int boundary_size;

        tuple<int, int, int> as_tuple() const {
            return make_tuple(width, height, boundary_size);
        }

        bool operator<(const key& b) const {
            return as_tuple()<b.as_tuple();
        }

        void print() const {
            cerr << width << " x " << height << " (BS: " << boundary_size << ")";
        }
    };

    struct data {
        deque<win::timer> timers;
    };

    map<key, map<profiling_mode*, data>> entries;

    public:
    profiling_type() {
        #ifdef __OPTIMIZE__
            cerr << "Doing spin loop to increase cpu clock\n";
            auto start_clock=clock();
            while ((clock()-start_clock)<1*CLOCKS_PER_SEC);
        #endif
    }

    auto_timer start(int width, int height, int boundary_size, profiling_mode* mode) {
        if (fast_profiling && !mode->used_in_fast_profiling) {
            return auto_timer();
        }

        key k;
        k.width=width;
        k.height=height;
        k.boundary_size=boundary_size;
        auto& e=entries[k][mode];
        e.timers.push_back(win::timer());
        auto_timer res;
        res.t=&(e.timers.back());
        return res;
    }

    private:
    void report_mode(profiling_mode& mode, map<profiling_mode*, data>& c_datas, int indent, double parent_time) {
        double total_time=0;
        int num_timers=0;

        auto c_data_iter=c_datas.find(&mode);
        if (c_data_iter!=c_datas.end()) {
            num_timers=c_data_iter->second.timers.size();
            for (auto& c_timer : c_data_iter->second.timers) {
                total_time+=c_timer.time();
            }
        } else {
            return;
        }

        for (int x=0;x<indent;++x) {
            cerr << " ";
        }

        cerr << fixed << setprecision(3) << (total_time*1000) << "ms total ; ";
        cerr << fixed << setprecision(3) << (total_time/num_timers*1000) << "ms avg ; ";
        cerr << fixed << setprecision(3) << num_timers << " calls ; ";

        if (parent_time!=-1) {
            cerr << fixed << setprecision(3) << (total_time/parent_time*100) << "% ";
        }

        cerr << "                        : " << mode.name << "\n";

        for (auto child_mode : mode.children) {
            report_mode(*child_mode, c_datas, indent+4, total_time);
        }
    }

    public:
    void report() {
        profiling_init_modes();

        auto do_report=[&](decltype(entries.begin()) entry_i) {
            auto& entry=*entry_i;

            cerr << ">>>> ";
            entry.first.print();
            cerr << ":\n";

            for (auto mode : profiling_mode::root_modes) {
                report_mode(*mode, entry.second, 0, -1);
            }

            cerr << "\n\n";
        };

        int max_width=-1;
        int max_height=-1;
        {
            auto c=entries.end();
            if (c!=entries.begin()) {
                --c;
                max_width=c->first.width;
                max_height=c->first.height;
            }
        }

        auto is_basic=[&](decltype(entries.begin()) entry_i) {
            auto& entry=*entry_i;

            if (
                entry.first.width==max_width ||
                entry.first.width==0 ||
                entry.first.height==max_height ||
                entry.first.height==0
            ) {
                return true;
            }

            auto next_i=entry_i;
            ++next_i;

            if (next_i==entries.end()) {
                return true;
            }

            //only squares (except last few iterations)
            if (entry.first.width!=entry.first.height) {
                return false;
            }

            //only with maximum boundary size
            if (
                next_i->first.width!=entry.first.width ||
                next_i->first.height!=entry.first.height
            ) {
                return true;
            }

            return false;
        };

        cerr << ">>>>>>>>>>>>>>>>> BASIC:\n\n";
        for (auto entry=entries.begin();entry!=entries.end();++entry) {
            if (is_basic(entry)) {
                do_report(entry);
            }
        }

        cerr << ">>>>>>>>>>>>>>>>> EXTENDED:\n\n";
        for (auto entry=entries.begin();entry!=entries.end();++entry) {
            do_report(entry);
        }
    }
};

profiling_type profiling;