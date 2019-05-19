namespace optimizer {


//each reuse slot has a queue of two entries
//each cycle, will read both entries to see if reuse is possible. if a reuse is done then the entry is invalidated
//each cycle, if reuse flag is set, will pop from queue and push the register
//each cycle, if reuse flag is cleared, will pop from queue and push an invalid value
//when popping from the queue, will pop the reused entry if a reuse read was done. otherwise, will pop the oldest entry
//writes do not affect the reuse cache, so a stale value will be read if a value in the reuse cache is overwritten
//branches do not clear the reuse cache

struct reuse_stream {
    static const int reuse_cache_size=2;

    struct entry {
        int register_index=-1;

        bool reuse_write=false;
        bool reuse_read=false;
    };

    deque<entry> entries;
    deque<entry*> cache;

    reuse_stream() {
        cache.resize(2, nullptr);
    }

    todo //this doesn't handle doubles; not sure how they are implemented by the gpu
    void add_read(int register_index, int size) {
        assert(size==1 || size==2);
        assert(cache.size()==reuse_cache_size);

        if (size==2) {
            todo //get this to work
            add_null(true, false);
            return;
        }

        int hit_index=-1;
        for (int x=0;x<cache.size();++x) {
            if (cache[x]!=nullptr && cache[x]->register_index==register_index) {
                hit_index=x;
                break;
            }
        }

        int cache_index=-1;
        if (hit_index==-1) {
            cache.pop_front();
            cache.push_back(nullptr);
            cache_index=cache.size()-1;
        } else {
            cache[hit_index]->reuse_write=true;
            cache_index=hit_index;
        }

        entry e;
        e.register_index=register_index;
        e.reuse_read=(hit_index!=-1);
        entries.push_back(e);
        cache.at(cache_index)=&entries.back();
    }

    void add_null(bool is_branch, bool is_label) {
        entries.push_back(entry());

        //can end up reading stale values from the reuse cache if the flags are wrong
        //need to make sure no reads happen across a label or branch
        if (is_branch || is_label) {
            for (entry*& e : cache) {
                e=nullptr;
            }
        }
    }

    void add_write(int register_index, int size) {
        for (entry*& e : cache) {
            if (e==nullptr) {
                continue;
            }

            int r=e->register_index;
            if (r>=register_index && r<register_index+size) {
                e=nullptr;
            }
        }
    }

    void set_flags(optimizer_state& s, int slot) {
        assert(s.outputs.size()==entries.size());
        for (int x=0;x<entries.size();++x) {
            entry& e=entries[x];

            s.outputs[x].flags.reuse_flags[slot]=e.reuse_write;
            s.outputs[x].reuse_reads[slot]=e.reuse_read;
            s.outputs[x].reuse_writes[slot]=e.reuse_write;
            s.outputs[x].reuse_final_reads[slot]=e.reuse_read && !e.reuse_write;
        }
    }
};

struct optimal_reuse {
    static const int num_reuse=3;
    array<reuse_stream, num_reuse> reuse_streams;

    static void get_instruction_inputs(
        const instruction_state& state,
        int& size,
        array<int, num_reuse>& reuse_registers,
        array<map<int, int>, 4>& reads_per_bank
    ) {
        size=-2;

        for (int x=0;x<num_reuse;++x) {
            reuse_registers[x]=-1;
        }

        for (int x=0;x<4;++x) {
            reads_per_bank[x].clear();
        }

        for (int x=0;x<state.num_inputs;++x) {
            argument_state i=state.inputs[x];

            if (i.mode!=argument_state::mode_register) {
                continue;
            }

            if (size==-2) {
                size=i.register_size;
            } else {
                if (size!=i.register_size) {
                    size=-1;
                }
            }

            assert(i.index>=0);
            assert( (i.index%4) % i.register_size == 0 );

            //register banks support broadcasts
            for (int y=0;y<i.register_size;++y) {
                ++reads_per_bank.at((i.index+y)%4)[i.index+y];
            }

            if (i.reuse_slot!=-1) {
                assert(size==1 || size==2);

                assert(reuse_registers[i.reuse_slot]==-1);
                reuse_registers[i.reuse_slot]=i.index;
            }
        }
    }

    static void bank_conflict_cycles(
        array<int, num_reuse> reuse_registers,
        array<map<int, int>, 4> reads_per_bank,
        array<bool, num_reuse> reuse_read_mask,
        int& cycles_before_reuse,
        int& cycles_after_reuse,
        bool& has_redundant_reuse
    ) {
        cycles_before_reuse=1;
        cycles_after_reuse=1;
        has_redundant_reuse=false;

        for (int x=0;x<4;++x) {
            cycles_before_reuse=max(cycles_before_reuse, (int)reads_per_bank[x].size());
        }

        array<map<int, int>, 4> reuse_reads_per_bank;
        for (int x=0;x<num_reuse;++x) {
            if (!reuse_read_mask[x]) {
                continue;
            }

            int index=reuse_registers[x];
            assert(index>=0);
            ++reuse_reads_per_bank[index%4][index];

            int &non_reuse=reads_per_bank[index%4].at(index);
            --non_reuse;
            assert(non_reuse>=0);

            if (non_reuse==0) {
                reads_per_bank[index%4].erase(index);
            }
        }

        for (int x=0;x<4;++x) {
            cycles_after_reuse=max(cycles_after_reuse, (int)reads_per_bank[x].size());
        }

        for (int x=0;x<4;++x) {
            map<int, int>& reuse_reads=reuse_reads_per_bank[x];
            map<int, int>& non_reuse_reads=reads_per_bank[x];

            for (pair<int, int> i : reuse_reads) {
                if (non_reuse_reads.count(i.first)) {
                    //if there is a broadcast, it should either be all reuse reads or all non-reuse reads
                    has_redundant_reuse=true;
                }
            }

            if (!reuse_reads.empty() && non_reuse_reads.size()<cycles_after_reuse) {
                //too many reuse reads
                has_redundant_reuse=true;
            }
        }
    }

    void add(const instruction_state& state) {
        int size;
        array<int, num_reuse> reuse_registers;
        array<map<int, int>, 4> reads_per_bank;
        get_instruction_inputs(state, size, reuse_registers, reads_per_bank);

        for (int x=0;x<num_reuse;++x) {
            auto& s=reuse_streams[x];
            int r=reuse_registers[x];

            if (r==-1) {
                s.add_null(state.is_branch(), state.is_label());
            } else {
                s.add_read(r, size);
            }
        }

        for (int x=0;x<state.num_outputs;++x) {
            argument_state o=state.outputs[x];

            if (o.mode==argument_state::mode_register) {
                for (int y=0;y<num_reuse;++y) {
                    reuse_streams[y].add_write(o.index, o.register_size);
                }
            }
        }
    }

    void calculate(optimizer_state& state) {
        for (int x=0;x<num_reuse;++x) {
            reuse_streams[x].set_flags(state, x);
        }
    }
};

void generate_reuse_stats(optimizer_state& state) {
    for (optimizer_state::output& c : state.outputs) {
        int size;
        array<int, optimal_reuse::num_reuse> reuse_registers;
        array<map<int, int>, 4> reads_per_bank;
        optimal_reuse::get_instruction_inputs(
            c.state,
            size,
            reuse_registers,
            reads_per_bank
        );

        int cycles_before_reuse;
        int cycles_after_reuse;
        bool has_redundant_reuse;
        optimal_reuse::bank_conflict_cycles(
            reuse_registers,
            reads_per_bank,
            c.reuse_reads,
            cycles_before_reuse,
            cycles_after_reuse,
            has_redundant_reuse
        );

        --cycles_before_reuse;
        assert(cycles_before_reuse>=0);

        --cycles_after_reuse;
        assert(cycles_after_reuse>=0);

        c.bank_conflict_cycles_before_reuse=cycles_before_reuse;
        c.bank_conflict_cycles_after_reuse=cycles_after_reuse;
    }
}

void generate_optimal_reuse(optimizer_state& state) {
    const int num_reuse=optimal_reuse::num_reuse;

    optimal_reuse c_optimal_reuse;
    for (auto& c : state.outputs) {
        c_optimal_reuse.add(c.state);
    }
    c_optimal_reuse.calculate(state);
}


}