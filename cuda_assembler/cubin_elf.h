//this uses the elfio library
// http://elfio.sourceforge.net/

struct elf_section {
    string name;
    string link_name;
    string info_name;
    bool info_name_is_symbol=false;
    uint64 info_other=0;

    uint64 type=0;
    uint64 flags=0;
    uint64 align=0;
    uint64 address=0;
    uint64 entry_size=0;

    string data;
    uint64 file_start=0;
    uint64 file_end=0;
};

struct elf_segment {
    vector<string> sections;

    uint64 type=0;
    uint64 flags=0;
    uint64 align=0;
    uint64 virtual_address=0;
    uint64 physical_address=0;

    bool mem_only=false;
};

struct elf_symbol {
    string name;
    string section;

    uint64 value=0;
    uint64 size=0;
    uint64 type=0;
    uint64 bind=0;
    uint64 other=0;
};

struct elf_file {
    uint64 class_=0;
    uint64 encoding=0;
    uint64 elf_version=0;
    uint64 type=0;
    uint64 machine=0;
    uint64 version=0;
    uint64 os_abi=0;
    uint64 abi_version=0;
    uint64 flags=0;
    uint64 entry=0;

    uint64 file_sections_start=0;
    uint64 file_sections_end=0;

    uint64 file_segments_start=0;
    uint64 file_segments_end=0;

    int last_segment_end=0;

    vector<string> shstrtab;
    map<string, int> shstrtab_pos;

    vector<string> strtab;
    map<string, int> strtab_pos;

    vector<elf_section> sections;
    map<string, int> sections_by_name;

    vector<elf_segment> segments;

    vector<elf_symbol> symbols;
    map<string, int> symbols_by_name;

    //
    //

    void add_string(const string& s, bool is_shstrtab, bool is_strtab) {
        if (is_shstrtab) {
            shstrtab.push_back(s);
        }
        if (is_strtab) {
            strtab.push_back(s);
        }
    }

    void add_section(elf_section&& s) {
        assert(sections_by_name.emplace(s.name, sections.size()).second);
        sections.push_back(move(s));
    }

    void add_symbol(elf_symbol&& s) {
        assert(symbols_by_name.emplace(s.name, symbols.size()).second);
        symbols.push_back(move(s));
    }

    string calculate_strings(const vector<string>& strings, map<string, int>& pos) {
        assert(pos.empty());
        string res;
        for (const string& c : strings) {
            int p=res.size();
            for (char d : c) {
                assert(d!='\0');
                res.push_back(d);
            }
            res.push_back('\0');

            assert(pos.emplace(c, p).second);
        }
        return res;
    }

    void calculate_symbol(string& res, elf_symbol& c) {
        uint64 name_int=strtab_pos.at(c.name);
        uint64 shndx_int=sections_by_name.at(c.section);

        Elf64_Sym entry;
        fill_n((char*)&entry, sizeof(entry), 0);
        entry.st_name  = name_int;
        entry.st_info  = ELF_ST_INFO(c.bind, c.type);
        entry.st_other = c.other;
        entry.st_shndx = shndx_int;
        entry.st_value = c.value;
        entry.st_size  = c.size;
        append_string(res, entry);
    }

    void calculate_section(string& res, elf_section& c) {
        uint64 name_int=shstrtab_pos.at(c.name);
        uint64 link=sections_by_name.at(c.link_name);
        uint64 info=(c.info_name_is_symbol? symbols_by_name : sections_by_name).at(c.info_name);
        info|=c.info_other;

        Elf64_Shdr header;
        fill_n((char*)&header, sizeof(header), 0);
        header.sh_name      = name_int;
        header.sh_type      = c.type;
        header.sh_flags     = c.flags;
        header.sh_addr      = c.address;
        header.sh_offset    = c.file_start;
        header.sh_size      = c.file_end-c.file_start;
        header.sh_link      = link;
        header.sh_info      = info;
        header.sh_addralign = c.align;
        header.sh_entsize   = c.entry_size;
        append_string(res, header);
    }

