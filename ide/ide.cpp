#include "include.h"
using namespace std;
using namespace generic;
using namespace graphics;
using namespace math;
using namespace fixed_size;

const int tab_width=4;
const int search_max_identifier_length=128;

#include "util.h"

#include "render.h"
#include "ui_element.h"

#include "history_tokenizer.h"
#include "history_persistance.h"

#include "search.h"

#include "history_data.h"
#include "history_sync.h"
#include "history_main.h"

#include "history_compile_log.h"
#include "highlighting.h"

#include "editor_file_reference.h"
#include "editor.h"

#include "history_analyze.h"

#undef main

int main(int argc, char** argv) {
    string working_directory_path=".";
    if (argc==2) {
        working_directory_path=argv[1];
    }
    
    history::history_main history_main(working_directory_path, [&](const string& s){ cerr << s << "\n"; });
    auto& state=history_main.get_state();
    
    highlighting search_highlighting(state);
    highlighting auto_search_highlighting(state);
    bool display_auto_search_highlighting=true;
    
    //
    //
    
    ivec2 screen_size(800, 600);
    
    do_sdl(SDL_Init(SDL_INIT_VIDEO));
    
    SDL_Window* sdl_window=SDL_CreateWindow(
        "IDE",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screen_size.x, screen_size.y,
        SDL_WINDOW_RESIZABLE
    );
    assert(sdl_window!=nullptr);
    
    init_renderer(sdl_window);
    
    texture font(postprocess_font(read_bmp("courier_new_8pt.bmp")));
    ivec2 font_size=font_character_size(*font);
    
    //
    //
    
    syntax_highlighting c_syntax_highlighting;
    {
        rgba fg_default(188, 255,  64);
        rgba bg_default( 55,  85,   0);
        rgba fg_string (255, 102,   0);
        rgba bg_comment(  0,   0,   0);
        rgba fg_number (  0, 204, 255);
        rgba fg_hash   (255, 204,   0);
        rgba fg_keyword(255, 255, 255);
        rgba bg_current(  0, 128,   0);
        rgba bg_tabs_1 ( 73, 110,   0);
        rgba bg_tabs_2 ( 46,  70,   0);
        rgba fg_selected=bg_default;
        rgba bg_selected=fg_default;
        rgba bg_highlighted(0, 32, 64);
        rgba bg_highlighted_selected(192, 224, 255);
        
        vector<rgba> age_colors={
            rgba(255,  65,  64),
            rgba(255, 123,  64),
            rgba(255, 195,  64),
            rgba(255, 253,  64),
            rgba(235, 255,  64),
            rgba(188, 255,  64)
        };
        double age_multiplier=1;
        
        //rgba bg_gutter(83, 128, 0);
        
        char_format token_string (false, false, fg_string , bg_default);
        char_format token_comment(false,  true, fg_default, bg_comment);
        char_format token_number (false, false, fg_number , bg_default);
        char_format token_hash   (false, false, fg_hash   , bg_default);
        char_format token_name   ( true, false, fg_default, bg_default);
        char_format token_keyword( true, false, fg_keyword, bg_default);
        char_format token_symbol (false, false, fg_default, bg_default);
        
        char_format token_group_header_default     (false, false, fg_default, bg_default);
        char_format token_group_header_current     (false, false, fg_default, bg_comment);
        char_format token_group_indentation_default(false, false, fg_default, bg_default);
        char_format token_group_indentation_current(false, false, fg_default, bg_comment);
        
        c_syntax_highlighting.token_colors={
            make_pair(history::tokenizer::token_type_invalid, token_name),
            make_pair(history::tokenizer::token_type_string_single, token_string),
            make_pair(history::tokenizer::token_type_string_double, token_string),
            make_pair(history::tokenizer::token_type_comment_c, token_comment),
            make_pair(history::tokenizer::token_type_comment_cpp, token_comment),
            make_pair(history::tokenizer::token_type_number, token_number),
            make_pair(history::tokenizer::token_type_preprocessor, token_hash),
            make_pair(history::tokenizer::token_type_identifier, token_name),
            make_pair(history::tokenizer::token_type_keyword, token_keyword),
            make_pair(history::tokenizer::token_type_space, token_symbol),
            make_pair(history::tokenizer::token_type_operator, token_symbol),
            make_pair(history::tokenizer::token_type_group_header_default, token_group_header_default),
            make_pair(history::tokenizer::token_type_group_header_current, token_group_header_current),
            make_pair(history::tokenizer::token_type_group_indentation_default, token_group_indentation_default),
            make_pair(history::tokenizer::token_type_group_indentation_current, token_group_indentation_current),
            make_pair(history::tokenizer::token_type_compile_log_path, token_name),
            make_pair(history::tokenizer::token_type_compile_log_line, token_number),
            make_pair(history::tokenizer::token_type_compile_log_column, token_number),
            make_pair(history::tokenizer::token_type_compile_log_other, token_name),
        };
        
        c_syntax_highlighting.fg_color_selected=fg_selected;
        c_syntax_highlighting.bg_color_selected=bg_selected;
        c_syntax_highlighting.bg_color_indentation_1=bg_tabs_1;
        c_syntax_highlighting.bg_color_indentation_2=bg_tabs_2;
        c_syntax_highlighting.bg_color_trailing_whitespace=bg_default;
        c_syntax_highlighting.bg_color_body_current_line=bg_current;
        c_syntax_highlighting.bg_color_trailing_whitespace_current_line=bg_current;
        c_syntax_highlighting.bg_color_highlighted=bg_highlighted;
        c_syntax_highlighting.bg_color_highlighted_selected=bg_highlighted_selected;
        
        c_syntax_highlighting.age_tokens={history::tokenizer::token_type_identifier};
        c_syntax_highlighting.age_multiplier=age_multiplier;
        c_syntax_highlighting.age_colors=age_colors;
    }
    
    ui_elements ui;
    
    history_file_reference left_reference(state);
    
    temporary_file_reference right_reference_analysis;
    temporary_file_reference right_reference_output;
    temporary_file_reference right_reference_auto_search;
    temporary_file_reference* c_right_reference=&right_reference_analysis;
    
    temporary_file_reference info_reference;
    temporary_file_reference command_reference;
    
    file_editor left_editor(c_syntax_highlighting, font, left_reference);
    file_editor right_editor(c_syntax_highlighting, font, *c_right_reference);
    file_editor info_editor(c_syntax_highlighting, font, info_reference);
    file_editor command_editor(c_syntax_highlighting, font, command_reference);
    
    right_reference_analysis.highlight_current=false;
    
    bool newly_executed_command=false;
    string last_executed_command="<none>";
    
    map<string, function<void(const string& arguments, temporary_file_reference& output)>> command_operators;
    
    auto select_characters=[&](int id_start, int id_end) {
        auto c_character=state.character_by_id(id_start);
        if (c_character==nullptr) {
            return; //file can get deleted after results are calculated
        }
        
        auto c_file=c_character->parent;
        
        if (c_file!=&*(left_reference.file().lock())) {
            left_reference.bind(state.get_file(c_file->character_id()));
            left_editor.bind(left_reference);
        }
        
        ui.select(left_editor);
        left_editor.move_cursor_to_range(
            c_file->get_position(*state.character_by_id(id_start)),
            c_file->get_position(*state.character_by_id(id_end))
        );
    };
    
    auto split_arguments=[&](const string& arguments) {
        vector<string> res;
        
        string buffer;
        for (int x=0;x<=arguments.size();++x) {
            char c=(x==arguments.size())? ' ' : arguments[x];
            if (c==' ') {
                if (!buffer.empty()) {
                    res.push_back(move(buffer));
                    buffer.clear();
                }
            } else {
                buffer+=c;
            }
        }
        
        return res;
    };
    
    command_operators["file"]=[&](const string& arguments, temporary_file_reference& output) {
        auto args=split_arguments(arguments);
        auto file_name=(args.size()==0)? string() : args[0];
        
        int line_number=-1;
        if (args.size()==2) {
            auto line=checked_from_string<int>(args[1]);
            if (line.second) {
                line_number=line.first-1;
            } else {
                output.push_back("\nInvalid line number.");
                return;
            }
        }
        
        for (auto c=state.files_by_name().begin();c!=state.files_by_name().end();++c) {
            if (file_name.empty() || c->first.find(file_name)!=string::npos) {
                int id=c->second;
                output.push_back_group({temporary_file_reference::push_back_data("\n"+ c->first)}, [id, select_characters, line_number, &left_editor, &ui]() {
                    select_characters(id, id);
                    if (line_number!=-1) {
                        left_editor.move_cursor_to_line(line_number);
                    }
                });
            }
        }
        
        if (output.num_lines()==0) {
            output.push_back("\nNo results found.");
        }
    };
    
    command_operators["goto"]=[&](const string& arguments, temporary_file_reference& output) {
        auto line=checked_from_string<int>(arguments);
        if (line.second) {
            left_editor.move_cursor_to_line(line.first-1);
        } else {
            output.push_back("\nInvalid line number.");
        }
    };
    
    command_operators["find"]=[&](const string& arguments, temporary_file_reference& output) {
        if (arguments.empty()) {
            search_highlighting.clear();
            display_auto_search_highlighting=true;
            return;
        }
        
        const int max_identifier_results=10000;
        const int max_iterations=10000;
        const int max_token_results=10000;
        auto search_results=state.search_index().lookup(arguments, max_identifier_results, max_iterations, max_token_results);
        
        if (search_results.empty()) {
            output.push_back("\nNo results found.");
            display_auto_search_highlighting=true;
            return;
        }
        
        vector<pair<int, int>> current_file_matches;
        vector<pair<int, int>> other_file_matches;
        
        auto current_file=left_reference.file().lock().get();
        
        for (auto c=search_results.begin();c!=search_results.end();++c) {
            for (auto d=c->second.begin();d!=c->second.end();++d) {
                auto character=state.character_by_id(d->first);
                if (character!=nullptr && character->parent==current_file) {
                    current_file_matches.push_back(*d);
                } else {
                    other_file_matches.push_back(*d);
                }
            }
        }
        
        for (int x=0;x<2;++x) {
            auto analysis_results=history::generate_analysis_results(state, [&](int n_raw) {
                auto& matches=(x==0)? current_file_matches : other_file_matches;
                
                if (n_raw>=matches.size()) {
                    return -1;
                } else {
                    return matches[n_raw].first;
                }
            });
            history::visualize(state, analysis_results, output, select_characters, x==0);
        }
        
        search_highlighting.bind(move(search_results));
        
        display_auto_search_highlighting=false;
    };
    
    command_operators["compiler"]=[&](const string& arguments, temporary_file_reference& output) {
        ifstream in(working_directory_path+ "compile.log");
        while (true) {
            string buffer;
            getline(in, buffer);
            if (!in) {
                break;
            }
            
            buffer="\n"+ buffer;
            
            string path;
            int line_number;
            int column_number;
            auto tokens=history::tokenizer::tokenize_gcc_compiler_log_line(buffer, path, line_number, column_number);
            
            auto file=state.files_by_name().find(path);
            if (file!=state.files_by_name().end()) {
                int target_character_id;
                
                int file_id=file->second;
                auto file_data=state.get_file(file_id).lock();
                if (line_number>=0 && line_number<file_data->num_lines()) {
                    auto& line_data=file_data->get_line(line_number);
                    if (column_number>=0 && column_number<line_data.cached_value_characters.size()) {
                        target_character_id=line_data.cached_value_characters[column_number]->character_id();
                    } else {
                        target_character_id=line_data.cached_value_characters.at(0)->character_id();
                    }
                } else {
                    target_character_id=file_id;
                }
                
                output.push_back_group(
                    {temporary_file_reference::push_back_data(buffer, tokens)},
                    [target_character_id, select_characters, &left_editor, &ui]() {
                        select_characters(target_character_id, target_character_id);
                    }
                );
            } else {
                output.push_back(buffer, tokens);
            }
        }
        
        if (output.num_lines()==0) {
            output.push_back("\nNothing found in compile.log .");
        }
    };
    
    auto execute_command=[&](const string& command) {
        right_reference_output.clear();
        
        auto first_space_index=command.find(' ');
        string command_name=(first_space_index==string::npos)? command : command.substr(0, first_space_index);
        string command_arguments=(first_space_index==string::npos)? string() : command.substr(first_space_index+1);
        
        auto operator_i=command_operators.find(command_name);
        if (operator_i==command_operators.end()) {
            right_reference_output.push_back("\nError: Unknown command.");
            right_reference_output.push_back("\nValid commands are:");
            for (auto c=command_operators.begin();c!=command_operators.end();++c) {
                right_reference_output.push_back("\n    "+ c->first);
            }
        } else {
            operator_i->second(command_arguments, right_reference_output);
        }
        
        ui.select(left_editor);
        last_executed_command=command;
        newly_executed_command=true;
    };
    
    auto input_command=[&](const string& command) {
        command_editor.input_command_string(command +" ");
        ui.select(command_editor);
    };
    
    command_reference.read_only=false;
    command_editor.is_command=true;
    command_editor.command_callback=execute_command;
    
    bool analysis_measures_views=true;
    bool analysis_is_files=false;
    
    bool only_display_left_editor=false;
    
    ui.add(left_editor);
    ui.add(right_editor);
    ui.add(info_editor);
    ui.add(command_editor);
    
    info_reference.push_back("\n");
    auto update_info_reference=[&]() {
        string file_name="<none>";
        int current_line=left_editor.current_line()+1;
        int num_lines=0;
        
        auto c_file=left_reference.file().lock();
        if (c_file) {
            file_name=c_file->name;
            num_lines=c_file->num_lines();
        }
        
        string right_panel_name;
        if (c_right_reference==&right_reference_analysis) {
            right_panel_name=string("Recently ") + (analysis_measures_views? "viewed or edited " : "edited ") + (analysis_is_files? "files" : "lines");
        } else
        if (c_right_reference==&right_reference_output) {
            right_panel_name="Output of '"+ last_executed_command +"'";
        } else
        if (c_right_reference==&right_reference_auto_search) {
            right_panel_name="Auto search";
        } else {
            assert(false);
        }
        
        string expected_line=
            "\n"+ file_name +" (line "+ to_string(current_line) +" of "+ to_string(num_lines) +")"+
            (only_display_left_editor? "" : "            "+ right_panel_name)
        ;
        
        if (info_reference.lines[0].value!=expected_line) {
            info_reference.lines[0].value=expected_line;
            info_editor.move_cursor_to_line(0);
        }
    };
    
    command_reference.push_back("\n");
    
    auto init_ui=[&]() {
        int info_height=font_size.y;
        int command_height=font_size.y;
        int left_pane_y=(only_display_left_editor)? info_height : 0;
        int left_pane_height=(only_display_left_editor)? screen_size.y-info_height-command_height : screen_size.y;
        int right_pane_height=(only_display_left_editor)? 0 : screen_size.y-info_height-command_height;
        int left_pane_width=(only_display_left_editor)? screen_size.x : screen_size.x/2;
        int right_pane_width=(only_display_left_editor)? 0 : screen_size.x/2;
        int info_command_x=(only_display_left_editor)? 0 : left_pane_width;
        int info_command_width=(only_display_left_editor)? left_pane_width : right_pane_width;
        int command_y=(only_display_left_editor)? info_height+left_pane_height : info_height+right_pane_height;
        
        left_editor.resize_pixels(ivec2(left_pane_width, left_pane_height), ivec2(0, left_pane_y));
        right_editor.resize_pixels(ivec2(right_pane_width, right_pane_height), ivec2(left_pane_width, info_height));
        
        info_editor.resize_pixels(ivec2(info_command_width, info_height), ivec2(info_command_x, 0));
        command_editor.resize_pixels(ivec2(info_command_width, command_height), ivec2(info_command_x, command_y));
        
        ui.set_redraw();
    };
    
    init_ui();
    
    ui.select(left_editor);
    
    history::analysis_results c_analysis_results;
    
    auto do_analysis=[&]() {
        if (analysis_is_files) {
            c_analysis_results=history::analyze_files(state, analysis_measures_views);
        } else {
            c_analysis_results=history::analyze_lines(state, analysis_measures_views);
        }
        
        history::visualize(state, c_analysis_results, right_reference_analysis, select_characters);
        right_editor.move_cursor_to_line(0, false);
        ui.set_redraw();
    };
    
    do_analysis();
    
    right_reference_analysis.seek_current_group(1);
    
    //
    //
    
    auto do_auto_search=[&]() {
        right_reference_auto_search.clear();
        right_editor.move_cursor_to_line(0, false);
        ui.set_redraw();
        
        string query=left_editor.current_identifier(false);
        
        if (query.empty()) {
            return;
        }
        
        vector<string> query_array={query};
        
        const int max_identifier_results=10000;
        const int max_iterations=10000;
        const int max_token_results=10000;
        auto search_results=state.search_index().lookup(query_array, false, true, max_identifier_results, max_iterations, max_token_results);
        
        if (search_results.empty()) {
            return;
        }
        
        vector<tuple<string, int /*matches in current file*/, int /*matches in all files*/>> processed_search_results;
        vector<pair<int, int>>* auto_search_highlighting_data=nullptr;
        
        for (auto c=search_results.begin();c!=search_results.end();++c) {
            processed_search_results.push_back(make_tuple(c->first, 0, c->second.size()));
            
            for (auto d=c->second.begin();d!=c->second.end();++d) {
                auto character=state.character_by_id(d->first);
                if (character!=nullptr && character->parent==left_reference.file().lock().get()) {
                    ++std::get<1>(processed_search_results.back());
                }
            }
            
            if (c->first==query) {
                auto_search_highlighting_data=&(c->second);
            }
        }
        
        if (auto_search_highlighting_data!=nullptr) {
            auto_search_highlighting.bind(move(*auto_search_highlighting_data));
        } else {
            auto_search_highlighting.clear();
        }
        
        sort(processed_search_results.begin(), processed_search_results.end(), [&](const tuple<string, int, int>& a, const tuple<string, int, int>& b) {
            return std::get<2>(a) > std::get<2>(b);
        });
        
        for (auto c=processed_search_results.begin();c!=processed_search_results.end();++c) {
            //right_reference_auto_search.push_back("\n"+ std::get<0>(*c) +" ("+ to_string(std::get<1>(*c)) +"/"+ to_string(std::get<2>(*c)) +")");
            right_reference_auto_search.push_back("\n"+ std::get<0>(*c));
        }
    };
    
    auto do_auto_complete=[&]() {
        string query=left_editor.current_identifier(true);
        //cerr << query << "\n";
        if (query.empty()) {
            return;
        }
        
        vector<string> query_array={query};
        
        const int max_identifier_results=10000;
        const int max_iterations=10000;
        const int max_token_results=100000;
        auto search_results=state.search_index().lookup(query_array, false, true, max_identifier_results, max_iterations, max_token_results);
        
        if (search_results.empty()) {
            return;
        }
        
        string match;
        for (auto c=search_results.begin();c!=search_results.end();++c) {
            if (c->first.size()==query.size()) {
                continue;
            }
            
            if (match.empty()) {
                match=c->first;
            } else {
                if (match.size()<c->first.size()) {
                    match.resize(c->first.size());
                }
                
                for (int x=0;x<match.size();++x) {
                    if (tolower(match[x])!=tolower(c->first[x])) {
                        match.resize(x);
                        break;
                    }
                }
            }
            //cerr << c->first << " -> " << match << "\n";
        }
        
        if (match.size()>query.size()) {
            left_editor.input_string(match.substr(query.size()));
        }
    };
    
    //
    //
    
    bool quit_pending=false;
    while (!quit_pending) {
    /*int xxx=0;
    while (true) {
        ++xxx;
        if (xxx%100==0) {
            cout << ((double)xxx)/SDL_GetTicks()*1000 << "\n";
        }*/
        
        SDL_WaitEvent(nullptr);
        //SDL_WaitEventTimeout(nullptr, 100);
        
        //required for searches
        state.cache_all_lines();
        
        bool changed_right_pane=false;
        int seek_right_pane_line_1=0;
        int seek_right_pane_line_2=0;
        
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type==SDL_KEYDOWN || e.type==SDL_KEYUP) {
                bool is_down=(e.type==SDL_KEYDOWN);
                int key=e.key.keysym.sym;
                
                if (is_down) {
                    pressed_keys.insert(key);
                } else {
                    pressed_keys.erase(key);
                }
                
                shift_pressed=pressed_keys.count(SDLK_LSHIFT) || pressed_keys.count(SDLK_RSHIFT);
                control_pressed=pressed_keys.count(SDLK_LCTRL) || pressed_keys.count(SDLK_RCTRL);
                alt_pressed=pressed_keys.count(SDLK_LALT) || pressed_keys.count(SDLK_RALT);
                
                ui.input_key(is_down, key);
                
                if (is_down) {
                    //note: both export and import will also export files that can be safely exported even if alt is not pressed
                    //(i.e. doing a safe export does not require the alt key to be pressed)
                    
                    //right pane -> output
                    if (key==SDLK_F1) {
                        c_right_reference=&right_reference_output;
                        changed_right_pane=true;
                        
                        auto res=right_reference_output.get_current_group();
                        if (res.first!=-1) {
                            seek_right_pane_line_1=res.first;
                            seek_right_pane_line_2=res.second;
                        }
                    }
                    
                    //right pane -> compiler log
                    if (key==SDLK_F2) {
                    }
                    
                    //seek output forward (f3), or backward (shift-f3)
                    if (key==SDLK_F3) {
                        right_reference_output.seek_current_group(shift_pressed? -1 : 1);
                        auto res=right_reference_output.get_current_group();
                        if (c_right_reference==&right_reference_output && res.first!=-1) {
                            changed_right_pane=true;
                            seek_right_pane_line_1=res.first;
                            seek_right_pane_line_2=res.second;
                        }
                    }
                    
                    //autosearch
                    if (key==SDLK_F4) {
                        c_right_reference=&right_reference_auto_search;
                        changed_right_pane=true;
                        display_auto_search_highlighting=true;
                    }
                    
                    //right pane -> analyze viewed/edited lines
                    //right pane -> analyze edited lines
                    //right pane -> analyze viewed/edited files
                    //right pane -> analyze edited files
                    if (key==SDLK_F5 || key==SDLK_F6 || key==SDLK_F7 || key==SDLK_F8) {
                        analysis_measures_views=(key==SDLK_F5 || key==SDLK_F7);
                        analysis_is_files=(key==SDLK_F7 || key==SDLK_F8);
                        c_right_reference=&right_reference_analysis;
                        changed_right_pane=true;
                        c_analysis_results.last_log_index=-1;
                    }
                    
                    //toggle right pane visibility
                    if (key==SDLK_F9) {
                        only_display_left_editor=!only_display_left_editor;
                        init_ui();
                    }
                    
                    //
                    //
                    
                    //persist (alt-s just persists, ctrl-s or ctrl-alt-s persists and then exports)
                    if ((control_pressed || alt_pressed) && key==SDLK_s) {
                        cerr << "Persist\n";
                        history_main.persist();
                    }
                    
                    //export memory -> disk
                    if (control_pressed && key==SDLK_s) {
                        cerr << "Export memory -> disk\n";
                        history_main.sync(false, !alt_pressed);
                    }
                    
                    //import disk -> memory
                    if (control_pressed && key==SDLK_i) {
                        cerr << "Import disk -> memory\n";
                        history_main.sync(true, !alt_pressed);
                    }
                    
                    //view compiler output
                    if (!control_pressed && alt_pressed && key==SDLK_c) {
                        input_command("compiler");
                    }
                    
                    //find text in all files
                    if (control_pressed && !alt_pressed && key==SDLK_f) {
                        input_command("find");
                    }
                    
                    //find files
                    if (!control_pressed && alt_pressed && key==SDLK_f) {
                        input_command("file");
                    }
                    
                    //go to line
                    if (control_pressed && !alt_pressed && key==SDLK_g) {
                        input_command("goto");
                    }
                    
                    //autocomplete
                    if (!control_pressed && !alt_pressed && key==SDLK_BACKQUOTE) {
                        do_auto_complete();
                    }
                }
                //cout << "keyboard " << is_down << ", " << key << ", " << "\n";
            } else
            if (e.type==SDL_MOUSEBUTTONDOWN || e.type==SDL_MOUSEBUTTONUP) {
                bool is_down=(e.type==SDL_MOUSEBUTTONDOWN);
                bool double_click=(e.button.clicks==2);
                int button=e.button.button; //1 - left, 2 - middle, 3 - right
                int x=e.button.x;
                int y=e.button.y;
                mouse_position=ivec2(x, y);
                
                if (is_down && button==1) {
                    ui.select_at(mouse_position);
                    left_mouse_button_pressed=true;
                }
                
                if (!is_down && button==1) {
                    left_mouse_button_pressed=false;
                }
                
                ui.input_mouse_button(is_down, double_click, button);
                //cout << "mouse button " << is_down << ", " << button << "\n";
            } else
            if (e.type==SDL_MOUSEMOTION) {
                int x=e.motion.x;
                int y=e.motion.y;
                mouse_position=ivec2(x, y);
                
                ui.input_mouse_move();
                //cout << "mouse motion " << x << ", " << y << "\n";
            } else
            if (e.type==SDL_QUIT) {
                quit_pending=true;
                //cout << "quit " << "\n";
            } else
            if (e.type==SDL_TEXTINPUT) {
                char c=e.text.text[0];
                if (c>=33 && c<=126 && !control_pressed && !alt_pressed) {//(control_pressed || (c!='`' && c!='~'))) {
                    ui.input_text(c);
                }
                //cout << "text input " << e.text.text[0] << "\n";
            } else
            if (e.type==SDL_MOUSEWHEEL) {
                int dy=-e.wheel.y;
                ui.input_mouse_wheel(dy);
                //cout << "wheel " << -e.wheel.y << "\n";
            } else
            if (e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_RESIZED) {
                screen_size.x=e.window.data1;
                screen_size.y=e.window.data2;
                init_renderer(sdl_window);
                init_ui();
                //cout << "resize " << width << ", " << height << "\n";
            } else
            if (e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_EXPOSED) {
                ui.set_redraw();
            }
        }
        
        if (newly_executed_command) {
            if (right_reference_output.num_lines()!=0) {
                c_right_reference=&right_reference_output;
                changed_right_pane=true;
            }
            newly_executed_command=false;
            ui.set_redraw();
        }
        
        if (changed_right_pane) {
            right_editor.bind(*c_right_reference);
            right_editor.move_cursor_to_line(seek_right_pane_line_1);
            right_editor.move_cursor_to_line(seek_right_pane_line_2);
            ui.set_redraw();
        }
        
        if (
            c_right_reference==&right_reference_analysis &&
            c_analysis_results.last_log_index!=((analysis_measures_views)? state.last_viewed_log_index() : state.last_modified_log_index())
        ) {
            do_analysis();
        }
        
        if (display_auto_search_highlighting) {
            do_auto_search();
            left_reference.bind(&auto_search_highlighting);
        } else {
            left_reference.bind(&search_highlighting);
        }
        
        if (ui.needs_render()) {
            search_highlighting.refresh();
            update_info_reference();
            render_clear(rgba(0, 0, 0, 0));
            ui.render();
            SDL_RenderPresent(sdl_renderer);
        }
    }
    
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    
    SDL_Quit();
    
    //CloseHandle(named_event);
    
    return 0;
}




    //const string ipc_file_name=string(getenv("TEMP")) +"/ilyas_ide_ipc_file";
    //this is retarded. seriously, could they come up with a more complicated way of doing this?
    //why can't you just give the window a global name, check if it exists, and send a message to it
    
    //just use files. add a max timeout of like 100ms to the sdl_waitevent call and have the ide open a file each loop iteration
    //-the file is deleted when the main process exits
    //-the main process will replace any data that it already read with null bytes
    //-just append to the file to write to it
    //-the file should be located in the ide_data directory so that multiple instances of the ide can be started for different projects
    //-procedure for child process:
    //--try to open the log file in write mode. if this is possible, the ide is not running even if the ipc file exists
    //--
    /*HANDLE named_event; {
        named_event=CreateEvent(nullptr, false, false, "Global\\ilyas_ide_ipc_event");
        if (named_event==nullptr) {
            assert(false);
        }
        
        if (GetLastError()==ERROR_ALREADY_EXISTS) {
            while (true) {
                ofstream out(ipc_file_name.c_str(), ios::app);
                out << "foo bar\n";
                
                if (out.good()) {
                    break;
                }
                
                Sleep(10);
            }
            SetEvent(named_event); //of course you can't actually pass any data here because of course not
            CloseHandle(named_event);
            return 0;
        }
        
        //... start a new thread to monitor the fucking event
        //... call SDL_PushEvent from the thread and set a flag. call resetevent on the event. keep monitoring the event
        //... the event loop in the main thread should open the ipc file and read all of the lines. if the file is locked,
        // keep trying to open it. it should eventually open. read the contents and replace them with null characters or something since
        // the file can't actually be truncated in an iostream
        //... when the main application exits, delete the file
    }*/
