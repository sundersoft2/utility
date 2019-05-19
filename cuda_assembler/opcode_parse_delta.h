struct delta {
    int argument_index=-1;
    fragment added_prefix;
    fragment added_suffix;
};

//c is unmodified if this returns false
bool remove_delta(processed_line& c, delta& d) {
    if (d.argument_index<0 || d.argument_index>=c.arguments.size()) {
        return false;
    }
    argument& a=c.arguments[d.argument_index];

    auto calculate_index=[&](vector<fragment>& current, fragment& added) {
        for (int x=0;x<current.size();++x) {
            if (current[x]==added) {
                return x;
            }
        }
        return -1;
    };

    int i_prefix=-1;
    int i_suffix=-1;

    if (!d.added_prefix.text.empty()) {
        i_prefix=calculate_index(a.prefix, d.added_prefix);
        if (i_prefix==-1) {
            return false;
        }
    }

    if (!d.added_suffix.text.empty()) {
        i_suffix=calculate_index(a.suffix, d.added_suffix);
        if (i_suffix==-1) {
            return false;
        }
    }

    if (i_prefix!=-1) {
        a.prefix.erase(a.prefix.begin()+i_prefix);
    }

    if (i_suffix!=-1) {
        a.suffix.erase(a.suffix.begin()+i_suffix);
    }

    return true;
}

todo //these assume processed_line has an opcode parameter but got rid of it; need to pass it in separately
/*
bool extract_delta(const processed_line& reduced, const processed_line& normalized, delta& res) {
    assert(reduced.arguments.size()==normalized.arguments.size());

    res=delta();
    for (int x=0;x<reduced.arguments.size();++x) {
        const argument& r=reduced.arguments.at(x);
        const argument& n=normalized.arguments.at(x);

        if (r.value!=n.value) {
            return false;
        }

        for (int is_prefix=0;is_prefix<2;++is_prefix) {
            const vector<fragment>& rf=(is_prefix)? r.prefix : r.suffix;
            const vector<fragment>& nf=(is_prefix)? n.prefix : n.suffix;
            fragment& of=(is_prefix)? res.added_prefix : res.added_suffix;

            if (rf.size()!=nf.size() && rf.size()+1!=nf.size()) {
                return false;
            }

            //normalized can either be the same as reduced, or have one extra fragment
            int modified_index=-1;
            for (int y=0;y<nf.size();++y) {
                int nf_index=y;
                int rf_index=(modified_index!=-1)? y-1 : y;

                if (rf_index>=rf.size() || !(rf[rf_index]==nf[nf_index])) {
                    if (modified_index!=-1) {
                        return false;
                    }
                    modified_index=nf_index;
                }
            }

            if ((modified_index==-1) != (rf.size()==nf.size())) {
                return false;
            }

            if (modified_index!=-1) {
                if (res.argument_index!=-1 && res.argument_index!=x) {
                    return false;
                }
                if (!of.text.empty()) {
                    return false;
                }

                res.argument_index=x;
                of=nf[modified_index];
                assert(of.mode==0);
            }
        }
    }

    return true;
}

vector<pair<int, delta>> simplify(
    processed_line& reduced,
    vector<processed_line>& normalized,
    vector<pair<processed_line, vector<bool>>>& all
) {
    vector<pair<int, delta>> res;

    for (const processed_line& c_normalized : normalized) {
        opcode_type delta_bit=get_set_bit(c_normalized.opcode & (~reduced.opcode));

        delta delta_text;
        if (!extract_delta(reduced, c_normalized, delta_text)) {
            continue;
        }

        map<processed_line, tuple<opcode_type, vector<bool>, bool>> without_delta; //key has 0 opcode
        vector<processed_line> with_delta; //delta has been removed from text; opcode unchanged

        for (auto& c : all) {
            processed_line c_copy=c.first;

            bool has_delta=remove_delta(c_copy, delta_text);

            if (has_delta) {
                with_delta.push_back(move(c_copy));
            } else {
                opcode_type opcode=c_copy.opcode;
                c_copy.opcode=0;
                assert(without_delta.emplace(move(c_copy), make_tuple(opcode, c.second, false)).second);
            }
        }

        bool success=true;

        for (processed_line& c : with_delta) {
            opcode_type current_opcode=c.opcode;
            c.opcode=0;

            auto i=without_delta.find(c);
            if (i==without_delta.end()) {
                success=false;
                break;
            }

            //each without-delta opcode must be associated with 0 or 1 with-delta opcodes
            if (get<2>(i->second)) {
                success=false;
                break;
            }
            get<2>(i->second)=true;

            opcode_type expected_opcode=(get<0>(i->second)) | (1ull << delta_bit);

            if (current_opcode!=expected_opcode) {
                success=false;
                break;
            }
        }

        if (!success) {
            continue;
        }

        res.emplace_back(delta_bit, delta_text);

        all.clear();
        for (auto& c : without_delta) {
            all.emplace_back( c.first, move(get<1>(c.second)) );
            all.back().first.opcode=get<0>(c.second);
            all.back().second.push_back(get<2>(c.second));
            assert(all.back().second.size()==res.size());
        }
    }

    return res;
}

vector<array<int, 65>> extract_numbers(vector<processed_line>& p_lines, opcode_type reduced_opcode) {
    vector<array<int, 65>> res;

    for (processed_line& p_line : p_lines) {
        opcode_type delta_bit=get_set_bit(p_line.opcode & (~reduced_opcode));
        bool added=false;

        int next_i_index=0;
        for (int arg_index=0;arg_index<p_line.arguments.size();++arg_index) {
            argument& a=p_line.arguments[arg_index];
            for (int v_index=0;v_index<a.value.size();++v_index) {
                fragment& v=a.value[v_index];

                if (v.mode==5) {
                    continue;
                }

                if (v.mode!=1 && v.mode!=2 && v.mode!=3 && v.mode!=4) {
                    continue;
                }

                while (res.size()<=next_i_index) {
                    res.emplace_back();
                    for (int x=0;x<65;++x) {
                        res.back()[x]=-1;
                    }
                }

                int i=-1;
                if (v.negative) {
                    i=0;
                    //assert(v.value==0);
                } else
                if (v.value!=0) {
                    i=get_set_bit(v.value)+1;
                }

                if (i!=-1) {
                    //can have multiple immediates mapped to the same bit
                    //assert(!added);
                    assert(res.at(next_i_index).at(i)==-1 || res.at(next_i_index).at(i)==delta_bit);
                    res.at(next_i_index).at(i)=delta_bit;

                    added=true;
                }

                ++next_i_index;
            }
        }

        assert(added);
    }

    return res;
} */