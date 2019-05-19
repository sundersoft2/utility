processed_line process_line(vector<fragment>& line) {
    processed_line res;

    vector<fragment> buffer;
    bool found_name=false;
    auto flush_buffer=[&](string t_padding) {
        if (buffer.empty()) {
            if (!res.padding.empty()) {
                res.padding.back()+=t_padding;
            }
            return;
        }

        int mode=0;
        bool is_bracket=false;
        argument arg;

        for (fragment& c : buffer) {
            int next_mode=-1;
            if (mode==0) {
                if (c.mode!=0 || c.text=="[") {
                    if (c.text=="[") {
                        is_bracket=true;
                    }
                    mode=1;
                }
            } else
            if (mode==1) {
                if (is_bracket) {
                    if (c.text=="]") {
                        next_mode=2;
                    }
                } else {
                    assert(c.mode==0);
                    mode=2;
                }
            } else {
                assert(mode==2);
                assert(c.mode==0);
                assert(c.text!="[" && c.text!="]");
            }

            if (mode==0) {
                assert(c.mode==0);
                arg.prefix.push_back(c);
            } else
            if (mode==1) {
                arg.value.push_back(c);

                fragment& f=arg.value.back();

                if (f.mode==5) {
                    assert(!found_name);
                    found_name=true;
                }
            } else {
                assert(mode==2);
                assert(c.mode==0);
                arg.suffix.push_back(c);
            }

            if (next_mode!=-1) {
                mode=next_mode;
            }
        }

        res.arguments.push_back(arg);
        res.padding.push_back(t_padding);
        buffer.clear();
    };

    for (int x=0;x<line.size();++x) {
        fragment& f=line[x];

        string prev_text;
        string this_text=f.text;
        string next_text;

        if (x!=0) {
            prev_text=line[x-1].text;
        }

        if (x<line.size()-1) {
            next_text=line[x+1].text;
        }

        if (this_text=="," || this_text==" ") {
            //nvdisasm will sometimes output "c[0x0] [0x0]" which is one argument
            if (prev_text=="]" && this_text==" " && next_text=="[") {
                assert(buffer.back().text=="]");
                buffer.pop_back();
                line[x+1].text="] [";
            } else {
                flush_buffer(this_text);
            }
        } else {
            if (this_text=="." && !next_text.empty() && line[x+1].mode==0) {
                line[x+1].text="." + line[x+1].text;
            } else
            if (this_text=="]" && next_text=="[") {
                line[x+1].text="][";
            } else {
                if (this_text=="[" && !found_name) {
                    flush_buffer(""); //some instructions don't have a space between the name and the '['
                    assert(found_name);
                }
                buffer.push_back(f);
            }
        }
    }
    flush_buffer("");

    for (string& s : res.padding) {
        if (s.find(',')!=string::npos) {
            s=", ";
        } else
        if (s.find(' ')!=string::npos) {
            s=" ";
        } else {
            s="";
        }
    }

    assert(!res.padding.empty());
    assert(res.padding.back()!=", ");
    res.padding.back()="";

    return res;
}

processed_line process_line(
    string line,
    function<fragment(string)> get_named_fragment=function<fragment(string)>()
) {
    vector<fragment> fragments=parse_disassembled(line, get_named_fragment);
    return process_line(fragments);
}