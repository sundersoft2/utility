struct opcode_number {
    array<int, 65> bits; //first bit is negative flag
    int num_bits=-1; //excludes sign bit
};

struct opcode_data {
    opcode_type opcode;
    uint64 delta_mask;
};

struct opcode_group {
    vector<pair<int, delta>> deltas;
    map<string, opcode_data> opcodes;

    processed_line normalized;
    vector<opcode_number> numbers;

    bool add_number(uint64 value, bool negative, int index, opcode_type& number_mask, opcode_type& number_set_mask) {
        uint64 num_bits=numbers.at(index).num_bits; //excludes sign bit

        if (negative) {
            if (value > (1ull << num_bits)) {
                debug_print("add_number", value, negative, index, (1ull << num_bits), "too big");
                return false;
            }

            int64 v=value;
            v=-v;
            value=v;
            value &= (1ull << num_bits)-1;
        }

        for (uint64 x=0;x<65;++x) {
            bool v=true;
            if (x==0 && !negative) {
                v=false;
            }
            if (x!=0 && (value & (1ull << (x-1)))==0) {
                v=false;
            }

            int bit=numbers.at(index).bits[x];

            if (bit==-1) {
                if (v) {
                    debug_print("add_number", value, negative, index, bit, "bit out of range");
                    return false;
                }
                continue;
            }

            opcode_type mask=(1ull << opcode_type(bit));

            bool v_existing=(number_mask & mask)!=0;
            bool v_set=(number_set_mask & mask)!=0;

            if (v_set) {
                //can have multiple numbers mapped to the same bits
                if (v!=v_existing) {
                    debug_print("add_number", value, negative, index, bit, "conflicting values for bit");
                    return false;
                }
            }

            number_set_mask|=mask;
            if (v) {
                number_mask|=mask;
            }
        }

        return true;
    }

    bool delta_reduce(processed_line& p_line, opcode_type& opcode) {
        debug_print("==== delta_reduce ====", "'"+ p_line.output(false, false, false) + "'");

        p_line.add_optional_predicate(normalized);

        opcode_type opcode_mask=0;
        uint64 delta_mask=0;
        for (int x=0;x<deltas.size();++x) {
            if (!remove_delta(p_line, deltas[x].second)) {
                continue;
            }

            assert(deltas.size()<=64);
            opcode_mask |= 1ull << uint64(deltas[x].first);
            delta_mask |= 1ull << uint64(x);
        }

        p_line.add_optional_end(normalized);

        string p_str=p_line.output(true, true, false);
        auto op_iter=opcodes.find(p_str);
        if (op_iter==opcodes.end()) {
            debug_print("delta_reduce", "'" + p_str + "'", "not found");
            for (auto& c : opcodes) {
                debug_print("delta_reduce", "'" + c.first + "'", "candidate");
            }
            return false;
        }

        opcode_type base_opcode=op_iter->second.opcode;

        uint64 allowed_delta_mask=op_iter->second.delta_mask;
        if ((delta_mask & (~allowed_delta_mask))!=0) {
            debug_print("delta_reduce", delta_mask, allowed_delta_mask, "forbidden delta");
            return false;
        }

        vector<pair<uint64, bool>> p_numbers=p_line.extract_numbers();
        assert(p_numbers.size()==numbers.size());

        opcode_type number_mask=0;
        opcode_type number_set_mask=0;
        for (int x=0;x<p_numbers.size();++x) {
            if (!add_number(p_numbers[x].first, p_numbers[x].second, x, number_mask, number_set_mask)) {
                return false;
            }
        }

        opcode=base_opcode | opcode_mask | number_mask;
        return true;
    }
};

struct opcode_encode {
    map<string, vector<opcode_group>> opcode_groups;

