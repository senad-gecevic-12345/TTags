#include "C_Main.h"
#include <string>
#include <wx/event.h>



BEGIN_EVENT_TABLE(C_Main, wxFrame)
	EVT_SIZE(C_Main::resize)
END_EVENT_TABLE()

namespace LIST_FUNC{
    static bool has_selection(wxListBox* list){
        return list->GetSelection() != wxNOT_FOUND;
    }
    static wxString get_str(wxListBox* list){
        return list->GetStringSelection();
    }
};

struct VIM_KEYS {
	enum {
		W = 10001,
		W_CANCEL,
		H,
		L,
		RESIZE_PLUS,
		RESIZE_MINUS,
		DEFAULT_STYLE,
		NO
	};
};

struct WIDGETS {
	enum{
		ADD = VIM_KEYS::NO,
		PUBLISH,
		LIST,
		NO
	};
};

struct FILECTRL {
	enum{
		SAVE = WIDGETS::NO,
		LOAD,
		NO
	};
};



void C_Main::update_accell_table() {

	wxAcceleratorEntry entries[9];
	entries[0].Set(wxACCEL_NORMAL,	(int) 'H', VIM_KEYS::H);
	entries[1].Set(wxACCEL_NORMAL,	(int) 'L', VIM_KEYS::L);
	entries[2].Set(wxACCEL_CTRL,	(int) 'W', VIM_KEYS::W);
	entries[3].Set(wxACCEL_NORMAL,	(int) WXK_ESCAPE, VIM_KEYS::W_CANCEL);

	entries[4].Set(wxACCEL_CTRL,	(int) 'H', VIM_KEYS::H);
	entries[5].Set(wxACCEL_CTRL,	(int) 'L', VIM_KEYS::L);
	// works but not Y X
	entries[6].Set(wxACCEL_CTRL,	(int) '+', VIM_KEYS::RESIZE_PLUS); // is it already reserved?
	entries[7].Set(wxACCEL_CTRL,	(int) '-', VIM_KEYS::RESIZE_MINUS);

	entries[8].Set(wxACCEL_CTRL,	(int) WXK_F1, VIM_KEYS::DEFAULT_STYLE);



	//entries[6].Set(wxACCEL_SHIFT,	(int) NK_SEMICOLON, FILECTRL::SAVE);

	wxAcceleratorTable table(9, entries);
	this->SetAcceleratorTable(table);
}

void C_Main::clear_accell_table() {
	wxAcceleratorEntry entries[4];
	entries[0].Set(wxACCEL_CTRL, 	(int) 'W', VIM_KEYS::W);
	entries[1].Set(wxACCEL_CTRL,	(int) '+', VIM_KEYS::RESIZE_PLUS);
	entries[2].Set(wxACCEL_CTRL,	(int) '-', VIM_KEYS::RESIZE_MINUS);
	entries[3].Set(wxACCEL_CTRL,	(int) WXK_F1, VIM_KEYS::DEFAULT_STYLE);

	wxAcceleratorTable table(4, entries);
	this->SetAcceleratorTable(table);
}


