namespace optimizer {


class greedy_ordering {
    struct barrier_settings {
        string name;
        set<int> numbers;
    };

    //each output register should either have at least one reader or no writers after this instruction
    struct unordered_operation {
        int source=-1;
        double priority=0; //used if multiple operations can be dispatched; greatest priority is chosen

        //bit 0: wait for read to finish
        //bit 1: wait for write to finish
        //if both bits are cleared then latency is 0 but has to be after
        vector<pair<unordered_operation*, char>> dependencies;
        vector<unordered_operation*> dependants;

        //both should be at least 1 except for special lines
        uint8 read_latency=0;
        uint8 write_latency=0;
        int pipeline=-1;
        int ordered_index=-1;

        //for branches etc
        uint8 min_stall_count=0;

        int cycle=-1;
        int num_determinate_dependencies=0;

        barrier_settings read_barrier;
        barrier_settings write_barrier;

        multimap<int, unordered_operation*>* c_map=nullptr;
        multimap<int, unordered_operation*>::iterator iter;
    };

    struct barrier_state {
        int barrier_number=-1;
        int finish_cycle=-1;

        set<unordered_operation*> pending_reads;
        set<unordered_operation*> finished_reads;

        set<unordered_operation*> pending_writes;
        set<unordered_operation*> finished_writes;

        set<unordered_operation*> determinate_operations;
    };

    enum pipeline_type {
        pipeline_fixed,
        pipeline_variable,
        pipeline_special,
        num_pipelines
    };

    vector<unordered_operation> graph;
    array<multimap<int, unordered_operation*>, num_pipelines> determinate_operations;
    int min_stall_end_cycle=-1;
    int last_yield_cycle=0;

    map<string, barrier_state> barrier_states;
    set<int> active_barrier_numbers;

    barrier_state* get_dependency_barrier(unordered_operation& dependency, char wait_read_write) {
        bool wait_read=(wait_read_write & 1);
        bool wait_write=(wait_read_write & 2);

        if (!wait_read && !wait_write) {
            return nullptr;
        }

        //if there is both a read and write dependency, only need to wait on the write barrier
        barrier_settings& t_settings=(wait_write)? dependency.write_barrier : dependency.read_barrier;

        if (t_settings.name.empty()) {
            return nullptr;
        }

        barrier_state& s=barrier_states.at(t_settings.name);

        set<unordered_operation*>& pending=(wait_write)? s.pending_writes : s.pending_reads;
        set<unordered_operation*>& finished=(wait_write)? s.finished_writes : s.finished_reads;

        if (finished.count(&dependency)) {
            assert(!pending.count(&dependency));
            return nullptr;
        }

        //if this assert fails then the dependency hasn't executed yet which is not supposed to happen
        assert(pending.count(&dependency));

        return &s;
    }

    void set_wait_flags(unordered_operation& dependency, char wait_read_write, control_flags& flags) {
        barrier_state* s_ptr=get_dependency_barrier(dependency, wait_read_write);
        if (s_ptr==nullptr) {
            return;
        }
        barrier_state& s=*s_ptr;

        bool wait_read=(wait_read_write & 1);
        bool wait_write=(wait_read_write & 2);

        if (s.ordered_index!=-1 && s.ordered_index==dependency.ordered_index && wait_read && !wait_write) {
            //if all of the stuff in the barrier is part of the same gpu pipeline and is executed in-order, then a write by dependency
            //will happen after all reads in the pipeline have finished so the flag doesn't actually need to be set
            return;
        }

        todo //can use depbar here; can auto-detect when it is useful and have a threshold for when to enable it
        flags.wait_flags.at(s.barrier_number)=true;

        for (unordered_operation* i : s.pending_writes) {
            assert(s.finished_writes.insert(i).second);
        }
        s.pending_writes.clear();

        for (unordered_operation* i : s.pending_reads) {
            assert(s.finished_reads.insert(i).second);
        }
        s.pending_reads.clear();

        s.determinate_operations.clear();

        assert(active_barrier_numbers.erase(s.barrier_number));
        s.barrier_number=-1;
        s.finish_cycle=-1;
    }

