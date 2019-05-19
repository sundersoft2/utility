/*template<class func_type> void iterate_out_file(string file, func_type func) {
    ifstream in(file);
    assert(in.good());

    int mode=0;
    int index=-1;
    while (true) {
        string s;
        getline(in, s);
        if (s.empty() && in.eof()) {
            break;
        }

        if (s=="OPCODE_SCAN_REDUCED") {
            assert(mode==0 || mode==5);
            ++index;
            mode=1;
        } else
        if (s=="OPCODE_SCAN_NUMBER") {
            assert(mode==1);
            mode=2;
        } else
        if (s=="OPCODE_SCAN_NORMALIZED") {
            assert(mode==2);
            mode=3;
        } else
        if (s=="OPCODE_SCAN_HYBRID") {
            assert(mode==3);
            mode=4;
        } else
        if (s=="OPCODE_SCAN_DONE") {
            assert(mode==4);
            mode=5;
        } else {
            string name;
            uint64 opcode;
            vector<fragment> f=parse_disassembled(s, -1, name, opcode);
            if (!func(index, mode, name, opcode, f)) {
                return;
            }
        }
    }
    assert(mode==5);
} */

bool extract_uint64(const string& c, uint64& res) {
    size_t pos_start=c.find("/* 0x");
    size_t pos_end=c.find(" */");

    bool valid=(
        pos_start!=string::npos &&
        pos_end!=string::npos &&
        pos_end>pos_start &&
        pos_end+3==c.size()
    );

    res=true;
    if (valid) {
        istringstream ss( c.substr(pos_start+5, pos_end-(pos_start+5)) );
        ss >> hex >> res >> dec;
        assert(!ss.fail());
    }

    return valid;
}

bool is_instruction_line(const string& c) {
    //cuda 10 nvdisasm does not output a semicolon for the second line in a block of dual issued lines
    return c.find(';')!=string::npos || c.find('}')!=string::npos;
}

bool is_headerflags_line(const string& c) {
    return c.find(".headerflags")!=string::npos;
}