C_Main::C_Main() : wxFrame(nullptr, wxID_ANY, "Window", wxPoint(0, 0), wxSize(800, 600)){
	auto color = wxColor(6, 20, 46);
	this->SetBackgroundColour(color);


	add = new wxButton(this, WIDGETS::ADD, "Add", wxPoint(10, 40), wxSize(150, 40));
	//focus = new wxObject();
	publish = new wxButton(this, WIDGETS::PUBLISH, "Publish", wxPoint(160, 40), wxSize(150, 40));
    txt_1 = new wxTextCtrl(this, wxID_ANY, "", wxPoint(10, 85), wxSize(300, 30));
	vim_editor = new wxRichTextCtrl(this, wxID_ANY, "", wxPoint(400, 140), wxSize(300, 300), wxTE_MULTILINE);
	list = new wxListBox(this, WIDGETS::LIST, wxPoint(10, 140), wxSize(300, 300));
    auto* menu = new wxMenuBar();
	auto* nav = new wxMenu();
	auto* file = new wxMenu();
	vim_command_line = (new wxTextCtrl(this, wxID_ANY, ""));
	vim_command_history = (new wxTextCtrl(this, wxID_ANY, "", wxPoint(), wxSize(), wxTE_MULTILINE));

	add->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& evt)->void {
		if (txt_1->IsEmpty()) { return; }
		list->AppendString(txt_1->GetValue());
		txt_1->Clear();
		});

	publish->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& evt) ->void {
		if (!LIST_FUNC::has_selection(list)) { return; };
		vim_editor->AppendText((LIST_FUNC::get_str(list)) + wxString("\n"));
		});

	auto attr = vim_editor->GetBasicStyle();
	attr.SetTextColour(wxColour(255, 255, 255));
	vim_editor->SetBasicStyle(attr);

	vim_editor->set_vim_editor([this](wxString str) { 
        if (str.Mid(0, 5) == ":mark") {
            //data->vim_editor->set_color_for_selection();
        }
        else if (str.Mid(0, 2) == ":w") {
            vim_editor->save();
        }

        else if (str.Mid(0, 3) == ":wa") {
            vim_editor->save();
        }		
        else if (str.Mid(0, 3) == ":wq") {
            vim_editor->save();
            Destroy();
        }
        else if (str.Mid(0, 7) == ":delete") {
            vim_editor->delete_segment(vim_editor->get_current_segment_pos());
        }
        else if (str.Mid(0, 3) == ":ma") {
            vim_editor->merge_segment(false, vim_editor->get_current_segment_pos());

        }
        else if (str.Mid(0, 3) == ":mb") {
            vim_editor->merge_segment(true, vim_editor->get_current_segment_pos());

        }
        else if (str.Mid(0, 4) == ":del") {
            vim_editor->delete_segment(vim_editor->get_current_segment_pos());
        }
        else if (str.Mid(0, 2) == ":q") {
            Destroy();
        }
        else if(str.Mid(0, 3) == ":ps"){
            wxMessageBox(vim_editor->get_segment_text(vim_editor->get_current_segment_pos()));
        }
        else if(str.Mid(0, 5) == ":endl"){
            wxMessageBox(std::to_string(vim_editor->calc_trailing_endlines(vim_editor->get_current_segment_pos())));
        }
        else if(str.Mid(0, 2) == ":s"){
            vim_editor->open_tags(str.Mid(2, str.size()));
        }
        else if (str.Mid(0, 3) == ":nt") {
            vim_editor->new_segment(str.Mid(3, str.size()));
        }
        else if(str.Mid(0, 2) == ":t") { // tag
            vim_editor->add_tag(str.Mid(2, str.size()), vim_editor->get_current_segment_pos());
        }
        else if(str.Mid(0, 3) == ":rt") { // remove tag
            vim_editor->remove_tag(str.Mid(3, str.size()), vim_editor->get_current_segment_pos());
        }
    }, vim_command_line, vim_command_history);


	vim_font_resize = [this](int change = 0) {
		auto font = vim_editor->GetFont();
		auto default_font = vim_editor->default_style.GetFont();

		if (change == 1) {
			font = font.Larger();
			default_font = vim_editor->default_style.GetFont().Larger();
		}
		else {
			font = font.Smaller();
			default_font = vim_editor->default_style.GetFont().Smaller();
		}
		vim_editor->SetFont(font);
		vim_editor->default_style.SetFont(default_font);
	};

 	file->Append(FILECTRL::SAVE, "SAVE");
	nav->Append(VIM_KEYS::L, "LEFT");
	nav->Append(VIM_KEYS::H, "RIGHT");
	nav->Append(VIM_KEYS::W, "NAV");
	nav->Append(VIM_KEYS::W_CANCEL, "ECS");
	nav->Append(VIM_KEYS::RESIZE_MINUS, "MIN");
	nav->Append(VIM_KEYS::RESIZE_PLUS, "PLUS");
	nav->Append(VIM_KEYS::DEFAULT_STYLE, "DEFAULT");


	menu->Append(nav, "navigation");
	menu->Append(file, "file");

	nav->Bind(wxEVT_COMMAND_MENU_SELECTED, &C_Main::menu_callback, this);
	file->Bind(wxEVT_COMMAND_MENU_SELECTED, &C_Main::menu_callback, this);

	SetMenuBar(menu);

	clear_accell_table();

	auto x = wxRect(wxPoint(0,0),wxSize(800, 600));
	SetSize(x);
	auto w_size = GetSize();
	resize_tree = window_test(w_size, add, publish, txt_1, list, vim_editor, vim_command_line.get(), vim_command_history.get()); 
	Window::print_window_helper(resize_tree,"C:\\Users\\meme_\\Desktop\\meme.txt");

	Refresh();
	vim_editor->SetFocus();
}

