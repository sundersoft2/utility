struct instruction_entry {
    string names;
    string command;
    double priority=0;
};

const double instruction_block_default_delta_priority=-1e-6;

struct instruction_block {
    enum mode_type {
        mode_manual,
        mode_auto,
        mode_debug
    };

    vector<pair<double, double>> priority_stack;

    int mode=-1;
    int yield_interval=-1;
    int min_yield_stalls=-1;
    double variable_delta=0;

    double next_priority=0;
    double delta_priority=instruction_block_default_delta_priority;
    map<string, pair<set<int>, bool>> name_barriers; //false: normal mode; true: wait on listed barriers
    vector<instruction_entry> instructions;
};

struct kernel_data {
    int index=-1;
    vector<instruction_block> blocks;
    cubin_kernel k;
    int num_instructions=0;

    map<string, fragment> named_fragments;

    map<string, uint64> label_addresses;
};

void optimize(optimizer::optimizer_state& state, instruction_block& block, ostream* optimizer_dump) {
    if (block.mode==instruction_block::mode_manual || block.mode==instruction_block::mode_debug) {
        if (block.mode==instruction_block::mode_debug) {
            for (auto& c : state.inputs) {
                bool is_write=false;
                bool is_read=false;
                if (c.state.needs_barrier()) {
                    if (c.state.num_inputs>=1) {
                        is_read=true;
                    }
                    if (c.state.num_outputs>=1) {
                        is_write=true;
                    }
                }

                c.flags.reuse_flags={false, false, false, false};
                c.flags.stall_count=15;
                c.flags.yield_flag=true;
                c.flags.write_barrier=(is_write)? 0 : -1;
                c.flags.read_barrier=(is_read)? 0 : -1;
                c.flags.wait_flags={true, true, true, true, true, true};
            }
        }

        for (auto& c : state.inputs) {
            c.names={{"sync", false}};
        }

        optimizer::greedy_ordering(state, true, -1, 0);
    } else {
        assert(block.mode==instruction_block::mode_auto);

        for (auto& c : state.inputs) {
            for (auto& n_pair : c.names) {
                auto i=block.name_barriers.find(n_pair.first);
                if (i==block.name_barriers.end()) {
                    continue;
                }
                if (!i->second.second) {
                    continue;
                }

                for (int id : i->second.first) {
                    c.flags.wait_flags.at(id)=true;
                }
            }
        }

        for (pair<const string, pair<set<int>, bool>>& c : block.name_barriers) {
            if (c.second.second) {
                continue;
            }

            set<int>& d=state.name_barriers[c.first];
            for (int id : c.second.first) {
                d.insert(id);
            }
        }

        optimizer::greedy_ordering(state, false, block.yield_interval, block.variable_delta);
        optimizer::generate_optimal_reuse(state);
        optimizer::generate_reuse_stats(state);
        optimizer::calculate_stalls(state, block.min_yield_stalls);
        optimizer::calculate_expected_wait_time(state);
    }

    if (optimizer_dump!=nullptr) {
        optimizer::dump_stats(state, *optimizer_dump);
    }
}

bool parse_label(string command, string& label_name) {
    if (!command.empty() && command.back()==':') {
        label_name.clear();

        bool found_non_space=false;
        bool found_space_after_non_space=false;
        for (int x=0;x<command.size()-1;++x) {
            char c=command[x];
            if (isspace(c)) {
                if (found_non_space) {
                    found_space_after_non_space=true;
                }
            } else {
                assert(!found_space_after_non_space);
                found_non_space=true;
                label_name+=c;
            }
        }
        assert(!label_name.empty());

        return true;
    } else {
        return false;
    }
}

void process_instructions(
    instruction_block& block,
    vector<pair<processed_line, control_flags>>& processed_lines,
    ostream* optimizer_dump,
    function<fragment(string)> get_named_fragment
) {
    optimizer::optimizer_state c_optimizer_state;

    for (auto& i : block.instructions) {
        optimizer::input_line l;
        l.priority=i.priority;

        string s=i.command;
        {
            size_t pos=s.find("//");
            if (pos!=string::npos) {
                l.comment=s.substr(pos+2);
                s=s.substr(0, pos);
            }
        }

        string label_name;
        if (parse_label(s, label_name)) {
            auto p=get_instruction_state_label(label_name);
            l.state=move(p.first);
            l.line=move(p.second);
        } else {
            if (block.mode==instruction_block::mode_manual) {
                s=l.flags.input(s);
            }

            l.line=process_line(s, get_named_fragment);
            get_instruction_state(l.line, l.state, block.mode!=instruction_block::mode_manual);
        }

        const string& names=i.names;
        if (!names.empty()) {
            string buffer;
            bool found_separator=false;
            for (int x=0;x<=names.size();++x) {
                char c=(x==names.size())? '\0' : names[x];

                if (c=='%' || c==',' || c=='\0') {
                    if (buffer.empty()) {
                        assert(c=='\0' || c=='%');
                    } else {
                        l.names.emplace_back(buffer, found_separator);
                    }
                    buffer.clear();

                    if (c=='%') {
                        assert(!found_separator);
                        found_separator=true;
                    }
                } else {
                    buffer+=c;
                }
            }
            assert(buffer.empty());
            assert(found_separator);
        }

        c_optimizer_state.inputs.push_back(l);
    }

    optimize(c_optimizer_state, block, optimizer_dump);

    for (auto& c : c_optimizer_state.outputs) {
        processed_lines.emplace_back(c.line, c.flags);
    }
}