    int get_wait_cycle_and_mark_dependant(unordered_operation& parent, unordered_operation& dependency, char wait_read_write) {
        barrier_state* s_ptr=get_dependency_barrier(dependency, wait_read_write);

        if (s_ptr!=nullptr) {
            s_ptr->determinate_operations.insert(&parent);
        }

        return (s_ptr==nullptr)? -1 : s_ptr->finish_cycle;
    }

    //call this twice with is_read=true then is_read=false
    bool set_read_write_flags(int c_cycle, unordered_operation& c_operation, control_flags& flags, bool is_read) {
        barrier_settings& t_settings=(is_read)? c_operation.read_barrier : c_operation.write_barrier;

        if (t_settings.name.empty()) {
            return false;
        }

        barrier_state& s=barrier_states[t_settings.name];

        assert( (s.barrier_number==-1) == (s.pending_reads.empty() && s.pending_writes.empty()) );
        if (s.barrier_number==-1) {
            for (int i : t_settings.numbers) {
                if (!active_barrier_numbers.count(i)) {
                    s.barrier_number=i;
                    break;
                }
            }

            assert(active_barrier_numbers.insert(s.barrier_number).second);

            s.ordered_index=c_operation.ordered_index;
        }
        assert(s.barrier_number!=-1);

        if (s.ordered_index!=c_operation.ordered_index) {
            //have stuff from multiple pipelines in the barrier
            s.ordered_index=-1;
        }

        set<unordered_operation*>& pending=(is_read)? s.pending_reads : s.pending_writes;
        set<unordered_operation*>& finished=(is_read)? s.finished_reads : s.finished_writes;

        assert(!finished.count(&c_operation));
        assert(pending.insert(&c_operation).second);

        ((is_read)? flags.read_barrier : flags.write_barrier)=s.barrier_number;

        s.finish_cycle=max(s.finish_cycle, c_cycle+(is_read? c_operation.read_latency : c_operation.write_latency));

        for (unordered_operation* c : s.determinate_operations) {
            assert(c->c_map!=nullptr);

            if (c->iter->first>=s.finish_cycle) {
                continue;
            }

            c->c_map->erase(c->iter);
            c->iter=c->c_map->emplace(s.finish_cycle, c);
        }

        return true;
    }

