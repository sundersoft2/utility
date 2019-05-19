namespace optimizer {


struct input_line {
    instruction_state state;
    processed_line line;
    control_flags flags;
    string comment;
    double priority=0;

    vector<pair<string, bool /*is_dependency*/>> names;
};

struct optimizer_state {
    vector<input_line> inputs;
    map<string, set<int>> name_barriers; //one barrier per name

    struct output {
        instruction_state state;
        processed_line line;
        control_flags flags;
        string comment;

        array<bool, 3> reuse_reads={false, false, false};

        array<bool, 3> reuse_writes={false, false, false};
        array<bool, 3> reuse_final_reads={false, false, false};

        int bank_conflict_cycles_before_reuse=0;
        int bank_conflict_cycles_after_reuse=0;

        int expected_wait_cycles=0;
    };
    vector<output> outputs;
};

void dump_stats(optimizer_state& state, ostream& out) {
    vector<string> lines;

    int max_line_size_no_comment=0;
    for (auto& c : state.outputs) {
        lines.push_back(c.line.output(false, false, true));
        if (c.line.is_label()) {
            lines.back()+=":";
        } else {
            lines.back()+=";";
        }

        max_line_size_no_comment=max(max_line_size_no_comment, (int)lines.back().size());
    }

    int max_line_size=0;
    for (int x=0;x<state.outputs.size();++x) {
        string& l=lines.at(x);
        auto& c=state.outputs[x];

        while (l.size()<max_line_size_no_comment) {
            l+= " ";
        }

        if (!c.comment.empty()) {
            l+= " //" + c.comment;
        }

        max_line_size=max(max_line_size, (int)l.size());
    }

    int total_bank_conflict_cycles_before_reuse=0;
    int total_bank_conflict_cycles_after_reuse=0;
    int total_expected_wait_cycles=0;
    int total_stall_count=0;

    map<string, int> num_instructions;

    for (int x=0;x<state.outputs.size();++x) {
        optimizer_state::output& c=state.outputs[x];

        if (c.line.is_label()) {
            out << lines[x] << "\n";
            continue;
        }

        ++num_instructions[c.line.get_name()];

        string s=lines[x];
        while (s.size()<max_line_size) {
            s+=' ';
        }
        s=c.flags.output() +"    "+ s +" ";

        for (int y=0;y<3;++y) {
            s+=(c.reuse_writes[y])? 'W' : ' ';
        }
        s+=':';
        for (int y=0;y<3;++y) {
            if (c.reuse_final_reads[y]) {
                assert(c.reuse_reads[y]);
                s+='F';
            } else {
                s+=(c.reuse_reads[y])? 'R' : ' ';
            }
        }
        s+=':';

        s+=to_string(c.bank_conflict_cycles_before_reuse);
        s+=':';

        s+=to_string(c.bank_conflict_cycles_after_reuse);
        s+=':';

        if (c.expected_wait_cycles<100) {
            s+=' ';
        }
        if (c.expected_wait_cycles<10) {
            s+=' ';
        }
        s+=to_string(c.expected_wait_cycles);
        s+=":    ";

        for (int y=0;y<c.bank_conflict_cycles_after_reuse;++y) {
            s+='!';
        }
        for (int y=0;y<c.expected_wait_cycles;++y) {
            if (y>30) {
                s+="[etc]";
                break;
            }
            s+=',';
        }
        for (int y=0;y<c.flags.stall_count;++y) {
            s+='.';
        }

        total_bank_conflict_cycles_before_reuse+=c.bank_conflict_cycles_before_reuse;
        total_bank_conflict_cycles_after_reuse+=c.bank_conflict_cycles_after_reuse;
        total_expected_wait_cycles+=c.expected_wait_cycles;
        total_stall_count+=c.flags.stall_count;

        out << s << "\n";
    }

    int total_cycles=total_bank_conflict_cycles_after_reuse + total_expected_wait_cycles + total_stall_count;

    out << "total_bank_conflict_cycles_before_reuse: " << total_bank_conflict_cycles_before_reuse << "\n";
    out << "total_bank_conflict_cycles_after_reuse:  " << total_bank_conflict_cycles_after_reuse << "\n";
    out << "total_expected_wait_cycles:              " << total_expected_wait_cycles << "\n";
    out << "total_stall_count:                       " << total_stall_count << "\n";
    out << "total_cycles:                            " << total_cycles << "\n";

    for (pair<const string, int>& c : num_instructions) {
        float percent=float(c.second)/total_cycles*100;
        out << c.first << ": " << c.second << " ( " << percent << "% )\n";
    }

    out << "\n\n\n\n";
}

/*class bank_conflict_graph {
    struct node {
        int bank=-1;
        vector<pair<node*, int>> edges;
    };
    vector<node> nodes;
    int edge_sum=0; //each edge is counted twice

    void init(int num_registers) {
        for (int x=0;x<num_registers;++x) {
            nodes.emplace_back();
            nodes.back().bank=(x%4);
        }
    }

    int change_bank_preview(int index, int new_bank) const {
        node& n=nodes.at(index);
        int old_bank=n.bank;

        if (old_bank==new_bank) {
            return 0;
        }

        int delta=0;
        for (pair<node*, int> e : n.edges) {
            int other_bank=e.first->bank;
            if (other_bank==
        }
    }
};*/

/*struct argument_state {
    uint8 index=255;
    uint8 size=0;
};*/

//const int num_inputs=instruction_state::max_num_inputs;
//const int num_outputs=instruction_state::max_num_outputs;
//const int num_reuse=3;
//const int max_dependencies=8;

//array<argument_state, num_reuse> reuse_inputs;
//array<argument_state, num_inputs> non_reuse_inputs;
//array<argument_state, num_ouputs> outputs;
//array<uint8, num_reuse> reuse_flags; //0 - none, 1 - write, 2 - read
//array<int8, 4> reads_per_bank={-1, -1, -1, -1};

//can swap two instructions which changes the total runtime minus time spent in bank conflicts
//can change the register banks which changes the total time spent in bank conflicts
//two adjacent operations can be swapped if neither is dependent on the other
//each instruction has a cycle number which is stored as a tree of deltas
//the minimum cycle number of an instruction is the max of the write latencies of its read dependencies and the read latencies of
// its write dependencies. each latency is added to the instruction cycle number
/*struct fenwick_tree {
    vector<int> bit; // binary indexed tree

    void init(int n) {
        bit.resize(n, 0);
    }

    //sum of first r+1 elements
    int sum(int r) {
        int ret = 0;
        //"(r & (r+1))" clears the least significant 1 bit
        for (; r >= 0; r = (r & (r+1)) - 1) {
            ret += bit[r];
        }
        return ret;
    }

    void add(int idx, int delta) {
        //"idx | (idx+1)" sets the least significant 0 bit
        for (; idx < bit.size(); idx = idx | (idx+1)) {
            bit[idx] += delta;
        }
    }
};*/


}