#ifndef ILYA_HEADER_IO_BASIC
#define ILYA_HEADER_IO_BASIC

#include <fstream>
#include <vector>
#include <map>
#include <utility>

#include "math_linear_vector.h"
#include "io_base.h"

namespace io {
using std::ostream;
using std::istream;
using std::vector;
using std::string;
using std::getline;
using math::vec_base;
using math::vec_traits;

bool parse_option(const string& str, const string& str_true, const string& str_false) {
    if (str==str_true) {
        return true;
    } else
    if (str==str_false) {
        return false;
    } else {
        assert(false);
    }
}

struct binary_type {} binary;
struct text_type {} text;

template<class type> type read(binary_type, istream& in) {
    type b;
    if (in.read((char*)&b, sizeof(type))) {
        return b;
    } else {
        assert(false);
    }
}

template<> string read<string>(binary_type, istream& in) {
    int size=read<int>(binary, in);
    string res;
    res.resize(size);
    in.read(&(res[0]), size);
    assert(in);
    return res;
}

template<class type> void write(binary_type, ostream& out, const type& t) {
    out.write(reinterpret_cast<const char*>(&t), sizeof(t));
    assert(out);
}

template<> void write<string>(binary_type, ostream& out, const string& t) {
    write(binary, out, int(t.size()));
    out.write(&(t[0]), t.size());
    assert(out);
}

template<class type> type read(text_type, istream& in) {
    type res;
    in >> res;
    assert(in);
    return res;
}

template<class type> void write(text_type, ostream& out, type t) {
    out << t << "\n";
    assert(out);
}

template<int size, class type=float, class mode_type=binary_type> vec_base<type, size> read_vec(mode_type mode, istream& in) {
    vec_base<type, size> res;
    for (int x=0;x<size;++x) {
        res.data[x]=read<type>(mode, in);
    }
    return res;
}

template<int size, class type, class mode_type=binary_type> void write_vec(mode_type mode, ostream& out, vec_base<type, size> v) {
    for (int x=0;x<size;++x) {
        write(mode, out, v[x]);
    }
}


}

#endif