    void load_opcodes(string file) {
        ifstream in(file);
        assert(in.good());

        string dummy;
        while (true) {
            string initial;
            getline(in, initial);
            if (initial.empty() && in.eof()) {
                break;
            }
            assert(initial == "OPCODE");

            string name;
            {
                string name_a;
                in >> name_a;
                getline(in, dummy);

                todo //get rid of this
                size_t a=name_a.find("#");
                assert(a!=string::npos);
                name=name_a.substr(0, a);
            }

            opcode_groups[name].emplace_back();
            opcode_group& c_group=opcode_groups[name].back();

            int num_deltas;
            in >> num_deltas;
            getline(in, dummy);

            for (int x=0;x<num_deltas;++x) {
                int bit;
                in >> bit;
                getline(in, dummy);

                delta d;
                in >> d.argument_index;
                getline(in, dummy);

                getline(in, d.added_prefix.text);
                getline(in, d.added_suffix.text);

                c_group.deltas.emplace_back(bit, d);
            }

            {
                string s;
                getline(in, s);
                c_group.normalized=process_line(s);
            }

            int num_number;
            in >> num_number;
            getline(in, dummy);

            for (int x=0;x<num_number;++x) {
                array<int, 65> a;
                int num_set=0;
                for (int y=0;y<65;++y) {
                    in >> a[y];
                    if (y!=0 && a[y]!=-1) {
                        ++num_set;
                    }
                }
                getline(in, dummy);

                assert(num_set<64);

                opcode_number n;
                n.bits=a;
                n.num_bits=num_set;
                c_group.numbers.push_back(n);
            }

            int num_all;
            in >> num_all;
            getline(in, dummy);

            for (int x=0;x<num_all;++x) {
                uint64 mask;
                in >> hex >> mask >> dec;
                getline(in, dummy);

                string line;
                getline(in, line);

                todo //this assumes the opcode is 64 bits; probably want it on a separate line
                opcode_type opcode;
                assert(extract_uint64(line, opcode));

                size_t e=line.find(';');
                assert(e!=string::npos);

                opcode_data c_data;
                c_data.opcode=opcode;
                c_data.delta_mask=mask;

                assert(c_group.opcodes.emplace(line.substr(0, e), c_data).second);
            }

            assert(in.good());
        }
    }

    instruction encode(const processed_line& p_line, bool allow_error=false, string error_line="", int error_index=-1) {
        vector<opcode_group> c_groups=opcode_groups.at(p_line.get_name());

        vector<opcode_type> found_opcodes;

        for (opcode_group& c_group : c_groups) {
            processed_line p_line_copy=p_line;

            opcode_type opcode;
            if (!c_group.delta_reduce(p_line_copy, opcode)) {
                continue;
            }

            found_opcodes.push_back(opcode);
        }

        if (found_opcodes.empty()) {
            if (allow_error) {
                print("ERROR: No opcode found", error_line, error_index);
                return instruction();
            } else {
                assert(false);
            }
        }

        for (int x=1;x<found_opcodes.size();++x) {
            if (found_opcodes[x]!=found_opcodes[0]) {
                if (allow_error) {
                    print("ERROR: Multiple conflicting opcodes found", error_line, error_index);
                    return instruction();
                } else {
                    assert(false);
                }
            }
        }

        string symbol_name;
        for (const argument& a : p_line.arguments) {
            for (const fragment& v : a.value) {
                if (!v.rel_symbol_name.empty()) {
                    assert(symbol_name.empty());
                    symbol_name=v.rel_symbol_name;
                }
            }
        }

        instruction res;

        string name=p_line.get_name();
        if (name=="JCAL" && !symbol_name.empty()) {
            res.mode=instruction::mode_rel_jcal;
            res.symbol_name=symbol_name;
            symbol_name.clear();
        } else
        if (name=="MOV32I" && !symbol_name.empty()) {
            string tail=symbol_name.substr(symbol_name.size()-3);
            assert(symbol_name.size()>=3 && (tail=="_lo" || tail=="_hi"));
            res.mode=(tail=="_lo")? instruction::mode_rel_mov32i_lo : instruction::mode_rel_mov32i_hi;
            res.symbol_name=symbol_name.substr(0, symbol_name.size()-3);
            symbol_name.clear();
        } else
        if (name=="SHFL") {
            res.mode=instruction::mode_shfl;
        } else
        if (name=="EXIT") {
            res.mode=instruction::mode_exit;
        } else
        if (name=="S2R" || name=="CS2R") {
            bool found_ctaid=false;
            int t_mode=0;
            for (const argument& a : p_line.arguments) {
                for (const fragment& v : a.prefix) {
                    if (v.text=="SR_CTAID") {
                        assert(!found_ctaid);
                        found_ctaid=true;
                    } else
                    if (v.text==".X" && found_ctaid) {
                        assert(t_mode==0);
                        t_mode=instruction::mode_s2r_ctaid_x;
                    } else
                    if (v.text==".Y" && found_ctaid) {
                        assert(t_mode==0);
                        t_mode=instruction::mode_s2r_ctaid_y;
                    } else
                    if (v.text==".Z" && found_ctaid) {
                        assert(t_mode==0);
                        t_mode=instruction::mode_s2r_ctaid_z;
                    }
                }
            }
            if (t_mode!=0) {
                res.mode=t_mode;
            }
        }

        res.opcode=found_opcodes[0];

        assert(symbol_name.empty());
        return res;
    }
};

opcode_encode& get_opcode_encode() {
    static opcode_encode c_encode;
    static bool init=false;
    if (!init) {
        c_encode.load_opcodes(compressed_file_path);
        init=true;
    }
    return c_encode;
}