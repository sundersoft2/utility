template<class type_a, class... types> void debug_print(const type_a& a, const types&... targs) {
    if (debug) {
        print_to(cerr, a, targs...);
    }
}

template<class type> void print_hex(ostream& out, type v) {
    out << hex << v << dec;
}

int num_set_bits(uint64 c) {
    int res=0;
    for (uint64 x=0;x<64;++x) {
        if (c|(1ull << x)) {
            ++res;
        }
    }
    return res;
}

uint64 parse_int(string text, int start, int& end, int base) {
    uint64 t_value=0;
    for (int i=start;i<text.size();++i) {
        char c=text[i];

        int d=-1;
        if (c>='0' && c<='9') {
            d=c-'0';
        } else
        if (c>='a' && c<='f') {
            d=(c-'a')+10;
        }

        if (!(d>=0 && d<base)) {
            end=i;
            return t_value;
        }

        t_value*=base;
        t_value+=d;
    }

    end=text.size();
    return t_value;
}

int get_set_bit(uint64 t) {
    int res=-1;
    for (uint64 x=0;x<64;++x) {
        if (t & (1ull << x)) {
            assert(res==-1);
            res=x;
        }
    }

    assert(res!=-1);
    return res;
}

template<class d_type> void append_string(string& s, const d_type& d) {
    for (int x=0;x<sizeof(d);++x) {
        s.push_back(((char*)&d)[x]);
    }
}

void overwrite_string(string& s, int pos, const string& d) {
    for (int x=0;x<d.size();++x) {
        s.at(pos)=d[x];
        ++pos;
    }
}