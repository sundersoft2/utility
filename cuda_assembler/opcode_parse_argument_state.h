struct argument_state {
    enum mode_type {
        mode_register,
        mode_predicate,
        mode_other
    };

    enum other_index {
        i_carry,            //.CC and .X
        i_sync,             //SSY and SYNC
        i_return,           //PRET and RET
        i_break,            //PBK and BRK
        i_continue          //PCNT and CONT
    };

    int8 mode=-1;
    int8 register_size=-1; //must be 1 if mode is not mode_register
    int8 reuse_slot=-1; //must be -1 if this is an output or register_size==4; each reuse slot is 8 bytes
    int8 fragment_index=-1;
    int index=-1; //can't be RZ or PT

    int get_normalized_index(int offset) const {
        assert(index>=0);
        assert(mode>=0 && mode<(1<<4));
        assert(register_size>=1);
        assert(index%register_size==0);
        assert(offset>=0 && offset<register_size);

        int i=index+offset;
        assert(i>=0 && i<(1<<16));
        i|=mode << 16;
        return i;
    }
};

struct instruction_state {
    enum pipeline_type {
        pipeline_fixed,
        pipeline_variable, //doubles, halves, mufu, branches, barriers, etc
        pipeline_shared,
        pipeline_global,
        pipeline_local,
        pipeline_constant,
        pipeline_uniform,
        pipeline_special_line //not an actual instruction
        //pipeline_texture,
        //pipeline_surface
    };

    static const uint8 flag_load=            1; //both load and store set for atomics and reduce
    static const uint8 flag_store=           2;
    static const uint8 flag_sync=            4;
    static const uint8 flag_branch=          8;
    static const uint8 flag_branch_prepare= 16;
    static const uint8 flag_label=          32;

    int8 num_inputs=-1;
    int8 num_outputs=-1;
    int8 pipeline=-1;
    uint8 flags=0;

    uint8 min_stall_count=0; //used for branches and bar.sync
    uint8 fixed_latency=0; //for variable pipeline ops, this is the minimum latency before a barrier can be waited on
    uint8 typical_read_latency=0;
    uint8 typical_write_latency=0;

    static const int max_num_inputs=4;
    array<argument_state, max_num_inputs> inputs;

    static const int max_num_outputs=2;
    array<argument_state, max_num_outputs> outputs;

    bool needs_barrier() const {
        return
            pipeline!=pipeline_fixed &&
            pipeline!=pipeline_special_line &&
            (flags & flag_sync)==0 &&
            (flags & flag_branch)==0 &&
            (flags & flag_branch_prepare)==0 &&
            (num_inputs>=1 || num_outputs>=1)
        ;
    }

    bool is_branch() const {
        return flags & flag_branch;
    }

    bool is_label() const {
        return flags & flag_label;
    }

    int get_ordered_index() const {
        if (pipeline!=pipeline_fixed && pipeline!=pipeline_variable && pipeline!=pipeline_special_line) {
            return pipeline;
        } else {
            return -1;
        }
    }
};

pair<instruction_state, processed_line> get_instruction_state_label(const string& name) {
    instruction_state state;
    processed_line line;

    state.num_inputs=0;
    state.num_outputs=0;
    state.pipeline=instruction_state::pipeline_special_line;
    state.flags=instruction_state::flag_label;

    state.min_stall_count=0;
    state.fixed_latency=0;
    state.typical_read_latency=0;
    state.typical_write_latency=0;

    line.arguments.emplace_back();
    line.arguments.back().prefix.emplace_back();
    line.arguments.back().prefix.back().text=name;

    return make_pair(state, line);
}

