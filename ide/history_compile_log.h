//will try to extract a filename, line number, and column number from each line. does not care if it says error, warning, etc
//will have syntax highlighting for error messages. filename, line, column, and strings are highlighted
//need to handle "in file included from"
//need line wrapping to actually display the error messages, but should be disabled by default
//uses same line wrapping implementation as ide
//errors displayed similar to search results; f3 advances to next error
//errors displayed on the output pane when the user enters the "compiler-output" command which has a hotkey

//line wrapping:
//-the currently selected line is wrapped. no other lines are wrapped
//-a wrapped line may take up multiple lines; affects renderer, etc
//-a wrapped line is indented using the source line's indentation
//-implemented at low level in the editor; higher levels unaware of line wrapping
//-the wrapped portion of the line has a different background color than the unwrapped part
//-

// In file included from ide.cpp:17:0:
// history_data.h:312:19: error: expected unqualified-id before 'int'
// history_data.h: In member function 'history::character& history::filef::insert_character(const history::character&, int, char, int)':
// /cygdrive/h/Programming/0-Projects/_Repository/3-Shared/shared/generic.h:31:25: note: in definition of macro 'assert'
// In file included from /usr/lib/gcc/i686-pc-cygwin/4.9.3/include/c++/bits/stl_tree.h:67:0,
//                  from include.h:7:
// /usr/lib/gcc/i686-pc-cygwin/4.9.3/include/c++/bits/shared_ptr.h:604:42:   required from 'std::shared_ptr<_Tp1> std::make_shared(_Args&& ...) [with _Tp = history::file; _Args = {int&, int&}]'
// history_data.h:569:53:   required from here

namespace history {


namespace tokenizer {
    struct gcc_compiler_log_tokenizer : public generic_tokenizer {
        bool string_backslash=false;
        
        string path;
        int line_number;
        int column_number;
        
        void flush_buffer(bool disable_assert) {
            generic_tokenizer::flush_buffer(disable_assert);
        }
        
        gcc_compiler_log_tokenizer(const string& t_current_line) :
            generic_tokenizer(token_type_compile_log_path, t_current_line), line_number(-1), column_number(-1)
        {}
        
        void process_invalid_token_impl() override {
            assert(false);
        }
        
        void loop_inner() override {
            if (current_token==token_type_compile_log_path) {
                if (c==':') {
                    auto i=buffer.find_last_of(' ');
                    if (i==string::npos) {
                        path=buffer;
                    } else {
                        path=buffer.substr(i+1);
                    }
                    flush_buffer(false);
                    
                    current_token=token_type_compile_log_line;
                }
                
                add_buffer();
            } else
            if (current_token==token_type_compile_log_line || current_token==token_type_compile_log_column) {
                if (c==':' || c==' ') {
                    auto value=checked_from_string<int>(buffer.substr(1));
                    
                    if (current_token==token_type_compile_log_line) {
                        line_number=(value.second)? value.first : -1;
                        
                        flush_buffer(false);
                        current_token=token_type_compile_log_column;
                    } else {
                        assert(current_token==token_type_compile_log_column);
                        column_number=(value.second)? value.first : -1;
                        
                        flush_buffer(false);
                        current_token=token_type_compile_log_other;
                    }
                }
                
                add_buffer();
            } else {
                assert(current_token==token_type_compile_log_other);
                add_buffer();
                /*if (c=='\\') {
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
                }*/
            }
        }
        
        void loop_finish() override {}
    };
    
    vector<s8> tokenize_gcc_compiler_log_line(const string& current_line, string& out_path, int& out_line_number, int& out_column_number) {
        gcc_compiler_log_tokenizer t(current_line);
        t.process();
        
        out_path=move(t.path);
        out_line_number=t.line_number;
        out_column_number=t.column_number;
        return move(t.res);
    }
}


}
