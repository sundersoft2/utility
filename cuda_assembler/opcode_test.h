/*void parse_file(string in_file) {
    ifstream in(in_file);
    while (true) {
        string s;
        getline(in, s);
        if (s.empty() && in.eof()) {
            break;
        }

        string name;
        uint64 opcode;
        vector<fragment> f=parse_disassembled(s, 0, name, opcode);

        cout << "::" << s << "\n";
        cout << name << ", ";
        print_hex(cout, opcode);
        cout << "\n";
        for (fragment& c : f) {
            cout << setw(12) << left << ("'" + c.text + "'") << ", ";
            cout << setw(1) << left << c.mode << ", ";
            cout << setw(1) << left << c.negative << ", ";
            cout << setw(4) << left << ("'" + c.prefix + "'") << ", ";
            cout << setw(30) << left << c.value << ", ";
            if (c.mode==2) {
                cout << scientific << setprecision(20) << (*((float*)&c.value)) << ", \t";
            } else
            if (c.mode==3) {
                cout << scientific << setprecision(20) << (*((double*)&c.value)) << ", \t";
            } else
            if (c.mode==4) {
                cout << scientific << setprecision(20) << ((float)(*((half*)&c.value))) << ", \t";
            }
            cout << "\n";
        }
        cout << "\n\n";
    }
} */