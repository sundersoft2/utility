//not implemented:
//-__shared__ globals
//-__managed__ globals
//-__device__ functions
//-__device__ globals with default values
//-linking a cubin with a __device__ function to a cubin with a __global__ function
struct cubin_file {
    vector<cubin_kernel> kernels;
    vector<cubin_param> constant_globals;
    vector<cubin_param> device_globals;

    string output() {
        set<string> externs;
        for (cubin_kernel& k : kernels) {
            k.calculate(externs);
        }

        int constant3_alignment;
        int constant3_size=calculate_params(constant_globals, constant3_alignment);

        int global_alignment;
        int global_size=calculate_params(device_globals, global_alignment);
        sort(
            device_globals.begin(), device_globals.end(),
            [&](cubin_param& a, cubin_param& b){ return a.index<b.index; }
        );

        int cuda_version=100;
        bool EF_CUDA_TEXMODE_UNIFIED=true;
        bool EF_CUDA_64BIT_ADDRESS=true;
        int EF_CUDA_SM=61; //8 bits
        int EF_CUDA_VIRTUAL_SM=61; //8 bits
        int os_abi=0x33; //?
        int abi_version=0x7; //?

        elf_file elf;
        elf.class_=ELFCLASS64;
        elf.encoding=ELFDATA2LSB;
        elf.os_abi=os_abi;
        elf.abi_version=abi_version;
        elf.type=ET_EXEC;
        elf.machine=EM_CUDA;
        elf.version=cuda_version;
        elf.entry=0;
        elf.elf_version=1;
        elf.flags=
            uint64(EF_CUDA_SM)              <<  0ull |
            uint64(EF_CUDA_TEXMODE_UNIFIED) <<  8ull |
            uint64(EF_CUDA_64BIT_ADDRESS)   << 10ull |
            uint64(EF_CUDA_VIRTUAL_SM)      << 16ull
        ;

        for (auto& s : { "", ".shstrtab", ".strtab", ".symtab", ".symtab_shndx", ".nv.info" }) {
            elf.add_string(s, true, true);
        }

        bool added_param=false;
        bool added_sreg=false;
        bool added_globals=false;
        for (cubin_kernel& k : kernels) {
            elf.add_string(k.name, false, true);
            elf.add_string(".text."+ k.name, true, true);
            elf.add_string(".nv.info."+ k.name, true, true);
            elf.add_string(".nv.shared."+ k.name, true, true);

            if (!added_globals) {
                if (global_size!=0) {
                    elf.add_string(".nv.global", true, true);
                }
                for (cubin_param& p : device_globals) {
                    elf.add_string(p.name, false, true);
                }

                if (constant3_size!=0) {
                    elf.add_string(".nv.constant3", true, true);
                }
                for (cubin_param& p : constant_globals) {
                    elf.add_string(p.name, false, true);
                }

                for (const string& s : externs) {
                    elf.add_string(s, false, true);
                }

                added_globals=true;
            }

            if (k.has_rel) {
                elf.add_string(".rel.text."+ k.name, true, true);
            }

            //assumes extern shared variable name is "shared"
            if (k.has_shared) {
                elf.add_string("$"+ k.name +"$shared", false, true);
            }

            elf.add_string(".nv.constant0."+ k.name, true, true);

            if (k.params.size()==0 && !added_sreg) {
                elf.add_string("_SREG", false, true);
                added_sreg=true;
            }

            if (k.params.size()!=0 && !added_param) {
                elf.add_string("_param", false, true);
                added_param=true;
            }
        }

        elf.add_section(elf_section());
        const uint64 nv_info_type=0x70000000;

        {
            elf_section s;
            s.name=".shstrtab";
            s.align=0x1;
            s.type=SHT_STRTAB;
            elf.add_section(move(s));
        }

        {
            elf_section s;
            s.name=".strtab";
            s.align=0x1;
            s.type=SHT_STRTAB;
            elf.add_section(move(s));
        }

        {
            elf_section s;
            s.name=".symtab";
            s.align=0x8;
            s.type=SHT_SYMTAB;
            s.link_name=".strtab";
            //s.info_other=2*kernels.size();
            s.entry_size=sizeof(Elf64_Sym);
            elf.add_section(move(s));
        }

        {
            elf_section s;
            s.name=".nv.info";
            s.link_name=".symtab";
            s.align=0x4;
            s.type=nv_info_type;
            elf.add_section(move(s));
        }

        for (cubin_kernel& k : kernels) {
            elf_section s;
            s.name=".nv.info."+ k.name;
            s.link_name=".symtab";
            s.align=0x4;
            s.type=nv_info_type;
            s.info_name=".text."+ k.name;
            elf.add_section(move(s));
        }

        for (cubin_kernel& k : kernels) {
            if (!k.has_rel) {
                continue;
            }

            elf_section s;
            s.name=".rel.text."+ k.name;
            s.link_name=".symtab";
            s.align=0x8;
            s.type=SHT_REL;
            s.info_name=".text."+ k.name;
            s.entry_size=0x10;
            elf.add_section(move(s));
        }

        if (constant3_size!=0) {
            assert(constant3_size<=0x10000);
            elf_section s;
            s.name=".nv.constant3";
            s.align=constant3_alignment;
            s.type=SHT_PROGBITS;
            s.flags=SHF_ALLOC;
            s.data.resize(constant3_size, 0);
            elf.add_section(move(s));
        }

        for (cubin_kernel& k : kernels) {
            elf_section s;
            s.name=".nv.constant0."+ k.name;
            s.align=0x4;
            s.type=SHT_PROGBITS;
            s.info_name=".text."+ k.name;
            s.flags=SHF_ALLOC;
            s.data.resize(k.constant0_size, 0);
            elf.add_section(move(s));
        }

        for (cubin_kernel& k : kernels) {
            elf_section s;
            s.name=".text."+ k.name;
            s.link_name=".symtab";
            s.align=0x20;
            s.type=SHT_PROGBITS;
            s.info_name=k.name;
            s.info_name_is_symbol=true;
            s.info_other=uint64(k.num_registers) << 24ull;
            s.flags=
                SHF_ALLOC |
                SHF_EXECINSTR |
                (uint64(k.num_barriers) << 20ull)
            ;
            s.data=k.text;
            elf.add_section(move(s));
        }

        for (cubin_kernel& k : kernels) {
            if (!k.has_shared) {
                continue;
            }

            //size is size of shared memory (0 if dynamically allocated)
            elf_section s;
            s.name=".nv.shared."+ k.name;
            s.align=0x10;
            s.type=SHT_NOBITS;
            s.info_name=".text."+ k.name;
            s.flags=SHF_WRITE | SHF_ALLOC;
            elf.add_section(move(s));
        }

        if (global_size!=0) {
            elf_section s;
            s.name=".nv.global";
            s.align=global_alignment;
            s.type=SHT_NOBITS;
            s.flags=SHF_WRITE | SHF_ALLOC;
            s.data.resize(global_size, 0);
            elf.add_section(move(s));
        }

        {
            elf_segment s;
            s.type=PT_PHDR;
            s.flags=PF_R | PF_X;
            s.align=0x8;
            elf.segments.push_back(move(s));
        }

        {
            elf_segment s;
            s.type=PT_LOAD;
            s.flags=PF_R | PF_X;
            s.align=0x8;

            if (constant3_size!=0) {
                s.sections.push_back(".nv.constant3");
            }

            for (cubin_kernel& k : kernels) {
                s.sections.push_back(".nv.constant0."+ k.name);
            }

            for (cubin_kernel& k : kernels) {
                s.sections.push_back(".text."+ k.name);
            }

            elf.segments.push_back(move(s));
        }

        {
            elf_segment s;
            s.type=PT_LOAD;
            s.flags=PF_R | PF_W;
            s.align=0x8; //can be 0x08?

            if (global_size!=0) {
                s.sections.push_back(".nv.global");
            }
            s.mem_only=true;

            elf.segments.push_back(move(s));
        }

        {
            int num_symtab=0; //excludes null and .text entries

            elf.add_symbol(elf_symbol());

            assert(!kernels.empty());

            {
                cubin_kernel& k=kernels[0];
                elf_symbol s;
                s.name=".text." + k.name;
                s.type=STT_SECTION;
                s.bind=STB_LOCAL;
                s.section=s.name;
                elf.add_symbol(move(s));
            }

            if (global_size!=0) {
                elf_symbol s;
                s.name=".nv.global";
                s.type=STT_SECTION;
                s.bind=STB_LOCAL;
                s.section=".nv.global";
                elf.add_symbol(move(s));
                ++num_symtab;
            }

            for (cubin_param& p : device_globals) {
                elf_symbol s;
                s.name=p.name;
                s.type=STT_OBJECT;
                s.bind=STB_LOCAL;
                s.section=".nv.global";
                s.value=p.offset;
                s.size=p.size;
                elf.add_symbol(move(s));
                ++num_symtab;
            }

            if (constant3_size!=0) {
                elf_symbol s;
                s.name=".nv.constant3";
                s.type=STT_SECTION;
                s.bind=STB_LOCAL;
                s.section=".nv.constant3";
                s.size=0; //constant3_size;
                elf.add_symbol(move(s));
                ++num_symtab;
            }

            for (cubin_param& p : constant_globals) {
                elf_symbol s;
                s.name=p.name;
                s.type=STT_OBJECT;
                s.bind=STB_LOCAL;
                s.section=".nv.constant3";
                s.value=p.offset;
                s.size=p.size;
                elf.add_symbol(move(s));
                ++num_symtab;
            }

            for (int x=0;x<kernels.size();++x) {
                cubin_kernel& k=kernels[x];

                if (x!=0) {
                    elf_symbol s;
                    s.name=".text." + k.name;
                    s.type=STT_SECTION;
                    s.bind=STB_LOCAL;
                    s.section=s.name;
                    elf.add_symbol(move(s));
                }

                {
                    elf_symbol s;
                    s.name=".nv.constant0." + k.name;
                    s.type=STT_SECTION;
                    s.bind=STB_LOCAL;
                    s.section=s.name;
                    elf.add_symbol(move(s));
                    ++num_symtab;
                }
            }

            for (cubin_kernel& k : kernels) {
                elf_symbol s;
                s.name=k.name;
                s.type=STT_FUNC;
                s.bind=STB_GLOBAL;
                s.section=".text."+ s.name;
                s.other=0x10;
                s.size=k.text.size();
                elf.add_symbol(move(s));
                ++num_symtab;
            }

            for (const string& e : externs) {
                elf_symbol s;
                s.name=e;
                s.type=STT_FUNC;
                s.bind=STB_GLOBAL;
                elf.add_symbol(move(s));
                ++num_symtab;
            }

            elf.sections[elf.sections_by_name.at(".symtab")].info_other=num_symtab;
        }

        for (int x=kernels.size()-1;x>=0;--x) {
            cubin_kernel& k=kernels[x];
            k.append_file_info(elf.sections[elf.sections_by_name.at(".nv.info")].data, elf);
        }

        for (cubin_kernel& k : kernels) {
            if (k.has_rel) {
                k.append_rel(elf.sections[elf.sections_by_name.at(".rel.text."+ k.name)].data, elf);
            }
            k.append_kernel_info(elf.sections[elf.sections_by_name.at(".nv.info."+ k.name)].data, elf);
        }

        return elf.output();
    }
};