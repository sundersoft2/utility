struct fragment {
    string text;

    int mode=0; //0-text, 1-number, 2-float, 3-double, 4-half, 5-name
    bool negative=false; //'-' before prefix if true
    string prefix; //before value. 'R' for registers, 'P' for predicates, '0x' for immediates. not used for floats/doubles
    uint64 value=0; //for floating points, this includes the sign. for integers, it doesn't

    string rel_symbol_name;

    bool operator==(const fragment& f) const {
        return text==f.text;
    }

    bool operator<(const fragment& f) const {
        return text<f.text;
    }

    //named immediates start with a '$' which is removed
    void parse(function<fragment(string)> get_named_fragment) {
        assert(mode==0 && !negative && prefix.empty() && value==0);
        assert(text.size()>0);

        bool t_negative=false;
        int i=0;
        if (text[0]=='-') {
            t_negative=true;
            i=1;
        }

        if (text.substr(i)=="RZ") {
            mode=1;
            negative=t_negative;
            prefix="R";
            value=255;
            return;
        }

        if (text.substr(i)=="PT") {
            mode=1;
            negative=t_negative;
            prefix="P";
            value=7;
            return;
        }

        if (text.size()>=i+1 && text[i]=='$') {
            assert(get_named_fragment);
            *this=get_named_fragment(text.substr(i+1));
            negative=t_negative;
            return;
        }

        if (text.size()>=i+1 && (text[i]=='R' || text[i]=='P')) {
            int end;
            uint64 t_value=parse_int(text, i+1, end, 10);
            if (end==text.size() && end>i+1) {
                mode=1;
                negative=t_negative;
                prefix=text[i];
                value=t_value;
            }
            return;
        }

        if (text.size()>=i+2 && text[i]=='0' && text[i+1]=='x') {
            int end;
            uint64 t_value=parse_int(text, i+2, end, 16);
            if (end==text.size() && end>i+2) {
                mode=1;
                negative=t_negative;
                prefix="0x";
                value=t_value;
            }
            return;
        }

        {
            int end;
            uint64 t_value=parse_int(text, i, end, 10);
            if (end==text.size() && end>i) {
                mode=1;
                negative=t_negative;
                prefix="";
                value=t_value;
            }
            return;
        }
    }

    string formatted(bool reduce_numbers, bool normalize, bool output_rel) const {
        string res;
        if (reduce_numbers && (mode==2 || mode==3 || mode==4)) {
            ostringstream ss;
            print_hex(ss, normalize? 0 : value);
            res="0x" + ss.str(); //sign is encoded in the hex value
        } else
        if (normalize && (mode==1 || mode==2 || mode==3 || mode==4)) {
            res=prefix + "0";
        } else {
            res=text;
        }

        if (output_rel && !rel_symbol_name.empty()) {
            res="$"+ rel_symbol_name;
        }

        return res;
    }

    void assign_int(int64 v, string t_prefix="0x") {
        mode=1;
        negative=(v<0);
        prefix=t_prefix;
        value=(v<0)? -v : v;

        ostringstream ss;
        ss << (negative? "-" : "") << prefix;
        print_hex(ss, value);
        text=ss.str();
    }

    void assign_double(double v) {
        mode=3;
        negative=(v<0);
        prefix="";
        value=*((uint64*)&v);

        ostringstream ss;
        ss.precision(numeric_limits<double>::max_digits10);
        ss << scientific << v;
        text=ss.str();
    }

    void cast_double(int t_mode) {
        assert(mode==3 && (t_mode==2 || t_mode==3 || t_mode==4));
        mode=t_mode;

        double v=*(double*)&value;

        if (t_mode==4) {
            *(half*)&value=v;
        } else
        if (t_mode==2) {
            *(float*)&value=v;
        }
    }
};