    void calculate_segment(string& res, elf_segment& c) {
        int start=-1;
        int size=-1;

        int pos=-1;
        for (string& s_name : c.sections) {
            elf_section& s=sections.at(sections_by_name.at(s_name));

            if (pos!=-1) {
                while (pos%s.align!=0) {
                    ++pos;
                }
            }

            assert(pos==-1 || pos==s.file_start);
            pos=s.file_start;

            if (start==-1) {
                assert(size==-1);
                start=pos;
                size=0;
            }
            assert(size!=-1);

            pos+=s.data.size();
            size+=s.data.size(); //this ignores padding because ptxas has a bug in it and also ignores padding

            while (pos%s.align!=0) {
                ++pos;
            }
        }

        if (c.type==PT_PHDR) {
            assert(start==-1 && size==-1 && pos==-1);
            start=file_segments_start;
            size=sizeof(Elf64_Phdr)*segments.size();
        }

        Elf64_Phdr ph;
        fill_n((char*)&ph, sizeof(ph), 0);
        ph.p_type   = c.type;
        ph.p_flags  = c.flags;
        ph.p_offset = (start==-1)? last_segment_end : start;
        ph.p_vaddr  = c.virtual_address;
        ph.p_paddr  = c.physical_address;
        ph.p_filesz = (c.mem_only || size==-1)? 0 : size;
        ph.p_memsz  = (size==-1)? 0 : size;
        ph.p_align  = c.align;
        append_string(res, ph);

        if (start!=-1) {
            assert(size!=-1);
            last_segment_end=start+size;
        }
    }

    string calculate_header() {
        Elf64_Ehdr header;
        fill_n((char*)&header, sizeof(header), 0);

        header.e_ident[EI_MAG0]         = ELFMAG0;
        header.e_ident[EI_MAG1]         = ELFMAG1;
        header.e_ident[EI_MAG2]         = ELFMAG2;
        header.e_ident[EI_MAG3]         = ELFMAG3;
        header.e_ident[EI_CLASS]        = class_;
        header.e_ident[EI_DATA]         = encoding;
        header.e_ident[EI_VERSION]      = elf_version;
        header.e_ident[EI_OSABI]        = os_abi;
        header.e_ident[EI_ABIVERSION]   = abi_version;
        header.e_type                   = type;
        header.e_machine                = machine;
        header.e_version                = version;
        header.e_entry                  = entry;
        header.e_phoff                  = file_segments_start;
        header.e_shoff                  = file_sections_start;
        header.e_flags                  = flags;
        header.e_ehsize                 = sizeof(header);
        header.e_phentsize              = sizeof(Elf64_Phdr);
        header.e_phnum                  = segments.size();
        header.e_shentsize              = sizeof(Elf64_Shdr);
        header.e_shnum                  = sections.size();
        header.e_shstrndx               = sections_by_name.at(".shstrtab");

        string res;
        append_string(res, header);
        return res;
    }

    string output() {
        string shstrtab_data=calculate_strings(shstrtab, shstrtab_pos);
        string strtab_data=calculate_strings(strtab, strtab_pos);

        string res=calculate_header(); //placeholder

        for (elf_section& s : sections) {
            if (s.name==".shstrtab") {
                assert(s.data.empty() && !shstrtab_data.empty());
                s.data=move(shstrtab_data);
                shstrtab_data.clear();
            } else
            if (s.name==".strtab") {
                assert(s.data.empty() && !strtab_data.empty());
                s.data=move(strtab_data);
                strtab_data.clear();
            } else
            if (s.name==".symtab") {
                assert(s.data.empty());
                for (elf_symbol& c : symbols) {
                    calculate_symbol(s.data, c);
                }
            }

            if (s.align!=0) {
                while ((res.size()%s.align)!=0) {
                    res.push_back('\0');
                }
            }

            if (s.type!=0) {
                s.file_start=res.size();
                s.file_end=res.size()+s.data.size();
                if (s.type!=SHT_NOBITS) {
                    res+=s.data;
                }
            } else {
                assert(s.data.empty());
            }
        }

        file_sections_start=res.size();
        for (elf_section& c : sections) {
            calculate_section(res, c);
        }
        file_sections_end=res.size();

        file_segments_start=res.size();
        for (elf_segment& c : segments) {
            calculate_segment(res, c);
        }
        file_segments_end=res.size();

        {
            string d=calculate_header();
            overwrite_string(res, 0, d);
        }

        return res;
    }
};