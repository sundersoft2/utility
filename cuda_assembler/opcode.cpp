#include "include.h"
#define todo

const bool debug=
0
;

const string compressed_file_path="/home/ilya/projects/em_simulator_data/out_opcode_scan_compressed.txt";

//todo: opcode_scan.h and opcode_compress.h don't work anymore; need to make them use processed_line
//for random scan, just output everything
//don't add any special handling for the predication operator
//when doing normalization, ignore numbers initially (mark the bit as belonging to a number and leave it alone)
//when calculating numbers, need to find optional numbers as well
//-to detect optional numbers, iterate all power of 2 values for the number to see which bits belong to the number. then set all of
// those bits to 0 or 1 to see if the number disappears
//-it is now an error for reduction to fail because of the reduced_is_normalized assert. should also be able to detect all default
// values so don't need any special handling for register+offset in constant addresses
//-also get rid of add_optional_predicate, add_optional_end
//don't need any special handling for "invalid", "?", etc
//hybrids:
//-current algorithm won't work well on volta
//-better algorithm:
//--generate a set of unqiue random hybrid opcodes
//--iterate all of the normalized bits and set those bits to true and false for that set of opcodes; determine if a valid delta
//  can be generated. if so then that bit is removed from consideration
//---the delta has to work for all of the opcodes
//--if changing the bit did nothing then it is also removed from consideration (for this to work, need like 100k random values
//  or something)
//--whatever bits remain will be fully iterated using the current algorithm. error if there are too many bits
//procedure for generating hybrids:
//-right now nvdisasm can crash at this stage
//-will start by randomizing one of the normalized bits. if this causes nvdisasm to crash then that bit is marked as broken and the
// next bit is tried
//-will keep going until nvdisasm does not crash and all of the broken bits are known
//-will just set the broken bits to 0 and treat them as part of the reduced opcode
//-nvdisasm shouldn't crash anymore after the broken bits are found
//processing found opcodes:
//-each file name has a reduced opcode along with a bitmask indicating which bits are used by that opcode
// (and are 0 in the reduced opcode)
//-each opcode in a found file is checked against the existing filenames to see if it should be processed or not. this is faster
// than the current method
//
//note: 9.2 cuda nvdisasm works with invalid instructions but crashes with --print-instruction-encoding. will use cuda 9.2 but without
// that option and just infer the encoding
//-9.2 cuda will still refuse to work if there are errors but it prints the addresses of all of the error instructions
//-this has no real effect on hybrid handling
//
//probably want to detect which argument the ".reuse" flag applies to

#include "utility.h"

#include "opcode_parse_line.h"
#include "opcode_parse_control_flags.h"
#include "opcode_parse_fragment.h"
#include "opcode_parse_processed_line.h"
#include "opcode_parse_processed_line_parse.h"
#include "opcode_parse_argument_state.h"
#include "opcode_parse_delta.h"
#include "opcode_parse_encode.h"

#include "optimizer.h"
#include "optimizer_ordering.h"
#include "optimizer_reuse.h"
#include "optimizer_stall.h"

#include "cubin_elf.h"
#include "cubin_info.h"
#include "cubin_kernel.h"
#include "cubin_file.h"
#include "cubin_parse.h"

#include "opcode_scan.h"
#include "opcode_compress.h"
#include "opcode_encode.h"
#include "opcode_test.h"

int main(int argc, char** argv) {
    assert(argc>=2);
    /*if (string(argv[1])=="single") {
        //opcode is passed in as hex. only opcodes with the same argument types are outputted
        //note: if input opcode uses "@PT" as its predicate, only opcodes with "@PT" as their predicates will be outputted

        assert(argc>=3);
        uint64 id=0;
        string expected_name="";

        istringstream in(argv[2]);
        in >> hex >> id >> dec;
        assert(!in.fail());

        if (argc>=4) {
            expected_name=argv[3];
        }

        cerr << "Single: ";
        print_hex(cerr, id);
        cerr << expected_name << "\n";
        process_file(id, expected_name);
        disassemble({}); //clear temp file
    } else
    if (string(argv[1])=="rand") {
        assert(argc>=3);
        unsigned int id;

        istringstream in(argv[2]);
        in >> id;
        assert(!in.fail());

        cerr << "Rand: " << id << "\n";
        random_opcodes_file(id);
        disassemble({}); //clear temp file
    } else
    if (string(argv[1])=="index") {
        assert(argc>=3);
        vector<string> files;
        for (int i=2;i<argc;++i) {
            files.push_back(argv[i]);
        }

        cerr << "Index\n";
        index_opcodes_file(argv[0], files);
        disassemble({}); //clear temp file
    } else
    if (string(argv[1])=="join") {
        assert(argc>=3);
        vector<string> files;
        for (int i=2;i<argc;++i) {
            files.push_back(argv[i]);
        }

        cerr << "Join\n";
        ofstream out("out_opcode_scan_join.txt");
        for (string c : files) {
            cerr << "Joining " << c << "\n";
            join_opcodes_file(out, c);
        }
        disassemble({}); //clear temp file
    } else
    if (string(argv[1])=="compress") {
        compress_file();
    } else*/
    if (string(argv[1])=="encode") {
        assert(argc>=3);
        encode_file(argv[2]);
    } else
    if (string(argv[1])=="parse") {
        assert(argc>=3);
        generate_cubin_file(argv[2], "generate_cubin_file_stats.log");
    } else {
        assert(false);
    }
}