//named fragments start with $ and can contain A-Z, a-z, 0-9, _
//doubles should be used for floating point immediates; will automatically get casted to the correct type
vector<fragment> parse_disassembled(
    string line,
    function<fragment(string)> get_named_fragment=function<fragment(string)>()
) {
    vector<fragment> res;

    {
        size_t end_pos=line.find(';');
        if (end_pos==string::npos) {
            //cuda 10 nvdisasm does not output a semicolon for the second line in a block of dual issued lines
            end_pos=line.find('}');
        }
        if (end_pos==string::npos) {
            return res;
        }

        int start_pos=end_pos;
        while (start_pos>=0 && line[start_pos]!='/' && line[start_pos]!='{') {
            --start_pos;
        }

        if (start_pos==-1) {
            start_pos=0;
        } else {
            assert(start_pos>=0);
            ++start_pos;
        }

        while (start_pos<end_pos && isspace(line[start_pos])) {
            ++start_pos;
        }
        assert(start_pos+1<end_pos);

        line=line.substr(start_pos, end_pos-start_pos);
    }

    vector<fragment> pass1;

    {
        fragment buffer;

        auto add_buffer=[&]() {
            if (buffer.text.empty()) {
                return;
            }

            vector<fragment>& c_pass=pass1;

            //there is a ".RZ" flag
            if (c_pass.empty() || c_pass.back().text!=".") {
                buffer.parse(get_named_fragment);
            }

            if (c_pass.empty() || c_pass.back().text!=" " || buffer.text!=" ") {
                c_pass.push_back(buffer);

                if (c_pass.back().text=="reuse") {
                    c_pass.pop_back();
                    assert(!c_pass.empty() && c_pass.back().text==".");
                    c_pass.pop_back();
                } else
                if (
                    c_pass.size()>=3 &&
                    c_pass[c_pass.size()-3].text=="SLOT" &&
                    c_pass[c_pass.size()-2].text==" " &&
                    (c_pass[c_pass.size()-1].text=="0" || c_pass[c_pass.size()-1].text=="1")
                ) {
                    //if two alu instructions are dual-issued then the first one will say "SLOT 0" and the second will say "SLOT 1"
                    c_pass.resize(c_pass.size()-3);
                    while (!c_pass.empty() && c_pass.back().text==" ") {
                        c_pass.pop_back();
                    }
                }
            }
            buffer=fragment();
        };

        for (int x=0;x<line.size();++x) {
            char c=line[x];
            if (x==0) {
                assert(!isspace(c));
            }

            //nvdisasm outputs these when an instruction is dual-issued
            if (c=='{' || c=='}') {
                continue;
            }

            assert(c!=';');

            if (!((c>='0' && c<='9') || (c>='A' && c<='Z') || (c>='a' && c<='z') || c=='-' || c=='_' || c=='?' || c=='$')) {
                add_buffer();
                buffer.text+=c;
                add_buffer();
            } else {
                buffer.text+=c;
            }
        }
        add_buffer();
    }

    string name;
    int name_i=-1;
    for (int i=0;i<pass1.size();++i) {
        if (i==0 && pass1[i].text!="@") {
            name=pass1[i].text;
            name_i=i;
            break;
        } else
        if (i>0 && pass1[i-1].text==" ") {
            name=pass1[i].text;
            name_i=i;
            break;
        }
    }
    assert(!name.empty());
    assert(name_i!=-1);
    assert(name[0]>='A' && name[0]<='Z');
    pass1[name_i].mode=5;

    bool use_doubles=(name[0]=='D');
    bool use_halves=(name[0]=='H');
    int float_mode=2;
    if (use_doubles) {
        float_mode=3;
    } else
    if (use_halves) {
        float_mode=4;
    }

    for (int i=0;i<pass1.size();) {
        //used by named predicates
        if (pass1[i].mode==3) {
            res.push_back(pass1[i]);
            res.back().cast_double(float_mode);

            ++i;
            continue;
        }

        //nvdisasm will omit the "+0x0" when using register-relative addressing
        if (i>=1 && pass1[i-1].mode==1 && pass1[i-1].prefix=="R" && pass1[i].text=="]") {
            res.emplace_back();
            res.back().text="+";

            res.emplace_back();
            res.back().text="0x0";
            res.back().mode=1;
            res.back().negative=false;
            res.back().prefix="0x";
            res.back().value=0;

            res.push_back(pass1[i]);

            ++i;
            continue;
        }

        {
            string c=pass1[i].text;
            bool skip_next=false;
            if (pass1.size()>=i+2 && pass1[i].text=="+") {
                c+=pass1[i+1].text;
                skip_next=true;
            }

            string n=c.substr(1);
            string s=c.substr(0, 1);
            if (
                (n=="SNAN" || n=="QNAN" || n=="INF") &&
                (s=="+" || s=="-")
            ) {
                double v;
                if (n=="SNAN") {
                    v=numeric_limits<double>::signaling_NaN();
                } else
                if (n=="QNAN") {
                    v=numeric_limits<double>::quiet_NaN();
                } else {
                    assert(n=="INF");
                    v=numeric_limits<double>::infinity();
                }

                if (s=="-") {
                    v=-v;
                }

                //note: for nans, some of the bits are unknown
                res.emplace_back();
                res.back().text=c;
                res.back().mode=float_mode;
                res.back().negative=(s=="-");
                res.back().value=0;

                if (use_doubles) {
                    *(double*)&res.back().value=v;
                } else
                if (use_halves) {
                    *(half*)&res.back().value=v;
                } else {
                    *(float*)&res.back().value=v;
                }

                i+=2;
                continue;
            }
        }

        if (
            pass1.size()>=i+3 &&
            pass1[i+0].mode==1 && pass1[i+0].prefix==""  &&
            pass1[i+1].mode==0 && pass1[i+1].text  =="."
        ) {
            string str=pass1[i+0].text + pass1[i+1].text + pass1[i+2].text;
            assert(!str.empty());
            i+=3;

            if (str.back()=='e') {
                assert(pass1.size()>=i+2);
                assert(pass1[i].text=="+");
                assert(pass1[i+1].mode==1 && pass1[i+1].prefix=="");

                str+=pass1[i+0].text + pass1[i+1].text;
                i+=2;
            }

            istringstream ss(str);

            double v;
            ss >> v;
            assert(!ss.fail());

            if (!ss.fail() && ss.eof()) {
                res.emplace_back();
                res.back().text=str;
                res.back().mode=float_mode;
                res.back().negative=(v<0);
                res.back().value=0;

                if (use_doubles) {
                    *(double*)&res.back().value=v;
                } else
                if (use_halves) {
                    *(half*)&res.back().value=v;
                } else {
                    *(float*)&res.back().value=v;
                }

                continue;
            }
        }

        if (
            pass1[i].mode==1 &&
            (pass1[i].prefix=="R" || pass1[i].prefix=="P") &&
            pass1[i].negative
        ) {
            assert(!pass1[i].text.empty() && pass1[i].text[0]=='-');

            res.emplace_back();
            res.back().text="-";

            res.push_back(pass1[i]);
            res.back().text=res.back().text.substr(1);
            res.back().negative=false;

            ++i;
            continue;
        }

        res.push_back(pass1[i]);
        if (res.back().mode==1 && res.back().prefix=="") {
            if (i>0 && pass1[i-1].text==".") {
                res.back().mode=0;
                res.back().negative=false;
                res.back().prefix="";
                res.back().value=0;
            } else {
                uint64 v=res.back().value;

                res.back().mode=float_mode;
                res.back().value=0;

                if (use_doubles) {
                    double r=v;
                    if (res.back().negative) {
                        r=-r;
                    }
                    *(double*)&res.back().value=r;
                } else
                if (use_halves) {
                    half r;
                    r=float(v);
                    if (res.back().negative) {
                        r=-r;
                    }
                    *(half*)&res.back().value=r;
                } else {
                    float r=v;
                    if (res.back().negative) {
                        r=-r;
                    }
                    *(float*)&res.back().value=r;
                }
            }
        }
        ++i;
    }

    return res;
}

todo //use a different file format
/*void print_opcode(ostream& out, const vector<fragment>& data, uint64 opcode) {
    for (const fragment& f : data) {
        out << f.text;
    }

    out << "; /* 0x";
    print_hex(out, opcode);
    out << " *" << "/\n";
}

void print_opcode(ostream& out, string data, uint64 opcode) {
    vector<fragment> dummy={fragment()};
    dummy[0].text=move(data);
    print_opcode(out, dummy, opcode);
} */

bool get_is_normalized(vector<fragment>& f) {
    for (fragment& c : f) {
        if (c.mode!=0 && c.mode!=5 && c.value!=0) {
            return false;
        }
    }
    return true;
}