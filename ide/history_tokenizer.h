namespace history { namespace tokenizer {


//if a token spans multiple characters, the first character has a positive token type and
//the other characters have the negation of that type
const s8 token_type_invalid=0;
const s8 token_type_string_single=1;
const s8 token_type_string_double=2;
const s8 token_type_comment_c=3; // /*
const s8 token_type_comment_cpp=4; // //
const s8 token_type_number=5; //float, hex, octal, or decimal
const s8 token_type_preprocessor=6;
const s8 token_type_identifier=7;
const s8 token_type_keyword=8;
const s8 token_type_space=9;
const s8 token_type_operator=10;

//not outputted by tokenizer:
const s8 token_type_group_header_default=11;
const s8 token_type_group_header_current=12;
const s8 token_type_group_indentation_default=13;
const s8 token_type_group_indentation_current=14;

const s8 token_type_compile_log_path=15;
const s8 token_type_compile_log_line=16;
const s8 token_type_compile_log_column=17;
const s8 token_type_compile_log_other=18;

struct line_state {
    s8 token; //initially invalid. after first line, can be space, string, or c comment.
    int net_brackets; //considers '{', '(', and '['. does not consider angle brackets. excludes brackets in comments and strings
    int num_closing_brackets_before_first_opening_bracket;
    int num_spaces_at_start;
    
    line_state() : token(token_type_invalid), net_brackets(0), num_closing_brackets_before_first_opening_bracket(0), num_spaces_at_start(0) {}
    explicit line_state(s8 t_token) : token(t_token), net_brackets(0), num_closing_brackets_before_first_opening_bracket(0), num_spaces_at_start(0) {}
    
    bool operator==(const line_state& s) {
        return token==s.token;
    }
    
    bool operator!=(const line_state& s) { return !(*this==s); }
};

const set<string> multi_char_operators={
    "++", "--", "==", "!=", ">=", "<=", "&&", "||", "<<", ">>", "+=", "-=",
    "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "->", "->*", ".*", "::"
};
const int max_multi_char_operator_size=3;

const set<string> keywords={
    "alignas", "alignof", "and", "and_eq", "asm", "auto(1)", "bitand",
    "bitor", "bool", "break", "case", "catch", "char", "char16_t",
    "char32_t", "class", "compl", "concept", "const", "constexpr",
    "const_cast", "continue", "decltype", "default", "delete", "do",
    "double", "dynamic_cast", "else", "enum", "explicit", "export",
    "extern", "false", "float", "for", "friend", "goto", "if", "inline",
    "int", "long", "mutable", "namespace", "new", "noexcept", "not",
    "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected",
    "public", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast",
    "struct", "switch", "template", "this", "thread_local", "throw", "true",
    "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
    "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
};

bool is_opening_bracket(char c) {
    return c=='(' || c=='{' || c=='[';
}

bool is_closing_bracket(char c) {
    return c==')' || c=='}' || c==']';
}

struct generic_tokenizer {
    vector<s8> res;
    string buffer;
    bool processed;
    int pos;
    s8 current_token;
    const string& current_line;
    
    char c;
    char next_c;
    
    bool is_letter;
    bool is_decimal;
    bool is_space;
    bool is_symbol;
    
    bool next_is_decimal;
    bool next_is_hex;
    
    generic_tokenizer(s8 first_token, const string& t_current_line, bool process_first_character=false) :
        pos(1), processed(false), current_token(first_token), current_line(t_current_line),
        c(0), next_c(0), is_letter(false), is_decimal(false), is_space(false), is_symbol(false), next_is_decimal(false), next_is_hex(false)
    {
        assert(first_token>=0);
        res.push_back(first_token);
        
        if (process_first_character) {
            pos=0;
        } else {
            assert(current_line[0]=='\n');
        }
    }
    
    void flush_buffer(bool disable_assert) {
        for (int x=0;x<buffer.size();++x) {
            bool negate=(x!=0);
            
            if (x==0 && res.size()==1 && res.back()==current_token) {
                negate=true;
            }
            
            res.push_back((negate)? -current_token : current_token);
        }
        
        current_token=token_type_invalid;
        buffer.clear();
        
        if (!disable_assert) {
            assert(res.size()==(processed? pos+1 : pos));
        }
    }
    
    void add_buffer() {
        assert(!processed);
        buffer+=c;
        processed=true;
    };
    
    virtual void loop_inner()=0;
    
    virtual void loop_finish()=0;
    
    virtual void process_invalid_token_impl()=0;
    
    void process_invalid_token() {
        assert(buffer.empty());
        
        if (processed) {
            assert(c!='\n');
        } else {
            add_buffer();
        }
        
        process_invalid_token_impl();
    };
    
    s8 process() {
        for (;pos<=current_line.size();++pos) {
            c=(pos==current_line.size())? '\n' : current_line[pos];
            next_c=(pos>=current_line.size()-1)? '\n' : current_line[pos+1];
            processed=false;
            
            if (pos<current_line.size()) {
                assert(c!='\n');
            }
            
            is_letter=(tolower(c)>='a' && tolower(c)<='z') || c=='_';
            is_decimal=(c>='0' && c<='9');
            is_space=(c==' ' || c=='\n');
            is_symbol=!is_letter && !is_decimal && !is_space;
            
            next_is_decimal=(next_c>='0' && next_c<='9');
            next_is_hex=(tolower(next_c)>='a' && tolower(next_c)<='f') || next_is_decimal;
            
            if (current_token==token_type_invalid) {
                process_invalid_token();
            } else {
                loop_inner();
            }
            
            if (!processed) {
                assert(current_token==token_type_invalid);
                process_invalid_token();
            }
            
            assert(processed);
        }
        
        loop_finish();
        
        processed=false;
        flush_buffer(false);
        
        assert(res.size()==current_line.size()+1);
        
        auto last_token=res.back();
        
        res.pop_back();
        return last_token;
    }
};

struct c_language_tokenizer : public generic_tokenizer {
    //per line
    int net_brackets;
    int num_closing_brackets_before_first_opening_bracket;
    int num_spaces_at_start;
    bool found_opening_bracket;
    
    //per token
    bool string_backslash;
    bool number_found_period;
    bool number_found_x;
    bool number_found_e;
    bool number_found_suffix;
    
    void flush_buffer(bool disable_assert) {
        generic_tokenizer::flush_buffer(disable_assert);
        
        string_backslash=false;
        number_found_period=false;
        number_found_x=false;
        number_found_e=false;
        number_found_suffix=false;
    };
    
    c_language_tokenizer(s8 first_token, const string& t_current_line) : generic_tokenizer(first_token, t_current_line),
        net_brackets(0), num_closing_brackets_before_first_opening_bracket(0), num_spaces_at_start(0), found_opening_bracket(false),
        string_backslash(false), number_found_period(false), number_found_x(false), number_found_e(false), number_found_suffix(false)
    {}
    
    void process_invalid_token_impl() override {
        if (c=='\'') {
            current_token=token_type_string_single;
        } else
        if (c=='"') {
            current_token=token_type_string_double;
        } else
        if (c=='/' && next_c=='*') {
            current_token=token_type_comment_c;
        } else
        if (c=='/' && next_c=='/') {
            current_token=token_type_comment_cpp;
        } else
        if (is_decimal || (c=='-' && next_is_decimal) || (c=='.' && next_is_decimal)) {
            current_token=token_type_number;
        } else
        if (c=='#') {
            current_token=token_type_preprocessor;
        } else
        if (is_letter) {
            current_token=token_type_identifier;
        } else
        if (is_space) {
            current_token=token_type_space;
        } else {
            assert(is_symbol);
            current_token=token_type_operator;
        }
    }
    
    void loop_inner() override {
        if (current_token==token_type_string_single || current_token==token_type_string_double) {
            if (c=='\\') {
                string_backslash=!string_backslash;
                add_buffer();
            } else
            if (
                !string_backslash && (
                    (current_token==token_type_string_single && c=='\'') ||
                    (current_token==token_type_string_double && c=='"') ||
                    c=='\n'
                )
            ) {
                if (c!='\n') {
                    add_buffer();
                }
                flush_buffer(false);
            } else {
                string_backslash=false;
                add_buffer();
            }
        } else
        if (current_token==token_type_comment_cpp || current_token==token_type_preprocessor) {
            if (c=='\n' && (current_token!=token_type_preprocessor || buffer.empty() || buffer[buffer.size()-1]!='\\')) {
                flush_buffer(false);
            } else {
                add_buffer();
            }
        } else
        if (current_token==token_type_comment_c) {
            add_buffer();
            
            if (((res.size()==1 && res.front()==-token_type_comment_c && buffer.size()>=2) || buffer.size()>=4) && buffer.substr(buffer.size()-2)=="*/") {
                flush_buffer(false);
            }
        } else
        if (current_token==token_type_number) {
            bool is_x=(tolower(c)=='x');
            bool is_e=(tolower(c)=='e');
            bool is_minus_sign=(c=='-');
            bool is_hex=(tolower(c)>='a' && tolower(c)<='f') || is_decimal;
            bool is_zero=(c=='0');
            bool is_period=(c=='.');
            bool is_u=(tolower(c)=='u');
            bool is_l=(tolower(c)=='l');
            bool is_f=(tolower(c)=='f');
            
            bool expect_hex=number_found_x;
            bool expect_decimal=!expect_hex;
            bool is_digit_after_leading_zero=(buffer=="0" || buffer=="-0");
            bool previous_was_e=(!buffer.empty() && tolower(buffer[buffer.size()-1])=='e');
            
            if (!number_found_suffix) {
                if (is_hex) {
                    if (!expect_decimal || is_decimal) {
                        add_buffer();
                    }
                } else
                if (is_minus_sign) {
                    if ((buffer.empty() || previous_was_e) && (next_is_decimal || (expect_hex && next_is_hex))) {
                        add_buffer();
                    }
                } else
                if (is_e) {
                    if (!number_found_x && !number_found_e && next_is_decimal) {
                        add_buffer();
                        number_found_e=true;
                    }
                } else
                if (is_x) {
                    if (is_digit_after_leading_zero && next_is_hex) {
                        add_buffer();
                        number_found_x=true;
                    }
                } else
                if (is_period) {
                    if (!number_found_period && !number_found_x && !number_found_e && next_is_decimal) {
                        add_buffer();
                        number_found_period=true;
                    }
                }
            }
            
            if (is_u || is_l || is_f) {
                string suffix;
                for (int x=buffer.size()-1;x>=0;--x) {
                    char c_x=tolower(buffer[x]);
                    if (c_x!='u' && c_x!='l' && c_x!='f') {
                        break;
                    }
                    
                    suffix+=c_x;
                }
                reverse(suffix.begin(), suffix.end());
                
                suffix+=tolower(c);
                
                if (
                    (
                        !number_found_period && !number_found_e &&
                        (suffix=="ull" || suffix=="llu" || suffix=="ul" || suffix=="lu" || suffix=="u" || suffix=="l" || suffix=="ll")
                    ) || (
                        !number_found_x &&
                        (suffix=="f" || suffix=="l")
                    )
                ) {
                    add_buffer();
                    number_found_suffix=true;
                }
            }
            
            if (!processed) {
                flush_buffer(false);
            }
        } else
        if (current_token==token_type_identifier) {
            if (is_letter || is_decimal) {
                add_buffer();
            } else {
                if (keywords.count(buffer)) {
                    current_token=token_type_keyword;
                }
                flush_buffer(false);
            }
        } else
        if (current_token==token_type_space) {
            if (is_space) {
                add_buffer();
            } else {
                flush_buffer(false);
            }
        } else
        if (current_token==token_type_operator) {
            bool force_flush=false;
            
            if (is_symbol) {
                add_buffer();
            } else {
                force_flush=true;
            }
            
            while (buffer.size()==max_multi_char_operator_size || (!buffer.empty() && force_flush)) {
                for (int x=buffer.size();x>=1;--x) {
                    if (x==1 || multi_char_operators.count(buffer.substr(0, x))) {
                        string rest_of_buffer=buffer.substr(x);
                        buffer.resize(x);
                        
                        if (buffer.size()==1 && is_opening_bracket(buffer[0])) {
                            ++net_brackets;
                            found_opening_bracket=true;
                        } else
                        if (buffer.size()==1 && is_closing_bracket(buffer[0])) {
                            if (!found_opening_bracket) {
                                ++num_closing_brackets_before_first_opening_bracket;
                            }
                            --net_brackets;
                        }
                        
                        flush_buffer(!rest_of_buffer.empty());
                        buffer=move(rest_of_buffer);
                        current_token=token_type_operator;
                        break; //out of for loop
                    }
                }
            }
            
            if (force_flush) {
                assert(buffer.empty());
                flush_buffer(false); //reset current_token
            }
        }
    }
    
    void loop_finish() override {
        assert(
            current_token==token_type_string_single ||
            current_token==token_type_string_double ||
            current_token==token_type_comment_c ||
            current_token==token_type_preprocessor ||
            current_token==token_type_space
        );
    }
};

pair<vector<s8>, line_state> tokenize(line_state previous_line_state, const string& current_line) {
    c_language_tokenizer t(previous_line_state.token, current_line);
    auto last_token=t.process();
    
    line_state res_line_state((last_token<0)? -last_token : last_token);
    res_line_state.net_brackets=t.net_brackets;
    res_line_state.num_closing_brackets_before_first_opening_bracket=t.num_closing_brackets_before_first_opening_bracket;
    res_line_state.num_spaces_at_start=find_if(current_line.begin()+1, current_line.end(), [&](char c){ return c!=' '; })-(current_line.begin()+1);
    
    return make_pair(move(t.res), res_line_state);
}


}}