C_Main::~C_Main(){
    delete add;
    delete publish;
    delete vim_editor;
    delete list;
    delete txt_1;
    Window::delete_window_tree(resize_tree);
}

// works for one but not vim input
void C_Main::menu_callback(wxCommandEvent& evt) {
	switch(evt.GetId()){
		case VIM_KEYS::H: {
			txt_1->SetFocus();
			clear_accell_table();
			break;
		}
		case VIM_KEYS::L: {
			//data->txt_2_list_output->Raise();
			vim_editor->SetFocus();
			clear_accell_table();
			break;
		}
		case VIM_KEYS::W: {
			update_accell_table();
			break;
		}
		case VIM_KEYS::W_CANCEL: {
			clear_accell_table();
			break;
		}
		case FILECTRL::SAVE: {
			wxMessageBox("save");
			if (vim_editor->get_v_mode() == VIMMODES::v_mode_navigation) {
			}
			save();
			break;
		}
		case VIM_KEYS::RESIZE_PLUS: {
			if(vim_font_resize)
			vim_font_resize(1);
			break;
		}
		case VIM_KEYS::RESIZE_MINUS: {
			if(vim_font_resize)
			vim_font_resize(-1);
			break;
		}
		case VIM_KEYS::DEFAULT_STYLE: {
			vim_editor->set_default_style();
			break;
		}
	}
}

void C_Main::save() {
	wxString str = vim_editor->GetValue();
}

void C_Main::resize(wxSizeEvent& evt) {
	if (resize_tree == nullptr) return;
	auto [x, y] = evt.GetSize();
	evt.Skip();

    resize_tree->resize({ x-16, y-59, 0, 0 });

	Refresh();
}

Window::Window(bool is_horizontal, wxWindow* frame)
	: obj(frame), is_horizontal(is_horizontal)
{}

Window* Window::add(bool is_horizontal, wxWindow* frame, const std::string& debug) {
	assert(obj == nullptr);

    WindowSizeData size;

    auto window = mem.add(new Window(is_horizontal, frame));
    window->debug = debug;

    return window;

}

int Window::resize(WindowSizeData p_size) {
    window_size = p_size;
            
    auto border_window = [this](WindowSizeData size){
        auto win_size = size;
        win_size.width  -= this->border_size.width * 2;
        win_size.height -= this->border_size.height * 2;
        win_size.posx += (this->border_size.width); 
        win_size.posy += (this->border_size.height);
        return win_size;
    }(p_size);


    if(on_resize_callback)
        on_resize_callback(border_window.width, border_window.height);

    Sizer args(&border_window, &mem, is_horizontal);
    args.calculate(); 

    if (obj != nullptr) {
        obj->SetSize(wxSize(border_window.width, border_window.height));
        obj->SetMinSize(wxSize(border_window.width, border_window.height));
        obj->SetPosition(wxPoint(border_window.posx, border_window.posy));
        obj->SetBackgroundColour(wxColor(31, 31, 31));
        obj->SetForegroundColour(wxColor(255, 255, 255));
        return window_size.get_length(is_horizontal);
    }

    int offset = 0;
    int reserve = 0;

    for (int i = 0; i < mem.count; ++i) {
        p_size = border_window;
        set_children_size_offset(&p_size, offset, i, args);
        { 
            if (i != mem.count - 1) {
                if (mem.windows[i]->border_size.get_length(is_horizontal) == mem.windows[i + 1]->border_size.get_length(is_horizontal)) {
                    const auto change_offset = mem.windows[i]->border_size.get_length(is_horizontal);
                    offset -= change_offset;
                    reserve += change_offset;
                }
            }
            else{
                p_size.get_length(is_horizontal) += reserve;
            }
        }
        mem.windows[i]->resize(p_size);
        offset += mem.windows[i]->window_size.get_length(is_horizontal);
    }
    return window_size.get_length(is_horizontal);
}