void generate_cubin_file(string in_file, string optimizer_dump_path) {
    ofstream optimizer_dump;
    if (!optimizer_dump_path.empty()) {
        optimizer_dump.open(optimizer_dump_path);
    }

    map<string, kernel_data> kernels;
    kernel_data* current_kernel=nullptr;

    map<string, fragment> named_fragments;

    auto add_named_fragment_impl=[&](string name, int v, string symbol_name, kernel_data* k) {
        fragment f;
        f.assign_int(v);
        f.rel_symbol_name=symbol_name;
        assert(((k==nullptr)? named_fragments : k->named_fragments).emplace(name, f).second);
    };

    auto add_named_fragment=[&](string name, int v, string symbol_name, kernel_data* k, int size) {
        add_named_fragment_impl(name, v, symbol_name, k);
        for (int x=0;x<size;++x) {
            assert(symbol_name.empty());
            add_named_fragment_impl(name + "_" + to_string(x), v+x, symbol_name, k);
        }
    };

    cubin_file res_file;

    ifstream in(in_file);
    assert(in.good());
    while (true) {
        string s;
        getline(in, s);
        if (s.empty() && in.eof()) {
            break;
        }

        if (s.empty()) {
            continue;
        }

        /*
        if (params[0]=="label") {
        assert(params.size()==2);
        assert(current_kernel!=nullptr);
        //before next instruction (or before next control code)
        uint64 dest=index_to_absolute_maxwell(current_kernel->instructions.size(), false);

        add_named_fragment(params[1], dest, "", current_kernel);
        } else
        */

        if (s[0]!='#') {
            assert(current_kernel!=nullptr && !current_kernel->blocks.empty());

            string names;
            string command;
            if (s.find('%')!=string::npos) {
                size_t space=s.find(' ');
                assert(space!=string::npos);
                names=s.substr(0, space);
                command=s.substr(space+1);
            } else {
                command=s;
            }

            string label_name;
            if (parse_label(command, label_name)) {
                assert(!label_name.empty());
                add_named_fragment(label_name, 0, label_name, current_kernel, 0);
            }

            instruction_entry c_entry;
            c_entry.names=names;
            c_entry.command=command;
            c_entry.priority=current_kernel->blocks.back().next_priority;
            current_kernel->blocks.back().next_priority+=current_kernel->blocks.back().delta_priority;

            current_kernel->blocks.back().instructions.emplace_back(c_entry);

            if (label_name.empty()) {
                ++current_kernel->num_instructions;
            }

            continue;
        }

        vector<string> params;
        size_t pos=1;
        while (pos!=string::npos) {
            size_t next_pos=s.find(' ', pos);
            params.push_back(s.substr(pos, (next_pos==string::npos)? string::npos : next_pos-pos));
            pos=(next_pos==string::npos)? string::npos : next_pos+1;
        }

        assert(params.size()>=1);

        if (params[0]=="kernel") {
            assert(params.size()==2);
            assert(kernels.emplace(params[1], kernel_data()).second);
            current_kernel=&kernels.at(params[1]);
            current_kernel->k.name=params[1];
            current_kernel->index=kernels.size()-1;
        } else
        if (params[0]=="block") {
            assert(params.size()>=2 && params.size()<=5);
            assert(current_kernel!=nullptr);

            int yield_interval=-1;
            if (params.size()>=3) {
                yield_interval=assert_from_string<int>(params[2]);
            }

            int min_yield_stalls=-1;
            if (params.size()>=4) {
                min_yield_stalls=assert_from_string<int>(params[3]);
            }

            double variable_delta=0;
            if (params.size()>=5) {
                variable_delta=assert_from_string<double>(params[4]);
            }

            int mode;
            if (params[1]=="manual") {
                mode=instruction_block::mode_manual;
            } else
            if (params[1]=="auto") {
                mode=instruction_block::mode_auto;
            } else {
                assert(params[1]=="debug");
                mode=instruction_block::mode_debug;
            }

            current_kernel->blocks.emplace_back();
            current_kernel->blocks.back().mode=mode;
            current_kernel->blocks.back().yield_interval=yield_interval;
            current_kernel->blocks.back().min_yield_stalls=min_yield_stalls;
            current_kernel->blocks.back().variable_delta=variable_delta;
        } else
        if (params[0]=="priority_push") {
            assert(params.size()==2 || params.size()==3);
            assert(current_kernel!=nullptr && !current_kernel->blocks.empty());
            auto& c_block=current_kernel->blocks.back();

            c_block.priority_stack.emplace_back(c_block.next_priority, c_block.delta_priority);
            c_block.next_priority=assert_from_string<double>(params[1]);

            c_block.delta_priority=(params.size()==3)? assert_from_string<double>(params[2]) : 1;
            c_block.delta_priority*=instruction_block_default_delta_priority;
        } else
        if (params[0]=="priority_pop") {
            assert(params.size()==1);
            assert(current_kernel!=nullptr && !current_kernel->blocks.empty());
            auto& c_block=current_kernel->blocks.back();

            assert(!c_block.priority_stack.empty());
            c_block.next_priority=c_block.priority_stack.back().first;
            c_block.delta_priority=c_block.priority_stack.back().second;
            c_block.priority_stack.pop_back();
        } else
        if (params[0]=="barrier") {
            assert(params.size()>=4);

            assert(params[2]=="wait" || params[2]=="sequence");
            bool wait_mode=(params[2]=="wait");

            assert(current_kernel!=nullptr && !current_kernel->blocks.empty());
            auto i=current_kernel->blocks.back().name_barriers.emplace(params[1], make_pair(set<int>(), wait_mode));
            assert(i.second);

            for (int x=3;x<params.size();++x) {
                int index=assert_from_string<int>(params[x]);
                assert(i.first->second.first.insert(index).second);
            }
        } else
        if (params[0]=="global" || params[0]=="constant" || params[0]=="param") {
            assert(params.size()>=4);
            cubin_param p;
            p.name=params[1];
            p.size=assert_from_string<int>(params[2]);
            p.alignment=assert_from_string<int>(params[3]);

            if (params[0]=="global") {
                assert(params.size()==5);
                p.index=assert_from_string<int>(params[4]);
                res_file.device_globals.push_back(p);
            } else
            if (params[0]=="constant") {
                assert(params.size()==4);
                res_file.constant_globals.push_back(p);
            } else {
                assert(params[0]=="param");
                assert(params.size()==4);
                assert(current_kernel!=nullptr);
                current_kernel->k.params.push_back(p);
            }
        } else
        if (params[0]=="extern") {
            assert(params.size()==2);
            add_named_fragment(params[1], 0, params[1], nullptr, 0);
        } else
        if (params[0]=="attribute") {
            assert(params.size()==3);
            assert(current_kernel!=nullptr);
            string n=params[1];
            int v=assert_from_string<int>(params[2]);

            if (n=="num_registers") {
                current_kernel->k.num_registers=v;
            } else
            if (n=="num_barriers") {
                current_kernel->k.num_barriers=v;
            } else
            if (n=="has_shared") {
                current_kernel->k.has_shared=v;
            } else
            if (n=="max_stack_size") {
                current_kernel->k.max_stack_size=v;
            } else
            if (n=="min_stack_size") {
                current_kernel->k.min_stack_size=v;
            } else
            if (n=="frame_size") {
                current_kernel->k.frame_size=v;
            } else
            if (n=="max_registers") {
                current_kernel->k.max_registers=v;
            } else
            if (n=="required_num_tids") {
                current_kernel->k.required_num_tids=v;
            } else
            if (n=="max_threads") {
                current_kernel->k.max_threads=v;
            } else
            if (n=="crs_stack_size") {
                current_kernel->k.crs_stack_size=v;
            } else {
                assert(false);
            }
        } else
        if (params[0]=="pad") {
            assert(params.size()<=2);
            assert(current_kernel!=nullptr && !current_kernel->blocks.empty());
            auto& i=current_kernel->blocks.back();

            int alignment=maxwell_default_alignment;
            if (params.size()==2) {
                alignment=assert_from_string<int>(params[1]);
            }

            while (!is_aligned_maxwell(current_kernel->num_instructions, alignment)) {
                if (i.mode==instruction_block::mode_manual) {
                    i.instructions.emplace_back();
                    i.instructions.back().command= " : :      :    :Y: 0    NOP;" ;
                } else {
                    i.instructions.emplace_back();
                    i.instructions.back().names= "sync%" ;
                    i.instructions.back().command= "NOP;" ;
                }
                ++current_kernel->num_instructions;
            }
        } else {
            assert(false);
        }
    }

    {
        int dummy;
        calculate_params(res_file.constant_globals, dummy);
        calculate_params(res_file.device_globals, dummy);
        for (auto& k : kernels) {
            calculate_params(k.second.k.params, dummy);
        }
    }

    // constant bank 0; each value is 4 bytes:
    // 0000    ?
    // 0004    ?
    // 0008    blockDimX
    // 000c    blockDimY
    // 0010    blockDimZ
    // 0014    gridDimX
    // 0018    gridDimY
    // 001c    gridDimZ
    // 0020    local address for end of stack frame (not sure how this works with function calls)
    // ...
    // 0140    start of params
    add_named_fragment( "block_size_x"  , 0x0008, "", nullptr, 4 );
    add_named_fragment( "block_size_y"  , 0x000c, "", nullptr, 4 );
    add_named_fragment( "block_size_z"  , 0x0010, "", nullptr, 4 );
    add_named_fragment( "grid_size_x"   , 0x0014, "", nullptr, 4 );
    add_named_fragment( "grid_size_y"   , 0x0018, "", nullptr, 4 );
    add_named_fragment( "grid_size_z"   , 0x001c, "", nullptr, 4 );
    add_named_fragment( "local_end"     , 0x0020, "", nullptr, 4 );

    add_named_fragment( "params_bank"   , 0x0000, "", nullptr, 0 );
    add_named_fragment( "globals_bank"  , 0x0003, "", nullptr, 0 );

    //constant bank 3
    for (cubin_param& c : res_file.constant_globals) {
        add_named_fragment(c.name, c.offset, "", nullptr, c.size);
    }

    for (cubin_param& c : res_file.device_globals) {
        add_named_fragment(c.name + "_lo", 0, c.name + "_lo", nullptr, 0);
        add_named_fragment(c.name + "_hi", 0, c.name + "_hi", nullptr, 0);
    }

    for (auto& k : kernels) {
        //constant bank 0
        for (cubin_param& c : k.second.k.params) {
            add_named_fragment(c.name, c.offset, "", &(k.second), c.size);
        }
    }

    for (auto& k : kernels) {
        auto get_named_fragment=[&](string name) {
            auto i1=named_fragments.find(name);
            auto i2=k.second.named_fragments.find(name);

            assert( (i1==named_fragments.end()) != (i2==k.second.named_fragments.end()) );
            if (i1!=named_fragments.end()) {
                return i1->second;
            } else {
                return i2->second;
            }
        };

        vector<pair<processed_line, control_flags>> processed_lines;

        for (auto& c : k.second.blocks) {
            process_instructions(
                c, processed_lines,
                (optimizer_dump_path.empty())? nullptr : &optimizer_dump,
                get_named_fragment
            );
        }

        int next_index=0;
        for (int x=0;x<processed_lines.size();++x) {
            auto& c=processed_lines[x];

            if (c.first.is_label()) {
                uint64 dest=index_to_absolute_maxwell(next_index, false);
                assert(k.label_addresses.emplace(c.first.get_label(), dest).second);
            } else {
                ++next_index;
            }
        }

        next_index=0;
        for (auto& c : processed_lines) {
            if (c.first.is_label()) {
                continue;
            }

            for (argument& a : c.first.arguments) {
                for (fragment& v : a.value) {
                    if (v.rel_symbol_name.empty()) {
                        continue;
                    }

                    auto i=k.label_addresses.find(v.rel_symbol_name);
                    if (i==k.label_addresses.end()) {
                        continue;
                    }

                    v.rel_symbol_name.clear();
                    v.assign_int(i->second);
                }
            }

            c.first.absolute_to_relative(next_index);
            k.second.k.instructions.push_back(get_opcode_encode().encode(c.first));
            k.second.k.instructions.back().c_flags=c.second;

            ++next_index;
        }
    }

    {
        ofstream out(in_file +".labels", ios::binary);
        for (auto& k : kernels) {
            out << k.k.name << "\n";
            out << -1 << "\n";

            for (auto& c : k.label_addresses) {
                out << c.first << "\n";
                out << c.second << "\n";
            }
        }

        out << "@end" << "\n";
        out << -1 << "\n";
    }

    res_file.kernels.resize(kernels.size());
    for (auto& k : kernels) {
        res_file.kernels.at(kernels.size()-1-k.second.index)=move(k.second.k);
    }

    {
        ofstream out(in_file +".cubin", ios::binary);
        out << res_file.output();
    }
}