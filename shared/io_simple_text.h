#ifndef ILYA_HEADER_IO_SIMPLE_TEXT
#define ILYA_HEADER_IO_SIMPLE_TEXT

#include <fstream>
#include <vector>
#include <map>
#include <utility>

#include "math_linear_vector.h"
#include "math_linear_transform.h"

#include "io_base.h"

namespace io { namespace simple_text {
    using std::ostream;
    using std::istream;
    using std::vector;
    using std::string;
    using std::getline;
    using math::matrix_across_base;
    using math::vec_base;
    using math::matrix_traits;
    using math::vec_traits;
    
    template<int width, int height> matrix_across_base<double, width, height> read_matrix(istream& in) {
        matrix_across_base<double, width, height> res;
        for (int y=0;y<height;++y) {
            for (int x=0;x<width;++x) {
                in >> res(y, x);
            }
        }
        return res;
    };
    template<class matrix> typename matrix_traits<matrix>::template only_if<void>::good write_matrix(ostream& out, const matrix& targ) {
        for (int y=0;y<matrix::height;++y) {
            for (int x=0;x<matrix::width;++x) {
                if (x!=0) out << " ";
                out << targ(y, x);
            }
            out << "\n";
        }
    };
    
    template<int num> vec_base<double, num> read_vec(istream& in) {
        vec_base<double, num> res;
        for (int x=0;x<num;++x) {
            in >> res[x];
        }
        return res;
    };
    template<class vec> typename vec_traits<vec>::template only_if<void>::good write_vec(ostream& out, const vec& targ) {
        for (int x=0;x<vec::num;++x) {
            if (x!=0) out << " ";
            out << targ[x];
        }
        out << "\n";
    };
    
    template<class type> type read(istream& in) {
        type res;
        in >> res;
        return res;
    };
    template<class type> void write(ostream& out, const type& targ) {
        out << targ << "\n";
    };
    
    string read_string(istream& in) {
        while (isspace(in.peek())) in.ignore();
        string res;
        if (in.peek()=='"') {
            in.ignore();
            getline(in, res, '"');
        } else {
            in >> res;
        }
        return res;
    };
    void write_string(ostream& out, const string& targ) {
        out << "\"" << targ << "\"\n";
    };
}}

#endif
