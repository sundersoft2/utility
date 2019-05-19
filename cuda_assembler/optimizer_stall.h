namespace optimizer {


struct stall_counter {
    int current_cycle=0;
    bool last_was_variable=false;
    int next_min_stall=0;
    int last_stall_index=-1;
    int next_index=0;

    control_flags pending_wait;

    map<int, int> last_writers;
    map<int, int> last_readers;
    map<int, int> last_set_barrier;

    void set_min_cycle(int i, bool is_read, bool is_barrier, int v) {
        if (is_barrier) {
            assert(!is_read);
            int& tb=last_set_barrier[i];
            tb=max(tb, v);
        } else
        if (is_read) {
            int& tr=last_readers[i];
            tr=max(tr, v);
        } else {
            int& tw=last_writers[i];
            tw=max(tw, v);
        }
    }

    void add_command(instruction_state state, control_flags flags, int& stall_index, int& stall_value) {
        stall_index=-1;
        stall_value=-1;

        if (state.pipeline==instruction_state::pipeline_special_line) {
            for (int x=0;x<pending_wait.wait_flags.size();++x) {
                pending_wait.wait_flags[x]|=flags.wait_flags[x];
            }
            ++next_index;
            return;
        }

        int min_cycle=current_cycle;

        for (int x=0;x<state.num_inputs;++x) {
            auto a=state.inputs[x];
            for (int offset=0;offset<a.register_size;++offset) {
                int i=a.get_normalized_index(offset);
                min_cycle=max(min_cycle, last_writers[i]); //read after write
            }
        }

        for (int x=0;x<state.num_outputs;++x) {
            auto a=state.outputs[x];
            for (int offset=0;offset<a.register_size;++offset) {
                int i=a.get_normalized_index(offset);
                min_cycle=max(min_cycle, last_readers[i]); //write after read
                min_cycle=max(min_cycle, last_writers[i]); //write after write
            }
        }

        for (int x=0;x<flags.wait_flags.size();++x) {
            if (pending_wait.wait_flags[x]) {
                flags.wait_flags[x]=true;
                pending_wait.wait_flags[x]=false;
            }

            if (flags.wait_flags[x]) {
                min_cycle=max(min_cycle, last_set_barrier[x]);
            }
        }

        bool is_variable=(state.pipeline!=instruction_state::pipeline_fixed);
        int stall=min_cycle-current_cycle; //between previous instruction and this instruction
        assert(stall>=0);

        if (!(is_variable && !last_was_variable) && stall==0) {
            stall=1;
        }

        stall=max(stall, next_min_stall);

        current_cycle+=stall;
        last_was_variable=is_variable;
        next_min_stall=state.min_stall_count;

        todo //for branches, need to make the stall count high enough to finish any SET instructions

        stall_index=last_stall_index;
        stall_value=stall;
        last_stall_index=next_index;
        ++next_index;

        //
        //

        if (is_variable) {
            //need a delay before barriers can be waited on
            if (flags.read_barrier!=-1) {
                set_min_cycle(flags.read_barrier, false, true, current_cycle+state.fixed_latency);
            }
            if (flags.write_barrier!=-1) {
                set_min_cycle(flags.write_barrier, false, true, current_cycle+state.fixed_latency);
            }
        } else {
            assert(flags.write_barrier==-1 && flags.read_barrier==-1);

            //can overwrite a fixed pipeline input immediately
            for (int x=0;x<state.num_inputs;++x) {
                auto a=state.inputs[x];
                for (int offset=0;offset<a.register_size;++offset) {
                    int i=a.get_normalized_index(offset);
                    set_min_cycle(i, true, false, current_cycle);
                }
            }

            for (int x=0;x<state.num_outputs;++x) {
                auto a=state.outputs[x];
                for (int offset=0;offset<a.register_size;++offset) {
                    int i=a.get_normalized_index(offset);
                    set_min_cycle(i, false, false, current_cycle+state.fixed_latency);
                }
            }
        }
    }

    void finish_commands(int& stall_index, int& stall_value) {
        int min_cycle=current_cycle+1;

        for (auto& c : last_writers) {
            min_cycle=max(min_cycle, c.second);
        }

        for (auto& c : last_readers) {
            min_cycle=max(min_cycle, c.second);
        }

        for (auto& c : last_set_barrier) {
            min_cycle=max(min_cycle, c.second);
        }

        int stall=min_cycle-current_cycle; //for last instruction
        assert(stall>=1);

        stall=max(stall, next_min_stall);

        stall_index=last_stall_index;
        stall_value=stall;

        *this=stall_counter();
    }
};

void calculate_stalls(optimizer_state& state, int min_yield_stalls) {
    stall_counter c_counter;
    bool yield_pending=false; //the ordering pass can set a yield flag on a dual issued instruction
    for (int x=0;x<=state.outputs.size();++x) {
        int i=-1;
        int v=-1;
        if (x<state.outputs.size()) {
            auto& o=state.outputs[x];
            c_counter.add_command(o.state, o.flags, i, v);
        } else {
            c_counter.finish_commands(i, v);
        }

        if (i!=-1) {
            control_flags& f=state.outputs.at(i).flags;
            f.set_stall(v);
            if ((min_yield_stalls!=-1 && v>=min_yield_stalls) || yield_pending) {
                f.yield_flag=true;
            }

            if (f.yield_flag && f.stall_count==0) {
                f.yield_flag=false;
                yield_pending=true;
            }

            if (f.yield_flag) {
                yield_pending=false;
            }
        }
    }
}

//
//

struct expected_wait_time_counter {
    map<int, int> barrier_end_cycles;
    int current_cycle=0;

    int add_command(optimizer_state::output& o) {
        int min_cycle=current_cycle;
        for (int x=0;x<o.flags.wait_flags.size();++x) {
            if (o.flags.wait_flags[x]) {
                min_cycle=max(min_cycle, barrier_end_cycles[x]);
            }
        }

        int res=min_cycle-current_cycle;
        assert(res>=0);
        current_cycle=min_cycle;

        if (o.flags.read_barrier!=-1) {
            barrier_end_cycles[o.flags.read_barrier]=current_cycle+o.state.typical_read_latency;
        }

        if (o.flags.write_barrier!=-1) {
            barrier_end_cycles[o.flags.write_barrier]=current_cycle+o.state.typical_write_latency;
        }

        current_cycle+=o.flags.stall_count;

        return res;
    }
};

void calculate_expected_wait_time(optimizer_state& state) {
    expected_wait_time_counter c_counter;
    for (int x=0;x<state.outputs.size();++x) {
        state.outputs[x].expected_wait_cycles=c_counter.add_command(state.outputs[x]);
    }
}


}