todo //figure out reuse flags from nvdisasm output
todo //can figure most of this out in benchmarks
//not handled here: DEPBAR
bool get_instruction_state(processed_line& p_line, instruction_state& res, bool force_no_error) {
    auto error=[&]() {
        if (force_no_error) {
            print("get_instruction_state failed: ", p_line.output(false, false, true));
            assert(false);
        }

        res=instruction_state();
        res.num_inputs=0;
        res.num_outputs=0;
        res.pipeline=instruction_state::pipeline_fixed;
        res.flags=0;

        res.min_stall_count=0;
        res.fixed_latency=1;
        res.typical_read_latency=1;
        res.typical_write_latency=1;
        return false;
    };

    res=instruction_state();

    int name_i=-1;
    const argument& name_a=p_line.get_name_argument(name_i);
    string name=name_a.value[0].text;
    bool first_argument_is_predicate=(
        p_line.arguments.size()>=(name_i+2) &&
        p_line.arguments[name_i+1].value.size()==1 &&
        p_line.arguments[name_i+1].value[0].prefix=="P"
    );

    auto has_suffix=[&](string s) {
        for (auto& c : name_a.suffix) {
            if (c.text==s) {
                return true;
            }
        }
        return false;
    };

    int output_mode=-1; //0-none; 1-register; 2-predicate; 3-two predicates; 4-predicate+register
    bool is_half=false;
    bool is_double=false;
    int output_size=1;
    int input_size=1; //excludes inputs in brackets
    int address_size=has_suffix(".E")? 2 : 1; //stuff in brackets (inputs)
    string maxas_mode;
    int8 input_other_index=-1;
    int8 output_other_index=-1;
    bool has_reuse=false;
    int reuse_offset=0;

    //
    //maxas $s2rT
    //
    if (name=="S2R") {
        maxas_mode="s2r";
        output_mode=1;
        res.pipeline=instruction_state::pipeline_variable;
    } else
    //
    //maxas $smemT / $gmemT
    //
    if (
        name=="BAR" || name=="MEMBAR" ||
        name=="LDS" || name=="LD" || name=="LDG" || name=="LDL" || name=="LDC" ||
        name=="STS" || name=="ST" || name=="STG" || name=="STL" ||
        name=="RED" || name=="ATOM" || name=="ATOMS"
    ) {
        if (name=="BAR") {
            maxas_mode="x32_branch";
            res.pipeline=instruction_state::pipeline_variable;
            if (has_suffix(".SYNC")) {
                res.flags|=instruction_state::flag_sync;
                output_mode=0;
                res.min_stall_count=5;
            } else
            if (has_suffix(".ARV")) {
                output_mode=0;
            } else {
                return error();
            }
        } else
        if (name=="MEMBAR") {
            maxas_mode="x32_branch";
            output_mode=0;
            res.pipeline=instruction_state::pipeline_variable;
            res.flags|=instruction_state::flag_sync;
        } else
        if (name=="ATOMS" || name=="ATOM" || name=="RED") {
            maxas_mode=(name=="ATOMS")? "smem" : "gmem";
            res.flags|=instruction_state::flag_load;
            res.flags|=instruction_state::flag_store;

            if (name=="ATOMS") {
                res.pipeline=instruction_state::pipeline_shared;
            } else {
                res.pipeline=instruction_state::pipeline_global;
            }

            int size=(has_suffix(".U64") || has_suffix(".S64") || has_suffix(".F64"))? 2 : 1;
            if (name=="RED") {
                output_mode=0;
                input_size=size;
            } else {
                output_mode=1;
                output_size=size;
                input_size=size;
            }
        } else {
            maxas_mode=(name=="LDS" || name=="STS")? "smem" : "gmem";
            if (name.substr(0, 2)!="LD" && name.substr(0, 2)!="ST") {
                return error();
            }
            int size=1;
            if (has_suffix(".64")) {
                size=2;
            } else
            if (has_suffix(".128")) {
                size=4;
            }

            if (name.substr(0, 2)=="LD") {
                output_mode=1;
                output_size=size;
                res.flags|=instruction_state::flag_load;
            } else {
                output_mode=0;
                input_size=size;
                res.flags|=instruction_state::flag_store;
            }

            if (name.substr(2)=="S") {
                res.pipeline=instruction_state::pipeline_shared;
            } else
            if (name.substr(2)=="") {
                res.pipeline=instruction_state::pipeline_uniform;
            } else
            if (name.substr(2)=="G") {
                res.pipeline=instruction_state::pipeline_global;
            } else
            if (name.substr(2)=="L") {
                res.pipeline=instruction_state::pipeline_local;
            } else
            if (name.substr(2)=="C") {
                res.pipeline=instruction_state::pipeline_constant;
            } else {
                return error();
            }
        }
    } else
    //
    //maxas $x32T
    //
    if (name=="HADD2_32I" || name=="HADD2" || name=="HFMA2_32I" || name=="HFMA2" || name=="HMUL2_32I" || name=="HMUL2") {
        //16-bit floating-point add, multiply, multiply-add
        maxas_mode="x32";
        is_half=true;
        res.pipeline=instruction_state::pipeline_variable;
        output_mode=1;
    } else
    if (
        name=="FADD32I" || name=="FADD" || name=="FFMA32I" || name=="FFMA" || name=="FMUL32I" || name=="FMUL" ||
        name=="IADD32I" || name=="IADD3" || name=="IADD" ||
        name=="LOP32I" || name=="LOP3" || name=="LOP" ||
        name=="FCHK" || name=="MOV32I" || name=="MOV" || name=="XMAD" || name=="PRMT" || name=="SEL" ||
        name=="NOP" || name=="P2R" || name=="CS2R" || name=="LEPC"
    ) {
        //32-bit floating-point add, multiply, multiply-add
        //32-bit integer add, extended-precision add, subtract, extended-precision subtract
        //32-bit bitwise AND, OR, XOR
        maxas_mode="x32";
        res.pipeline=instruction_state::pipeline_fixed;

        if (name=="LOP" && first_argument_is_predicate) {
            maxas_mode= "cmp" ; //need to wait more cycles after setting a predicate
            output_mode=4;
        } else
        if (name=="FCHK") {
            maxas_mode= "cmp" ;
            output_mode=2;
        } else
        if (name=="NOP") {
            output_mode=0;
        } else {
            output_mode=1;
        }

        //mov: first is $icr20      (all other reuse instructions have the correct order)
        //order is: $r8 ; ($r20|$ir20|$cr20|$icr20|$fcr20|$dr20|$r39s20) ; ($r39|$r39a|$cr39)
        has_reuse=true;
        if (name=="MOV") {
            reuse_offset=1;
        }
    } else
    if (
        name=="BRA" || name=="BRX" || name=="CAL" || name=="EXIT" || name=="JCAL" || name=="JMP" || name=="JMX" ||
        name=="RET" || name=="BRK" || name=="CONT" || name=="SYNC"
    ) {
        //branches
        maxas_mode="x32_branch";
        output_mode=0;
        res.pipeline=instruction_state::pipeline_variable;
        res.flags|=instruction_state::flag_branch;
        res.min_stall_count=5;

        if (name=="RET") {
            input_other_index=argument_state::i_return;
        } else
        if (name=="BRK") {
            input_other_index=argument_state::i_break;
        } else
        if (name=="CONT") {
            input_other_index=argument_state::i_continue;
        } else
        if (name=="SYNC") {
            input_other_index=argument_state::i_sync;
        }
    } else
    if (name=="PRET" || name=="PBK" || name=="PCNT" || name=="SSY") {
        //prepare branch
        maxas_mode="x32_branch";
        output_mode=0;
        res.pipeline=instruction_state::pipeline_variable;
        res.flags|=instruction_state::flag_branch_prepare;

        if (name=="PRET") {
            output_other_index=argument_state::i_return;
        } else
        if (name=="PBK") {
            output_other_index=argument_state::i_break;
        } else
        if (name=="PCNT") {
            output_other_index=argument_state::i_continue;
        } else
        if (name=="SSY") {
            output_other_index=argument_state::i_sync;
        } else {
            return error();
        }
    } else
    //
    //maxas $x64T (uses $x32T with is_double=true)
    //
    if (name=="DADD" || name=="DFMA" || name=="DMUL") {
        //64-bit floating-point add, multiply, multiply-add
        maxas_mode="x32";
        output_mode=1;
        res.pipeline=instruction_state::pipeline_variable;
        input_size=2;
        output_size=2;
        is_double=true;
        has_reuse=true;
    } else
    //
    //maxas $shftT
    //
    if (
        name=="SHF" || name=="SHL" || name=="SHR" || name=="BFE" || name=="BFI" ||
        name=="FMNMX" || name=="FSET" || name=="IMNMX" || name=="ISET" || name=="ISCADD" || name=="ISCADD32I"
        //|| name=="R2P"
        //|| (name=="LEA" && !first_argument_is_predicate)
    ) {
        //32-bit integer shift
        maxas_mode="shft";
        res.pipeline=instruction_state::pipeline_fixed;

        /*if (name=="R2P") {
            output_mode=0; ... todo
        } else {
            output_mode=1;
        }*/
        output_mode=1;
        has_reuse=true;
    } else
    //
    //maxas $cmpT
    //
    if (
        //name=="ICMP" || name=="FCMP" || name=="HMNMX" ||
        name=="HSET2" || name=="HSETP2" ||
        name=="FSETP" ||
        name=="DSET" || name=="DSETP" || name=="DMNMX" ||
        name=="ISETP" ||
        name=="PSET" || name=="PSETP"
        //|| name=="CSET" || name=="CSETP"
        //|| (name=="LEA" && first_argument_is_predicate)
    ) {
        //compare, minimum, maximum
        maxas_mode="cmp";
        is_half=(name.at(0)=='H');
        is_double=(name.at(0)=='D');
        res.pipeline=instruction_state::pipeline_fixed;
        if (name.substr(1)=="SETP2" || name.substr(1)=="SETP") {
            output_mode=3;
        } else {
            output_mode=1;
        }

        if (is_double) {
            input_size=2;
        }
        has_reuse=true;
    } else
    //
    //maxas $qtrT
    //
    if (name=="MUFU") {
        //32-bit floating-point reciprocal, reciprocal square root, base-2 logarithm (__log2f), etc
        maxas_mode="qtr";
        res.pipeline=instruction_state::pipeline_variable;
        output_mode=1;
    } else
    if (name=="SHFL") {
        //warp shuffle
        maxas_mode="qtr";
        res.pipeline=instruction_state::pipeline_variable;
        output_mode=4;
    } else
    if (name=="I2I" || name=="I2F" || name=="F2I" || name=="F2F") {
        //Type conversions from 8-bit and 16-bit integer to 32-bit types
        //Type conversions from and to 64-bit types
        maxas_mode="qtr";
        string suffix_0=name_a.suffix.at(0).text;
        string suffix_1=name_a.suffix.at(1).text;
        if (!(suffix_0==".U16" || suffix_0==".S16" || suffix_0==".U32" || suffix_0==".S32" || suffix_0==".F32" || suffix_0==".F64")) {
            return error();
        }
        if (!(suffix_1==".U16" || suffix_1==".S16" || suffix_1==".U32" || suffix_1==".S32" || suffix_1==".F32" || suffix_1==".F64")) {
            return error();
        }
        is_double=(suffix_0==".F64" || suffix_1==".F64");
        is_half=(suffix_0==".F16" || suffix_1==".F16");
        output_size=(suffix_0==".F64" || suffix_0==".U64" || suffix_0==".S64")? 2 : 1;
        input_size=(suffix_1==".F64" || suffix_1==".U64" || suffix_1==".S64")? 2 : 1;
        output_mode=1;
        res.pipeline=instruction_state::pipeline_variable;
    } else
    //
    //maxas $rroT
    //
    if (name=="RRO") {
        //rro
        maxas_mode="rro";
        res.pipeline=instruction_state::pipeline_fixed;
        output_mode=1;
    } else
    //
    //maxas $voteT
    //
    /*if (name=="VOTE") {
        //vote
        maxas_mode="vote";
    } else*/
    {
        return error();
    }

    if (has_suffix(".X")) {
        if (output_other_index!=-1) {
            return error();
        }
        output_other_index=argument_state::i_carry;
    }

    for (int x=name_i+1;x<p_line.arguments.size();++x) {
        const argument& a=p_line.arguments[x];
        for (int y=0;y<a.suffix.size();++y) {
            if (a.suffix[y].text==".CC") {
                if (input_other_index!=-1) {
                    return error();
                }
                input_other_index=argument_state::i_carry;
            }
        }
        if (a.prefix.size()==1 && a.prefix[0].text=="CC") {
            if (name!="P2R") {
                return error();
            }
            if (input_other_index!=-1) {
                return error();
            }
            input_other_index=argument_state::i_carry;
        }
    }

    bool allow_reuse=false;

    todo //use microbenchmarks to derive most of this (including whether the instruction uses the fixed or variable pipeline)
    if (res.pipeline==-1) {
        return error();
    }
    if (maxas_mode=="s2r") {
        res.typical_read_latency=1;
        res.typical_write_latency=25;
    } else
    if (maxas_mode=="smem") {
        res.typical_read_latency=20;
        res.typical_write_latency=30;
    } else
    if (maxas_mode=="gmem") {
        res.typical_read_latency=20;
        res.typical_write_latency=200;
    } else
    if (maxas_mode=="x32") {
        res.fixed_latency=6;
        allow_reuse=true;
    } else
    if (maxas_mode=="x32_branch") {
        res.typical_read_latency=2;
        res.typical_write_latency=2;
    } else
    if (maxas_mode=="shft") {
        res.fixed_latency=6;
        allow_reuse=true;
    } else
    if (maxas_mode=="cmp") {
        res.fixed_latency=13;
        allow_reuse=true;
    } else
    if (maxas_mode=="qtr") {
        res.typical_read_latency=4;
        res.typical_write_latency=20;
    } else
    if (maxas_mode=="rro") {
        res.fixed_latency=13; todo //not sure about this
    } else
    if (maxas_mode=="vote") {
        res.typical_read_latency=20;
        res.typical_write_latency=30;
    } else {
        return error();
    }

    if ((is_half || is_double) && res.pipeline==instruction_state::pipeline_fixed) {
        res.pipeline=instruction_state::pipeline_variable;
        res.typical_read_latency=20;
        res.typical_write_latency=128;
    }

    if (res.pipeline!=instruction_state::pipeline_fixed) {
        if (res.fixed_latency!=0) {
            return error();
        }
        res.fixed_latency=2; //minimum time to stall before waiting on a barrier
    } else {
        if (res.fixed_latency==0) {
            return error();
        }
    }


    res.num_inputs=0;
    res.num_outputs=0;

    if (input_other_index!=-1) {
        argument_state a;
        a.mode=argument_state::mode_other;
        a.index=input_other_index;
        a.register_size=1;
        res.inputs.at(res.num_inputs)=a;
        ++res.num_inputs;
    }
    if (output_other_index!=-1) {
        argument_state a;
        a.mode=argument_state::mode_other;
        a.index=output_other_index;
        a.register_size=1;
        res.outputs.at(res.num_outputs)=a;
        ++res.num_outputs;
    }

    int num_outputs;
    vector<string> output_prefixes;
    if (output_mode==0) {
        num_outputs=0;
    } else
    if (output_mode==1 || output_mode==2) {
        num_outputs=1;
        output_prefixes.push_back((output_mode==1)? "R" : "P");
    } else {
        if (output_mode!=3 && output_mode!=4) {
            return error();
        }
        num_outputs=2;
        output_prefixes.push_back("P");
        output_prefixes.push_back((output_mode==4)? "R" : "P");
    }

    int n_index=-1;
    int next_fragment_index=0;
    for (int x=0;x<p_line.arguments.size();++x) {
        const auto& a=p_line.arguments[x];

        bool is_simple=false;
        if (x>name_i && a.value.size()==1) {
            ++n_index;
            if (n_index<0) {
                return error();
            }
            is_simple=true;
        }

        int register_size=1;
        bool is_output=false;

        if (is_simple) {
            if (n_index<num_outputs) {
                if (a.value[0].prefix!=output_prefixes[n_index]) {
                    return error();
                }
                is_output=true;
                register_size=output_size;
            } else {
                register_size=input_size;
            }
        } else {
            register_size=address_size;
        }

        bool is_reuse=(allow_reuse && is_simple && !is_output && a.value[0].mode==1 && a.value[0].prefix!="P");

        for (int y=0;y<a.value.size();++y) {
            int c_fragment_index=next_fragment_index;
            ++next_fragment_index;

            auto& v=a.value[y];
            if (v.mode!=1 || (v.prefix!="R" && v.prefix!="P")) {
                continue;
            }

            argument_state arg;
            arg.mode=(v.prefix=="R")? argument_state::mode_register : argument_state::mode_predicate;
            arg.index=v.value;
            arg.register_size=(arg.mode==argument_state::mode_register)? register_size : 1;
            arg.fragment_index=c_fragment_index;

            if (arg.index!=v.value) {
                return error();
            }
            if (arg.fragment_index!=c_fragment_index) {
                return error();
            }

            if (is_reuse) {
                arg.reuse_slot=reuse_offset;
                if (arg.reuse_slot!=reuse_offset) {
                    return error();
                }
            }

            if (arg.mode==argument_state::mode_register && arg.index==255) {
                continue;
            }
            if (arg.mode==argument_state::mode_predicate && arg.index==7) {
                continue;
            }

            if (is_output) {
                res.outputs.at(res.num_outputs)=arg;
                ++res.num_outputs;
            } else {
                res.inputs.at(res.num_inputs)=arg;
                ++res.num_inputs;
            }
        }

        if (is_reuse) {
            ++reuse_offset;
        }
    }

    return true;
}

//applies any changes to predicate or register indexes
void apply_instruction_state(const instruction_state& state, processed_line& p_line) {
    vector<fragment*> fragments_by_index;

    for (int x=0;x<p_line.arguments.size();++x) {
        auto& a=p_line.arguments[x];

        for (int y=0;y<a.value.size();++y) {
            fragments_by_index.push_back(&a.value[y]);
        }
    }

    auto bind=[&](argument_state a) {
        if (a.mode!=argument_state::mode_register && a.mode!=argument_state::mode_predicate) {
            return;
        }

        assert(a.index>=0);
        fragments_by_index.at(a.fragment_index)->value=a.index;
    };
}