void Window::set_children_size_offset(WindowSizeData* arr, int offset, int it, const Sizer& args) {
    auto[x, y] = args.get_space(it);
    arr->width  = x;
    arr->height = y;
    arr->get_offset(is_horizontal) += offset;
}

bool Window::size_args(int num,...){
	assert(mem.count == 0 && mem.alloc_count == 0 && obj == nullptr);
	mem.allocate(num);
    int count = 0;
    va_list args;
    va_start(args, num);
    for(int i = 0; i < num; ++i){
        mem.sizes[i] = va_arg(args, int);
        count += mem.sizes[i];
    }
    va_end(args);
    

    if(count > 100){
        for(int i = 0; i < num; ++i){
            mem.sizes[i] = -1;
        }
    }

    return 1;
}

// WINDOW::SIZER

std::pair<int, int> Window::Sizer::get_space(int x)const {
    //if (x >= get_count()) { return -1; }
    auto set_output = [this](int length)->std::pair<int, int> {
        if (this->is_horizontal)
                return std::make_pair(length, this->window_size->get_height());
        else 
                return std::make_pair(this->window_size->get_width(), length);
    };

    if (is_last_norm(x)) {
        int norm_size = get_norm_size(); 
        int remainder = get_remainder_space(); 
        int result = norm_size + remainder;
        return set_output(result);
    }
    if (is_norm(x)) { return set_output(get_norm_size()); }

    return set_output(window_size->get_length(is_horizontal) * get_percent(x));
}


void Window::Sizer::calculate() {
    norm_count = 0;
    specified_size = 0;
    last_norm = -1;
    for(int i = 0; i < memory->get_count(); ++i){
        if(memory->get_size(i) != -1){ 
            specified_size += window_size->get_length(is_horizontal) * get_percent(i);
        }
        else{
            last_norm = i;
            ++norm_count;
        }
    }
}



// window memes
void Window::delete_window_tree(Window* window) {
	if (window == nullptr)return;
	for (int i = 0; i < window->mem.count; ++i) {
		delete_window_tree(window->mem.windows[i]);
	}
	delete window;
}

void Window::print_window_tree_to_file(Window* window, FILE* file, int depth) {
	std::string output;
	for (int i = 0; i < depth; ++i) {
		output.append("-");
	}
	output.append(window->debug);	
	output.append(", window_size(");
	output.append(std::to_string(window->window_size.width));
	output.append(", ");
	output.append(std::to_string(window->window_size.height)); 
	output.append(", ");
	output.append(std::to_string(window->window_size.posx));
	output.append(", ");
	output.append(std::to_string(window->window_size.posy));
	output.append(")\n");


	fputs(output.c_str(), file);
	for (int i = 0; i < window->mem.count; ++i) {
		print_window_tree_to_file(window->mem.windows[i], file, depth + 1);
	}
}

void Window::print_window_helper(Window* window, const std::string& file_name_loc) {
	FILE* file = fopen(file_name_loc.c_str(), "w");
	print_window_tree_to_file(window, file);
	fclose(file);
}

Window* window_test(wxSize size, wxWindow* add, wxWindow* publish, wxWindow* text, wxWindow* list, wxRichTextCtrl* vim, wxWindow* vim_command, wxWindow* vim_history){
    auto main = new Window(1, nullptr);
	main->size_args(2, 20, 80);

	auto left = main->add(false, nullptr, "_left");
	left->size_args(2, 10, 10);
	left->border_size = { 20,20,0,0 };

	auto click = left->add(true, nullptr, "click");
    auto _add = click->add(true, add, "add"); 
    click->add(1, publish, "publish"); 
    left->add(1, text, "text");
    left->add(1, list, "list"); 

    auto right = main->add(false, nullptr, "right");
	right->border_size = { 20,20,0,0 }; 

	auto vim_memes = right->add(0, nullptr, "vim_memes");
	vim_memes->size_args(3, 70, 10, 20);

	auto vim_editor_meme = vim_memes->add(true, vim, "vim");
	vim_editor_meme->on_resize_callback = [vim](int width, int height) { vim->set_size(width, height); };
	vim_memes->add(1, vim_command, "vim_command");
	auto logging = vim_memes->add(false, vim_history, "vim_history");

	main->resize({ 800, 600, 0, 0 });

	return main;
}
