void encode_file(string in_file) {
    //stats
    set<string> dual_issue_0;
    set<string> dual_issue_1;
    set<string> write_barrier;
    set<string> read_barrier;
    bool stall_was_0=false;
    //

    for (auto& c : parse_nvdisasm_maxwell(getfile(in_file, true))) {
        auto& c_line=c.instruction;
        auto& c_control_code=c.c_flags;
        int c_index=c.index;
        opcode_type expected_opcode=c.opcode;

        processed_line p_line=process_line(c_line);
        string p_line_string=p_line.output(false, false, false);
        p_line.absolute_to_relative(c_index);
        opcode_type res=get_opcode_encode().encode(p_line, true, c_line, c_index).opcode;

        if (res!=expected_opcode) {
            print("ERROR: Opcode mismatch", c_line, c_index, expected_opcode, res);
        }

        cout << c_control_code.output() << "    " << p_line_string << ";\n";

        //stats
        {
            string c_name=p_line.get_name();

            bool stall_is_0=(c_control_code.stall_count==0 && c_name!="NOP");
            if (stall_was_0 || stall_is_0) {
                (stall_was_0? dual_issue_1 : dual_issue_0).insert(c_name);
            }

            if (c_control_code.read_barrier!=-1) {
                read_barrier.insert(c_name);
            }

            if (c_control_code.write_barrier!=-1) {
                write_barrier.insert(c_name);
            }

            stall_was_0=stall_is_0;
        }
    }

    //stats
    {
        print("==== dual_issue_0 ====");
        for (auto& s : dual_issue_0) {
            print(s);
        }

        print("==== dual_issue_1 ====");
        for (auto& s : dual_issue_1) {
            print(s);
        }

        print("==== write_barrier ====");
        for (auto& s : write_barrier) {
            print(s);
        }

        print("==== read_barrier ====");
        for (auto& s : read_barrier) {
            print(s);
        }
    }
}