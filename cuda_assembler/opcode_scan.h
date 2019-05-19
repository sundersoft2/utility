/*vector<pair<uint64, vector<fragment>>> found_names_buffer;
void found_name(uint64 opcode, string& name, vector<fragment>& f) {
    static set<string> found_names;

    if (name.empty() || found_names.count(name)) {
        return;
    }
    found_names.insert(name);

    found_names_buffer.emplace_back(opcode, f);
}

void found_names_flush(ostream& out) {
    for (auto& c : found_names_buffer) {
        print_opcode(out, c.second, c.first);
        out.flush();
    }
}

shared_ptr<FILE> exec(string cmd) {
    shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    return pipe;
}

string exec_finish(shared_ptr<FILE> pipe) {
    array<char, 128> buffer;
    string result;
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    return result;
}

vector<string> disassemble(vector<uint64> opcodes) {
    const uint64 header=0x001F8000FC0007E0ull;

    if (!opcodes.empty()) {
        while (opcodes.size()%3!=0) {
            opcodes.push_back(opcodes.back());
        }
    }

    ostringstream path;
    path << "/tmp/opcode_scan.tmp." << getpid();

    ofstream instructions(path.str().c_str(), ios::binary);

    for (int x=0;x<opcodes.size();x+=3) {
        instructions.write((char*)&header, 8);
        instructions.write((char*)&opcodes[x+0], 8);
        instructions.write((char*)&opcodes[x+1], 8);
        instructions.write((char*)&opcodes[x+2], 8);
    }
    instructions.close();

    if (opcodes.empty()) {
        return {};
    }

    ostringstream command;
    command << "nvdisasm --print-instruction-encoding --binary SM61 /tmp/opcode_scan.tmp." << getpid() << " 2>/dev/null";

    string s=exec_finish(exec(command.str()));
    debug_print("NVDISASM\n", s);

    uint64 address=0;

    vector<string> res;
    size_t end_pos=0;
    while (true) {
        size_t start_pos=end_pos+1;
        end_pos=s.find("\n", start_pos);
        if (end_pos==string::npos) {
            break;
        }

        res.push_back(s.substr(start_pos, end_pos-start_pos));
    }

    return res;
}

string check_valid(uint64 opcode, bool* is_normalized=nullptr, vector<fragment>* t_f=nullptr) {
    if (is_normalized) {
        *is_normalized=false;
    }
    if (t_f) {
        t_f->clear();
    }

    vector<string> lines=disassemble({opcode});

    for (string& s : lines) {
        string name;
        uint64 t_opcode;
        vector<fragment> f=parse_disassembled(s, 0, name, t_opcode);
        if (name.empty()) {
            continue;
        }

        found_name(opcode, name, f);

        if (is_normalized) {
            *is_normalized=get_is_normalized(f);
        }

        if (t_f) {
            *t_f=f;
        }
        return name;
    }

    return "";
}

void process_file(uint64 source_opcode, string expected_name="") {
    bool reduced_is_normalized=false;
    uint64 reduced_opcode=source_opcode;
    vector<fragment> reduced_fragments;
    string name;

    name=check_valid(source_opcode, &reduced_is_normalized, &reduced_fragments);

    if (expected_name!="") {
        assert(name==expected_name);
    }

    if (name=="") {
        cerr << "Invalid\n";
        return;
    }

    for (uint64 x=0;x<64;++x) {
        uint64 mask=(1ull << x);
        if (!(reduced_opcode & mask)) {
            continue;
        }

        uint64 new_opcode=(reduced_opcode & (~mask));
        bool is_normalized;
        vector<fragment> fragments;
        string reduced_name=check_valid(new_opcode, &is_normalized, &fragments);
        if (reduced_name==name) {
            reduced_opcode=new_opcode;
            reduced_is_normalized=is_normalized;
            reduced_fragments=fragments;

            if (debug) {
                debug_print("NEW REDUCED_OPCODE");
                print_opcode(cerr, reduced_fragments, reduced_opcode);
            }
        }
    }

    //not 100% sure this will work for all instructions
    assert(reduced_is_normalized);

    ostringstream out_name;
    //out_name << "out_opcode_scan_single." << name << ".txt";
    out_name << "out_opcode_scan_single.";
    print_hex(out_name, reduced_opcode);
    out_name << ".txt";
    ofstream out(out_name.str(), ios::app);

    if (out.tellp()!=0) {
        //was already calculated
        cerr << "Already calculated (2)\n";
        return;
    }

    vector<pair<uint64, vector<fragment>>> normalized_opcodes; //excludes reduced_opcode
    vector<pair<uint64, vector<fragment>>> number_opcodes;

    for (uint64 x=0;x<64;++x) {
        uint64 mask=(1ull << x);
        if (reduced_opcode & mask) {
            continue;
        }

        uint64 new_opcode=(reduced_opcode | mask);
        bool is_normalized;
        vector<fragment> fragments;
        string new_name=check_valid(new_opcode, &is_normalized, &fragments);
        if (new_name==name) {
            (is_normalized? normalized_opcodes : number_opcodes).emplace_back(new_opcode, fragments);
        }
    }

    out_name << ".found";
    ofstream found_names_out(out_name.str());

    found_names_out << "OPCODE_SCAN_FIND_NAMES\n";
    found_names_flush(found_names_out);

    out << "OPCODE_SCAN_REDUCED\n";
    print_opcode(out, reduced_fragments, reduced_opcode);

    out << "OPCODE_SCAN_NUMBER\n";
    for (auto& c : number_opcodes) {
        print_opcode(out, c.second, c.first);
    }

    out << "OPCODE_SCAN_NORMALIZED\n";
    for (auto& c : normalized_opcodes) {
        print_opcode(out, c.second, c.first);
    }

    out << "OPCODE_SCAN_HYBRID\n";
    out.flush();

    if (normalized_opcodes.size()>=24) {
        cerr << "WARNING: Reducing number of normalized opcodes because there are too many of them: " << normalized_opcodes.size() << "\n";

        //remove anything that doesn't change the instruction text
        //this includes all of the unused bits but could remove a bit that is used for something
        vector<pair<uint64, vector<fragment>>> new_normalized_opcodes;
        for (auto& c : normalized_opcodes) {
            if (c.second!=reduced_fragments) {
                new_normalized_opcodes.push_back(c);
            }
        }
        normalized_opcodes=new_normalized_opcodes;

        if (normalized_opcodes.size()>=24) {
            cerr << "ERROR: Still too many normalized opcodes: " << normalized_opcodes.size() << "\n";
            assert(false);
        }
    }

    vector<uint64> hybrid_opcodes_buffer;
    //runs in exponential time; there are some ways to do it in quadratic or cubic time

    auto flush_hybrid_buffer=[&]() {
        if (hybrid_opcodes_buffer.empty()) {
            return;
        }

        vector<pair<uint64, vector<fragment>>> hybrid_opcodes;
        vector<string> lines=disassemble(hybrid_opcodes_buffer);
        int index=0;
        for (string& s : lines) {
            string s_name;
            uint64 s_opcode;
            vector<fragment> f=parse_disassembled(s, index, s_name, s_opcode);
            if (s_name.empty()) {
                continue;
            }

            ++index;

            if (s_name!=name) {
                continue;
            }

            hybrid_opcodes.emplace_back(s_opcode, f);
        }

        for (auto& c : hybrid_opcodes) {
            print_opcode(out, c.second, c.first);
        }

        out.flush();
        hybrid_opcodes_buffer.clear();
    };

    for (int x=0;x<(1<<normalized_opcodes.size());++x) {
        uint64 code=0;
        int num=0;
        for (int y=0;y<normalized_opcodes.size();++y) {
            if (!(x&(1<<y))) {
                continue;
            }

            code|=normalized_opcodes[y].first;
            ++num;
        }

        if (num<2) {
            continue;
        }

        hybrid_opcodes_buffer.push_back(code);
        if (hybrid_opcodes_buffer.size()>100000) {
            flush_hybrid_buffer();
        }
    }
    flush_hybrid_buffer();

    out << "OPCODE_SCAN_DONE\n";
    out.flush();

    cerr << "Calculated\n";
}

void random_opcodes_file(unsigned int seed) {
    ostringstream out_name;
    out_name << "out_opcode_scan_rand." << seed << ".txt.found";
    ofstream found_names_out(out_name.str());
    found_names_out << "OPCODE_SCAN_FIND_NAMES\n";

    mt19937_64 random;
    random.seed(seed);

    int num_searched=0;
    while(true) {
        //todo: for volta, need to initialize the control bits to some predefined value
        uint64 opcode=random();
        check_valid(opcode);

        ++num_searched;
        if (num_searched%100==0) {
            cerr << "Num_searched: " << num_searched << "\n";
        }

        found_names_flush(found_names_out);
    }
}

void index_opcodes_file(string self_path, vector<string> files) {
    set<string> found_names;
    vector<pair<uint64, string>> pending;

    for (string file : files) {
        ifstream in(file);
        assert(in.good());

        while (true) {
            string s;
            getline(in, s);
            if (s.empty() && in.eof()) {
                break;
            }

            string name;
            uint64 opcode;
            vector<fragment> f=parse_disassembled(s, -1, name, opcode);

            if (found_names.count(name)) {
                continue;
            }
            found_names.insert(name);

            pending.emplace_back(opcode, name);
        }
    }

    cerr << "Index_opcodes: num=" << pending.size() << "\n";
    for (auto& c : pending) {
        ostringstream ss;
        ss << self_path << " single ";
        print_hex(ss, c.first);
        ss << " " << c.second;

        system(ss.str().c_str()); //can make this multithreaded if i can be fucked
        //process(c.first, c.second); //for testing
    }
}

void join_opcodes_file(ostream& out, string file) {
    const int num_modes=6;
    array<int, num_modes> num_in;
    for (int x=0;x<num_modes;++x) {
        num_in[x]=0;
    }

    map<vector<fragment>, pair<uint64, int>> found_opcodes;
    string common_name;

    iterate_out_file(file, [&](int index, int mode, string& name, uint64 opcode, vector<fragment>& f) {
        assert(index==0);
        ++num_in.at(mode);

        assert(!name.empty());
        assert(common_name.empty() || name==common_name);
        assert((!get_is_normalized(f)) == (mode==2));
        common_name=name;

        bool valid=true;
        for (fragment& c : f) {
            if (
                c.text.find("INVALID")!=string::npos ||
                c.text.find("?")!=string::npos
            ) {
                valid=false;
            }
        }

        //can only remove invalid hybrids
        if (mode!=4 || valid) {
            auto& c=found_opcodes.emplace(f, make_pair(opcode, mode)).first->second;
            if (num_set_bits(opcode)<num_set_bits(c.first)) {
                c.first=opcode;
                c.second=mode;
            }
        }

        return true;
    });

    assert(num_in[0]==0); //before start
    assert(num_in[1]==1); //reduced
    assert(num_in[2]>=0); //number
    assert(num_in[3]>=0); //normalized
    assert(num_in[4]>=0); //hybrid
    assert(num_in[5]==0); //done

    {
        int expected=(1<<num_in[3])-1-num_in[3];
        int actual=num_in[4];
        if (expected>actual) {
            //this gets hit if there are more than 24 normalized opcodes since it will get reduced in that case
            cerr << "WARNING: Hybrid expected: " << expected << ", actual: " << actual << "\n";
        }
    }

    auto output=[&](int mode, string name) {
        out << "OPCODE_SCAN_" << name << "\n";
        for (auto& c : found_opcodes) {
            if (c.second.second==mode) {
                print_opcode(out, c.first, c.second.first);
            }
        }
    };

    output(1, "REDUCED");
    output(2, "NUMBER");
    output(3, "NORMALIZED");
    output(4, "HYBRID");
    out << "OPCODE_SCAN_DONE\n";
} */