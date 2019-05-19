struct control_flags {
    //all of these are present on maxwell, pascal, volta
    //kepler has 8-bit control flags. no idea how they work. it has no reuse flags
    //(pretty sure kepler gpu works even if you omit the control flags, but there is a performance loss)
    array<bool, 4> reuse_flags={false, false, false, false};
    int stall_count=1;
    bool yield_flag=false;
    int write_barrier=-1;
    int read_barrier=-1;
    array<bool, 6> wait_flags={false, false, false, false, false, false};

    void verify() const {
        assert(stall_count>=0 && stall_count<=15);
        //assert(stall_count<=11 || yield_flag);
        assert(write_barrier>=-1 && write_barrier<=5);
        assert(read_barrier>=-1 && read_barrier<=5);
    }

    void set_stall(int n) {
        assert(n>=0 && n<=15);
        stall_count=n;
        if (n>=12) {
            yield_flag=true;
        }
    }

    string output() const {
        string res;

        res+=(read_barrier!=-1)? to_string(read_barrier) : " ";
        res+=":";

        res+=(write_barrier!=-1)? to_string(write_barrier) : " ";
        res+=":";

        for (int x=0;x<6;++x) {
            res+=(wait_flags[x])? "W" : " ";
        }
        res+=":";

        for (int x=0;x<4;++x) {
            res+=(reuse_flags[x])? "R" : " ";
        }
        res+=":";

        res+=(yield_flag)? "Y" : " ";
        res+=":";

        if (stall_count<=9) {
            res+=" ";
        }
        res+=to_string(stall_count);

        return res;
    }

    //return rest of s
    string input(const string& s) {
        // "R:W:wwwwww:RRRR:Y:SS"

        int pos=0;
        auto get=[&]() {
            char r=s.at(pos);
            ++pos;
            return r;
        };

        auto parse_int=[&](int max) {
            char r=get();
            if (r==' ') {
                return -1;
            } else {
                int v=r-'0';
                assert(v>=0 && v<=max);
                return v;
            }
        };

        auto parse_bool=[&](char v_true) {
            char r=get();
            assert(r==' ' || r==v_true);
            return r==v_true;
        };

        read_barrier=parse_int(5);
        assert(get()==':');

        write_barrier=parse_int(5);
        assert(get()==':');

        for (int x=0;x<6;++x) {
            wait_flags[x]=parse_bool('W');
        }
        assert(get()==':');

        for (int x=0;x<4;++x) {
            reuse_flags[x]=parse_bool('R');
        }
        assert(get()==':');

        yield_flag=parse_bool('Y');
        assert(get()==':');

        stall_count=parse_bool('1')? 10 : 0;
        stall_count+=parse_int(9);

        return s.substr(pos);
    }
};

typedef uint64 opcode_type; todo

struct instruction {
    enum mode_type {
        mode_none,
        mode_rel_jcal,      //R_CUDA_ABS32_20
        mode_rel_mov32i_lo, //R_CUDA_ABS32_LO_20
        mode_rel_mov32i_hi, //R_CUDA_ABS32_HI_20
        mode_shfl,
        mode_s2r_ctaid_x,
        mode_s2r_ctaid_y,
        mode_s2r_ctaid_z,
        mode_exit
    };

    char mode=mode_none;
    string symbol_name; //used with rel modes and MOV32I or JCAL instructions with a 0 immediate
    opcode_type opcode=0;

    control_flags c_flags;
};

array<control_flags, 3> decode_control_flags_maxwell(uint64 control_code) {
    assert((control_code & 0x8000000000000000ull) == 0);

    array<control_flags, 3> res;

    array<uint64, 3> control_shift={0, 21, 42};
    array<uint64, 3> reuse_shift={17, 38, 59};

    for (int x=0;x<3;++x) {
        control_flags f;
        int c=(control_code >> control_shift[x]) & 0x1ffff;
        int r=(control_code >> reuse_shift[x]) & 0xf;

        for (int y=0;y<4;++y) {
            f.reuse_flags[y]=(r & (1<<y))!=0;
        }

        f.stall_count  =(c & 0x0000f) >> 0;
        f.yield_flag   =(c & 0x00010) >> 4;
        f.write_barrier=(c & 0x000e0) >> 5;
        f.read_barrier =(c & 0x00700) >> 8;
        int wait_flags =(c & 0x1f800) >> 11;

        for (int y=0;y<6;++y) {
            f.wait_flags[y]=(wait_flags & (1<<y))!=0;
        }

        f.yield_flag=!f.yield_flag;

        if (f.write_barrier==7) {
            f.write_barrier=-1;
        }

        if (f.read_barrier==7) {
            f.read_barrier=-1;
        }

        f.verify();
        res[x]=f;
    }

    return res;
}

