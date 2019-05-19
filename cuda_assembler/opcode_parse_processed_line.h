struct argument {
    vector<fragment> prefix;
    vector<fragment> value;
    vector<fragment> suffix;

    bool operator==(const argument& t) const {
        return prefix==t.prefix && value==t.value && suffix==t.suffix;
    }

    bool same_normalized(const argument& t) const {
        if (prefix==t.prefix && value.size()==t.value.size() && suffix==t.suffix) {
            for (int x=0;x<value.size();++x) {
                const fragment& v1=value[x];
                const fragment& v2=t.value[x];

                if (v1.mode!=v2.mode || v1.prefix!=v2.prefix) {
                    return false;
                }
            }
        } else {
            return false;
        }
        return true;
    }

    bool operator<(const argument& t) const {
        if (prefix!=t.prefix) {
            return prefix<t.prefix;
        } else
        if (value!=t.value) {
            return value<t.value;
        } else {
            return suffix<t.suffix;
        }
    }
};

struct processed_line {
    vector<argument> arguments;
    vector<string> padding; //between arguments

    todo //if these are needed then should use a pair<processed_line, opcode> in sets/maps
    /*bool operator==(const processed_line& t) const {
        return opcode==t.opcode && arguments==t.arguments;
    }

    bool operator<(const processed_line& t) const {
        if (opcode!=t.opcode) {
            return opcode<t.opcode;
        } else {
            return arguments<t.arguments;
        }
    }*/

    bool is_label() const {
        return arguments.size()==1 && arguments[0].prefix.size()==1 && arguments[0].value.empty() && arguments[0].suffix.empty();
    }

    string get_label() const {
        return arguments[0].prefix[0].text;
    }

    const argument& get_name_argument(int& name_i) const {
        name_i=-1;
        const argument* res=nullptr;
        for (int x=0;x<arguments.size();++x) {
            const argument& a=arguments[x];
            if (a.value.size()==1 && a.value[0].mode==5) {
                assert(res==nullptr);
                res=&a;
                name_i=x;
            }
        }

        assert(res!=nullptr);
        return *res;
    }

    string get_name() const {
        int dummy;
        return get_name_argument(dummy).value[0].text;
    }

    //does not include opcode
    //reduce numbers will convert floats to ints
    string output(bool reduce_numbers, bool normalize, bool output_rel) const {
        string res;

        for (int x=0;x<arguments.size();++x) {
            const argument& c=arguments[x];
            for (const fragment& f : c.prefix) {
                res+=f.formatted(reduce_numbers, normalize, output_rel);
            }
            for (const fragment& f : c.value) {
                res+=f.formatted(reduce_numbers, normalize, output_rel);
            }
            for (const fragment& f : c.suffix) {
                res+=f.formatted(reduce_numbers, normalize, output_rel);
            }

            if (x<padding.size()) {
                res+=padding[x];
            }
        }

        return res;
    }

    vector<pair<uint64, bool>> extract_numbers() {
        vector<pair<uint64, bool>> res;
        for (argument& a : arguments) {
            for (fragment& v : a.value) {
                if (v.mode==1 || v.mode==2 || v.mode==3 || v.mode==4) {
                    res.emplace_back(v.value, v.negative);
                }
            }
        }
        return res;
    }

    //special absolute address values are used for labels
    todo //for register+offset BRA, make the default offset go to the start of the text section
    void absolute_to_relative(int index, function<uint64(uint64)> translator=function<uint64(uint64)>()) {
        string name=get_name();
        if (!(name=="BRA" || name=="BRX" || name=="SSY" || name=="CAL" || name=="PBK" || name=="PCNT")) {
            return;
        }

        bool found_name=false;
        argument* t_a=nullptr;
        for (argument& a : arguments) {
            if (a.value.size()==1 && a.value[0].mode==5) {
                found_name=true;
                continue;
            }

            if (!found_name) {
                continue;
            }

            //BRA CC.GE, 0x0
            if (a.value.size()==0) {
                continue;
            }

            //BRX R0, 0x0
            if (a.value.size()==1 && a.value[0].mode==1 && a.value[0].prefix=="R" && t_a==nullptr) {
                continue;
            }

            if (a.value.size()!=1 || a.value[0].mode!=1 || a.value[0].prefix!="0x" || t_a!=nullptr) {
                return;
            }
            t_a=&a;
        }

        //int address=((index%3)+1+(index/3)*4+1)*8;
        int address=index_to_absolute_maxwell(index+1, false); //after instruction (which can be a control code)

        int64 v;
        if (translator) {
            v=(int64)translator(t_a->value[0].value);
        } else {
            v=(int64)t_a->value[0].value;
        }

        assert(v>=0 && !t_a->value[0].negative);

        t_a->value[0].assign_int(v-address);
    }

    //
    //

    void add_optional_predicate(const processed_line& normalized) {
        assert(!arguments.empty() && !normalized.arguments.empty());

        //optional "@PT"
        if (
            !normalized.arguments[0].prefix.empty() && normalized.arguments[0].prefix[0].text=="@" &&
            (arguments[0].prefix.empty() || arguments[0].prefix[0].text!="@")
        ) {
            arguments.insert(arguments.begin(), argument());
            padding.insert(padding.begin(), " ");

            arguments[0].prefix.emplace_back();
            arguments[0].prefix.back().text="@";

            arguments[0].value.emplace_back();
            arguments[0].value.back().text="PT";
            arguments[0].value.back().mode=1;
            arguments[0].value.back().prefix="P";
            arguments[0].value.back().value=7;
        }
    }

    //need to remove deltas before doing this
    void add_optional_end(const processed_line& normalized) {
        assert(!arguments.empty() && !normalized.arguments.empty());

        //optional last argument
        if (
            arguments.size()==normalized.arguments.size()-1 &&
            normalized.arguments.size()>=2
        ) {
            bool same=true;
            for (int x=0;x<arguments.size();++x) {
                if (!arguments[x].same_normalized(normalized.arguments[x])) {
                    same=false;
                    break;
                }
            }

            string name=get_name();
            if (same && (name=="MOV" || name=="MOV32I")) {
                assert(padding.back().empty());
                padding.back()=", ";
                padding.push_back("");

                arguments.emplace_back();
                arguments.back().value.emplace_back();
                arguments.back().value.back().text="0xf";
                arguments.back().value.back().mode=1;
                arguments.back().value.back().prefix="0x";
                arguments.back().value.back().value=15;
            }
        }
    }
};