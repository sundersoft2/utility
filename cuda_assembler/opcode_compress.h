/*void compress_file() {
    ofstream out("out_opcode_scan_compressed.txt");

    string data_name;
    vector<processed_line> data_reduced;
    vector<processed_line> data_number;
    vector<processed_line> data_normalized;
    vector<pair<processed_line, vector<bool>>> data_all;

    auto vector_to_int=[&](vector<bool>& v) -> uint64 {
        assert(v.size()<=64);
        uint64 res=0;
        for (uint64 x=0;x<v.size();++x) {
            if (v[x]) {
                res |= uint64(1ull << x);
            }
        }
        return res;
    };

    auto process=[&]() {
        if (data_name.empty() && data_reduced.empty() && data_number.empty() && data_normalized.empty() && data_all.empty()) {
            return;
        }

        assert(data_reduced.size()==1);
        vector<pair<int, delta>> deltas=simplify(data_reduced[0], data_normalized, data_all);

        out << "OPCODE\n";
        out << data_name << "\n";
        out << deltas.size() << "\n";
        for (auto& c : deltas) {
            out << c.first << "\n";
            out << c.second.argument_index << "\n";
            out << c.second.added_prefix.text << "\n";
            out << c.second.added_suffix.text << "\n";
        }

        print_opcode(out, data_reduced[0].output(true, false), data_reduced[0].opcode);

        vector<array<int, 65>> numbers=extract_numbers(data_number, data_reduced[0].opcode);

        out << numbers.size() << "\n";
        for (auto& c : numbers) {
            for (int y=0;y<65;++y) {
                out << c[y] << " ";
            }
            out << "\n";
        }

        out << data_all.size() << "\n";
        for (auto& c : data_all) {
            print_hex(out, vector_to_int(c.second));
            out << "\n";
            print_opcode(out, c.first.output(true, false), c.first.opcode);
        }

        data_reduced.clear();
        data_number.clear();
        data_normalized.clear();
        data_all.clear();
        data_name.clear();
    };

    int last_index=-1;
    iterate_out_file( "out_opcode_scan_join.txt", [&](int index, int mode, string& name, uint64 opcode, vector<fragment>& f) {
        processed_line l=process_line(f, opcode);

        if (index!=last_index) {
            process();
            last_index=index;
        }

        if (mode==1) {
            data_reduced.push_back(l);
            data_all.emplace_back(l, vector<bool>());
        } else
        if (mode==2) {
            data_number.push_back(l);
        } else
        if (mode==3) {
            data_normalized.push_back(l);
            data_all.emplace_back(l, vector<bool>());
        } else {
            assert(mode==4);
            data_all.emplace_back(l, vector<bool>());
        }

        assert(!name.empty());
        assert(data_name.empty() || data_name==name);
        data_name=name;

        return true;
    });
    process();
} */