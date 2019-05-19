struct cubin_param {
    string name;
    int size=1;
    int alignment=1;
    int index=-1; //only used for __device__ globals

    //calculate
    int offset=-1;
};

//returns total size
int calculate_params(vector<cubin_param>& params, int& alignment) {
    int params_size=0;
    alignment=1;
    for (cubin_param& p : params) {
        assert(p.alignment>=1 && p.size>=1);
        while (params_size%(p.alignment)!=0) {
            ++params_size;
        }

        //assumes alignments are powers of 2
        if (alignment<p.alignment) {
            alignment=p.alignment;
        }

        assert(p.offset==-1 || p.offset==params_size);
        p.offset=params_size;

        params_size+=p.size;
    }

    return params_size;
}

struct cubin_kernel {
    string name;
    int num_registers=0;
    int num_barriers=0;
    bool has_shared=false; //dynamically allocated

    uint32 max_stack_size=0;
    uint32 min_stack_size=0;
    uint32 frame_size=0;

    int max_registers=-1;
    int required_num_tids=-1;
    int max_threads=-1;
    int crs_stack_size=-1;

    vector<cubin_param> params;

    vector<instruction> instructions;

    //calculated
    int constant0_size=-1;
    int params_size=-1;
    string text;
    vector<uint32> shfl_offsets;
    vector<uint32> exit_offsets;
    vector<uint32> s2r_ctaid_offsets;
    bool ctaid_z_used=false;
    set<string> externs;

    bool has_rel=false;

    void calculate(set<string>& all_externs) {
        assert(constant0_size==-1 && params_size==-1);
        int params_alignment;
        params_size=calculate_params(params, params_alignment);
        constant0_size=params_initial_offset + params_size;

        //
        //

        for (int x=0;x<instructions.size();++x) {
            instruction& i=instructions[x];
            int i_offset=index_to_absolute_maxwell(x, true); //before instruction (can't be a control code)

            if (
                i.mode==instruction::mode_rel_jcal ||
                i.mode==instruction::mode_rel_mov32i_lo ||
                i.mode==instruction::mode_rel_mov32i_hi
            ) {
                if (i.mode==instruction::mode_rel_jcal) {
                    all_externs.insert(i.symbol_name);
                    externs.insert(i.symbol_name);
                }
                has_rel=true;
            } else
            if (
                i.mode==instruction::mode_s2r_ctaid_x ||
                i.mode==instruction::mode_s2r_ctaid_y ||
                i.mode==instruction::mode_s2r_ctaid_z
            ) {
                s2r_ctaid_offsets.push_back(i_offset);
                if (i.mode==instruction::mode_s2r_ctaid_z) {
                    ctaid_z_used=true;
                }
            } else
            if (i.mode==instruction::mode_shfl) {
                shfl_offsets.push_back(i_offset);
            } else
            if (i.mode==instruction::mode_exit) {
                exit_offsets.push_back(i_offset);
            }
        }

        text=encode_instructions_maxwell(instructions);
    }

    void append_rel(string& res, elf_file& elf) {
        assert(has_rel);

        for (int x=0;x<instructions.size();++x) {
            instruction& i=instructions[x];
            int i_offset=index_to_absolute_maxwell(x, true); //before instruction (can't be a control code)

            uint32_t mode=0;
            if (i.mode==instruction::mode_rel_jcal) {
                mode=0x2a; //R_CUDA_ABS32_20
            } else
            if (i.mode==instruction::mode_rel_mov32i_lo) {
                mode=0x2b; //R_CUDA_ABS32_LO_20
            } else
            if (i.mode==instruction::mode_rel_mov32i_hi) {
                mode=0x2c; //R_CUDA_ABS32_HI_20
            } else {
                continue;
            }

            append_string(res, (uint64)i_offset);
            append_string(res, (uint32)mode);
            append_string(res, (uint32)elf.symbols_by_name.at(i.symbol_name));
        }
    }

    void append_file_info(string& res, elf_file& elf) {
        nv_info_entry().max_stack_size(name, max_stack_size).append(res, elf);
        nv_info_entry().min_stack_size(name, min_stack_size).append(res, elf);
        nv_info_entry().frame_size(name, frame_size).append(res, elf);
    }

    void append_kernel_info(string& res, elf_file& elf) {
        nv_info_entry().header().append(res, elf);

        assert(params_size!=-1);

        if (!params.empty()) {
            nv_info_entry().param_cbank(".nv.constant0."+ name, params_size).append(res, elf);
            nv_info_entry().param_cbank_size(params_size).append(res, elf);
        }

        for (int x=params.size()-1;x>=0;--x) {
            cubin_param& p=params[x];
            nv_info_entry().param_info(x, p.offset, p.size).append(res, elf);
        }

        nv_info_entry().max_reg_count((max_registers==-1)? num_registers : max_registers).append(res, elf);

        if (!shfl_offsets.empty()) {
            nv_info_entry().shfl_offsets(shfl_offsets).append(res, elf);
        }

        if (!exit_offsets.empty()) {
            nv_info_entry().exit_offsets(exit_offsets).append(res, elf);
        }

        if (!s2r_ctaid_offsets.empty()) {
            nv_info_entry().s2r_ctaid_offsets(s2r_ctaid_offsets).append(res, elf);
        }

        if (ctaid_z_used) {
            nv_info_entry().ctaid_z_used().append(res, elf);
        }

        if (required_num_tids!=-1) {
            nv_info_entry().required_num_tids(required_num_tids).append(res, elf);
        }

        if (max_threads!=-1) {
            nv_info_entry().max_threads(max_threads).append(res, elf);
        }

        if (crs_stack_size!=-1) {
            nv_info_entry().crs_stack_size(crs_stack_size).append(res, elf);
        }

        vector<uint32> externs_int;
        for (const string& s : externs) {
            externs_int.push_back(elf.symbols_by_name.at(s));
        }
        if (!externs_int.empty()) {
            nv_info_entry().externs(externs_int).append(res, elf);
        }
    }
};