    void init(const optimizer_state& source, bool disable_barriers) {
        graph.resize(source.inputs.size());

        map<int, unordered_operation*> last_writers;
        map<int, unordered_operation*> last_readers; //includes writers
        map<string, unordered_operation*> named_operations;
        vector<unordered_operation*> current_sync_operations;

        for (int source_index=0;source_index<source.inputs.size();++source_index) {
            unordered_operation& r=graph[source_index];
            const instruction_state& r_source=source.inputs[source_index].state;

            r.source=source_index;
            r.priority=source.inputs[source_index].priority;
            r.ordered_index=r_source.get_ordered_index();

            if (r_source.pipeline==instruction_state::pipeline_fixed) {
                r.read_latency=1;
                r.write_latency=r_source.fixed_latency;
                r.pipeline=pipeline_fixed;
            } else {
                r.read_latency=r_source.typical_read_latency;
                r.write_latency=r_source.typical_write_latency;
                r.pipeline=pipeline_variable;
            }
            r.min_stall_count=r_source.min_stall_count;

            if (r_source.pipeline==instruction_state::pipeline_special_line) {
                r.pipeline=pipeline_special;
            }

            map<unordered_operation*, char> pending_dependencies;

            vector<barrier_settings> c_barrier_settings;

            bool is_sync=false;
            for (auto& n_pair : source.inputs[source_index].names) {
                const string& n=n_pair.first;
                assert(!n.empty());

                auto i=source.name_barriers.find(n);
                if (i!=source.name_barriers.end()) {
                    c_barrier_settings.emplace_back();
                    c_barrier_settings.back().name=n;
                    c_barrier_settings.back().numbers=i->second;
                }

                if (n_pair.second) {
                    if (named_operations.count(n)) {
                        pending_dependencies.emplace(named_operations.at(n), 0);
                    }
                } else {
                    unordered_operation*& o=named_operations[n];

                    if (o!=nullptr) {
                        pending_dependencies.emplace(o, 0);
                    }

                    if (n=="sync") {
                        assert(!is_sync);
                        is_sync=true;

                        for (unordered_operation* t : current_sync_operations) {
                            pending_dependencies.emplace(t, 0);
                        }
                        current_sync_operations.clear();
                    }

                    o=&r;
                }
            }

            if (!is_sync) {
                auto i=named_operations.find("sync");
                if (i!=named_operations.end()) {
                    pending_dependencies.emplace(i->second, 0);
                    current_sync_operations.push_back(&r);
                }
            }

            auto add_dependency=[&](const argument_state& a, bool is_read) {
                assert(a.register_size>=1);
                for (int offset=0;offset<a.register_size;++offset) {
                    int i=a.get_normalized_index(offset);

                    unordered_operation* previous_writer=nullptr;
                    unordered_operation* previous_reader=nullptr;

                    auto write_iter=last_writers.find(i);
                    if (write_iter!=last_writers.end()) {
                        previous_writer=write_iter->second;
                    }

                    auto read_iter=last_readers.find(i);
                    if (read_iter!=last_readers.end()) {
                        previous_reader=read_iter->second;
                    }

                    //read after read isn't a dependency

                    if (is_read && previous_writer!=nullptr) {
                        //read after write
                        char& t_flags=pending_dependencies.emplace(previous_writer, 0).first->second;
                        t_flags|=2;
                    }
                    if (!is_read && previous_reader!=nullptr) {
                        //write after read
                        char& t_flags=pending_dependencies.emplace(previous_reader, 0).first->second;
                        t_flags|=1;
                    }
                    if (!is_read && previous_writer!=nullptr) {
                        //write after write
                        char& t_flags=pending_dependencies.emplace(previous_writer, 0).first->second;
                        t_flags|=2;
                    }
                }
            };

            auto update_registers=[&](const argument_state& a, bool is_read) {
                assert(a.register_size>=1);
                for (int offset=0;offset<a.register_size;++offset) {
                    int i=a.get_normalized_index(offset);

                    (is_read? last_readers : last_writers)[i]=&r;
                }
            };

            for (int y=0;y<r_source.num_inputs;++y) {
                add_dependency(r_source.inputs[y], true);
            }

            for (int y=0;y<r_source.num_outputs;++y) {
                add_dependency(r_source.outputs[y], false);
            }

            for (int y=0;y<r_source.num_inputs;++y) {
                update_registers(r_source.inputs[y], true);
            }

            for (int y=0;y<r_source.num_outputs;++y) {
                update_registers(r_source.outputs[y], false);
            }

            for (auto& c : pending_dependencies) {
                r.dependencies.push_back(c);
                c.first->dependants.emplace_back(&r);
            }

            if (r.dependencies.empty()) {
                r.c_map=&determinate_operations[r.pipeline];
                r.iter=r.c_map->emplace(0, &r);
            }

            bool needs_barrier=r_source.needs_barrier();

            if (disable_barriers) {
                needs_barrier=false;
            }

            //can put a fixed pipeline instruction on a barrier sequence
            if (needs_barrier) {
                assert(c_barrier_settings.size()==1 || c_barrier_settings.size()==2);

                if (c_barrier_settings.size()==2) {
                    assert(r_source.num_inputs!=0 && r_source.num_outputs!=0);
                    r.read_barrier=c_barrier_settings[0];
                    r.write_barrier=c_barrier_settings[1];
                } else {
                    assert(r_source.num_inputs!=0 || r_source.num_outputs!=0);
                    if (r_source.num_inputs!=0) {
                        r.read_barrier=c_barrier_settings[0];
                    }
                    if (r_source.num_outputs!=0) {
                        r.write_barrier=c_barrier_settings[0];
                    }
                }
            }
        }
    }