uint64 encode_control_flags_maxwell(array<control_flags, 3> flags) {
    uint64 control_code=0;

    array<uint64, 3> control_shift={0, 21, 42};
    array<uint64, 3> reuse_shift={17, 38, 59};

    for (int x=0;x<3;++x) {
        control_flags f=flags[x];
        f.verify();

        f.yield_flag=!f.yield_flag;

        if (f.write_barrier==-1) {
            f.write_barrier=7;
        }

        if (f.read_barrier==-1) {
            f.read_barrier=7;
        }

        int wait_flags=0;
        for (int y=0;y<6;++y) {
            if (f.wait_flags[y]) {
                wait_flags|=(1<<y);
            }
        }

        int c=0;
        int r=0;

        c|=f.stall_count     << 0;
        c|=int(f.yield_flag) << 4;
        c|=f.write_barrier   << 5;
        c|=f.read_barrier    << 8;
        c|=wait_flags        << 11;

        for (int y=0;y<4;++y) {
            if (f.reuse_flags[y]) {
                r|=(1<<y);
            }
        }

        control_code|=uint64(c) << uint64(control_shift[x]);
        control_code|=uint64(r) << uint64(reuse_shift[x]);
    }

    return control_code;
}

int index_to_absolute_maxwell(int index, bool after_control_code) {
    assert(index>=0);
    int block=index/3;
    int offset=index%3;

    return (block*4 + ((offset==0)? (after_control_code? 1 : 0) : offset+1))*8;
}

bool is_control_code_maxwell(int offset) {
    return offset%32==0;
}

//cuda requirements are 32 byte alignment
//the loader aligns to 128 bytes
//the instruction cache line size is 128 or 256 bytes (not sure)
const int maxwell_default_alignment=32;

bool is_aligned_maxwell(int size, int alignment) {
    assert(alignment%32==0);
    int bytes=index_to_absolute_maxwell(size, false);
    return bytes%alignment==0;
}

string encode_instructions_maxwell(const vector<instruction>& instructions) {
    string res;

    assert(instructions.size()%3==0);
    int x=0;
    for (x=0;x<instructions.size();x+=3) {
        array<control_flags, 3> flags={
            instructions.at(x+0).c_flags,
            instructions.at(x+1).c_flags,
            instructions.at(x+2).c_flags
        };

        append_string(res, (uint64)encode_control_flags_maxwell(flags));
        for (int y=0;y<3;++y) {
            append_string(res, (uint64)instructions[x+y].opcode);
        }
    }
    assert(x==instructions.size());

    return res;
}

//note: nvdisasm running on a cubin file doesn't work because some instructions are hidden and the labels are in the wrong spot
//should extract the code section into a raw file then run nvdisasm on the raw file to avoid this
//
//if two instructions are dual issued and there is a control code between them, nvdisasm 9.0 will get the order wrong and put
//an instruction text next to a control opcode. nvdisasm 10.0 will put the instruction and control code on separate lines

struct nvdisasm_output {
    string instruction;
    int index=-1;
    opcode_type opcode=0;
    control_flags c_flags;
};

vector<nvdisasm_output> parse_nvdisasm_maxwell(string data) {
    vector<pair<string, int>> instructions_text;
    vector<uint64> instructions_uint64;
    vector<control_flags> c_flags;

    int next_offset=0;
    int next_instruction_index=0;
    size_t c_pos=0;
    while (c_pos!=string::npos) {
        size_t next_pos=data.find('\n', c_pos);
        string c=data.substr(c_pos, (next_pos==string::npos)? string::npos : next_pos-c_pos);
        c_pos=(next_pos==string::npos)? string::npos : next_pos+1;

        if (is_headerflags_line(c)) {
            //supports multiple nvdisasm outputs concatenated together
            next_instruction_index=0;
            continue;
        }

        uint64 c_uint64=0;
        bool has_uint64=extract_uint64(c, c_uint64);
        if (has_uint64) {
            if (is_control_code_maxwell(next_offset)) {
                array<control_flags, 3> v=decode_control_flags_maxwell(c_uint64);
                for (int x=0;x<3;++x) {
                    c_flags.push_back(v[x]);
                }
            } else {
                instructions_uint64.push_back(c_uint64);
            }
            next_offset+=8;
        }

        bool is_instruction=is_instruction_line(c);
        if (is_instruction) {
            instructions_text.push_back(make_pair(c, next_instruction_index));
            ++next_instruction_index;
        }
    }

    assert(instructions_uint64.size()==instructions_text.size());
    assert(c_flags.size()==instructions_text.size());

    vector<nvdisasm_output> res;

    for (int x=0;x<instructions_text.size();++x) {
        nvdisasm_output c;

        c.instruction=instructions_text[x].first;
        c.index=instructions_text[x].second;
        c.opcode=instructions_uint64[x];
        c.c_flags=c_flags[x];

        res.push_back(c);
    }

    return res;
}