    void add_output(int c_cycle, optimizer_state& source, unordered_operation& o, bool should_yield, bool is_special) {
        optimizer_state::output c_output;

        c_output.state=source.inputs.at(o.source).state;
        c_output.line=source.inputs.at(o.source).line;
        c_output.flags=source.inputs.at(o.source).flags;
        c_output.comment=source.inputs.at(o.source).comment;

        if (should_yield) {
            c_output.flags.yield_flag=true;
        }

        for (pair<unordered_operation*, char> c : o.dependencies) {
            set_wait_flags(*c.first, c.second, c_output.flags);
        }
        bool set_read=set_read_write_flags(c_cycle, o, c_output.flags, true);
        bool set_write=set_read_write_flags(c_cycle, o, c_output.flags, false);

        if (set_read && set_write && c_output.flags.read_barrier==c_output.flags.write_barrier) {
            //don't redundantly set the read barrier
            c_output.flags.read_barrier=-1;
        }

        source.outputs.push_back(move(c_output));
    }

    bool process_cycle(optimizer_state& source, int current_cycle, int pipeline, int yield_interval) {
        auto& c_operations=determinate_operations[pipeline];

        double pending_priority=NAN;
        multimap<int, unordered_operation*>::iterator pending=c_operations.end();

        for (auto i=c_operations.begin();i!=c_operations.end();++i) {
            assert(i->first != -1);
            if (i->first > current_cycle) {
                break;
            }

            if (pending==c_operations.end() || (pending_priority < i->second->priority)) {
                pending=i;
                pending_priority=i->second->priority;
            }
        }

        if (pending==c_operations.end() || (min_stall_end_cycle!=-1 && current_cycle<min_stall_end_cycle)) {
            return false;
        }

        unordered_operation& o=*pending->second;
        c_operations.erase(pending);
        o.c_map=nullptr;

        assert(o.source!=-1);
        assert(o.cycle==-1);
        o.cycle=current_cycle;

        if (o.min_stall_count!=0) {
            min_stall_end_cycle=current_cycle+o.min_stall_count;
        }

        bool should_yield=false;
        if (yield_interval!=-1 && o.pipeline!=pipeline_special && last_yield_cycle+yield_interval<current_cycle) {
            should_yield=true;
            last_yield_cycle=current_cycle;
        }

        add_output(current_cycle, source, o, should_yield, pipeline==pipeline_special);

        for (unordered_operation* c : o.dependants) {
            assert(c->cycle==-1);

            ++c->num_determinate_dependencies;
            if (c->num_determinate_dependencies < c->dependencies.size()) {
                continue;
            }

            int min_cycle=0;
            for (pair<unordered_operation*, char>& d : c->dependencies) {
                bool wait_read=(d.second & 1);
                bool wait_write=(d.second & 2);

                auto& r=*d.first;
                assert(r.pipeline==pipeline_special || (r.read_latency>=1 && r.write_latency>=1 && r.write_latency>=r.read_latency));

                //dependant operation has to write for this operation to read
                //dependant operation has to read for this operation to write
                int latency=0;
                if (wait_read) {
                    assert(r.pipeline!=pipeline_special);
                    latency=max(latency, (int)r.read_latency);
                }
                if (wait_write) {
                    assert(r.pipeline!=pipeline_special);
                    latency=max(latency, (int)r.write_latency);
                }

                assert(r.cycle!=-1);
                min_cycle=max(min_cycle, r.cycle+latency);

                min_cycle=max(min_cycle, get_wait_cycle_and_mark_dependant(*c, *d.first, d.second));
            }

            assert(c->c_map==nullptr);
            c->c_map=&determinate_operations[c->pipeline];
            c->iter=c->c_map->emplace(min_cycle, c);
        }

        return true;
    }

    public:
    greedy_ordering(
        optimizer_state& source, bool disable_barriers, int yield_interval, double variable_delta
    ) {
        init(source, disable_barriers);
        assert(source.outputs.empty());

        double variable_cooldown=0;

        int current_cycle=0;
        while (source.outputs.size()<graph.size()) {
            process_cycle(source, current_cycle, pipeline_fixed, yield_interval);

            if (variable_cooldown<=0) {
                if (process_cycle(source, current_cycle, pipeline_variable, yield_interval)) {
                    variable_cooldown+=variable_delta;
                }
            }

            while (true) {
                if (!process_cycle(source, current_cycle, pipeline_special, yield_interval)) {
                    break;
                }
            }

            ++current_cycle;

            if (variable_cooldown>0) {
                --variable_cooldown;
            }
        }
    }
};


}