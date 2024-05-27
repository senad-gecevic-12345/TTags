#ifndef CMAIN_
#define CMAIN_
#include "wx/wx.h"
#include <wx/listctrl.h>
#include <wx/notebook.h>

#include <wx/richtext/richtextctrl.h>
#include <array>

struct Window {
	struct WindowSizeData {
		int width{0};
		int height{0};
		int posx{0};
		int posy{0};
		int get_width()const    { return width; }
		int get_height()const   { return height; }
		int get_posx()const     { return posx; }
		int get_posy()const     { return posy; }
        int& get_length(bool is_horizontal){ return is_horizontal ? width : height; }
        int& get_offset(bool is_horizontal){ return is_horizontal ? posx : posy; }
        const int& get_length(bool is_horizontal)const{ return is_horizontal ? width : height; }
        const int& get_offset(bool is_horizontal)const{ return is_horizontal ? posx : posy; }
	}window_size, border_size;
	class Memory {
		int count{ 0 };
		int alloc_count{ 0 };
		Window** windows{ nullptr };
		int* sizes{ nullptr };
		void allocate(int no_new_items = 1) {
			{
				bool newly_allocated = false;
				if (windows == nullptr) {
					windows = new Window * [no_new_items];
					newly_allocated = true;
				}
				if (sizes == nullptr) {
					sizes = new int[no_new_items];
					memset(sizes, -1, no_new_items * sizeof(int));
					newly_allocated = true;
				}
				if (newly_allocated) {
					alloc_count = no_new_items;
					return;
				}
			}

			alloc_count += no_new_items;
			alloc_count *= 2;

			auto* new_win = new Window * [alloc_count];
			memcpy(new_win, windows, (count) * sizeof(Window*));

			auto* new_sizes = new int[alloc_count];
			memcpy(new_sizes, sizes, count * sizeof(int));
			memset(new_sizes + count, -1, (alloc_count - count) * sizeof(int));

			delete[] windows;
			delete[] sizes;
			windows = new_win;
			sizes = new_sizes;
		}
		const int get_size(int x)const { return sizes[x]; }
		const int get_count()const { return count; }

		Window* add(Window* window) {
			if(count + 1 > alloc_count)
				allocate();
			return windows[count++] = window;
		}
		Window* last() {
			if (count < 1) return nullptr;
			return windows[count - 1];
		}

	public:
		Memory(){}
		Memory(Memory& other) = delete;
		Memory& operator = (Memory& other) = delete;
		Memory(Memory&& other) = delete;
		Memory& operator = (Memory&& other) = delete;
		~Memory() {
            delete[] windows;
            delete[] sizes;
		}
		friend class Window;

	}mem;

    bool size_args(int num,...);
	class Sizer {
		const WindowSizeData* const window_size;
		const Memory* const memory;
		const int is_horizontal;

		int norm_count{0};
		int specified_size{0};
		int last_norm{-1};
		bool use_window_size{1};
		bool is_last(int x)const		{ return memory->get_count() - 1 == x; }
		bool is_norm(int x)const		{ return (memory->get_size(x) == -1); }
		bool is_last_norm(int x)const	{ return x == last_norm; }
		double get_percent(int x)const	{ return double(memory->get_size(x)) / 100.0; } 

	public:
		int get_count()const { return memory->get_count(); }
		bool has_any_specified_size()const	{ return specified_size > 0; } 
		int get_norm_count()const			{ return norm_count; } 
		int get_specified_space()const		{ return specified_size; } 
		int get_specified_count()const		{ return memory->get_count() - norm_count; }
		int get_unspecified_space()const	{ return window_size->get_length(is_horizontal) - get_specified_space(); }
		int get_norm_size()const			{ return get_unspecified_space() / get_norm_count(); } 
		int get_remainder_space()const { 
			int x = get_norm_size() * get_norm_count();
			int y = get_unspecified_space();
			return y - x;
		}

	public:
		void calculate();

        std::pair<int, int> get_space(int x)const;

		Sizer(const WindowSizeData* window_size, const Memory* memory, bool is_horizontal):
			window_size(window_size), memory(memory), is_horizontal(is_horizontal){};

	};

	std::string debug;
	bool is_horizontal{false};
    bool is_hidden{false};
	wxWindow* obj{nullptr};
	std::function<void(int, int)> on_resize_callback;

    void set_children_size_offset(WindowSizeData* arr, int offset, int it, const Sizer& args);
	int resize(WindowSizeData p_size);
	Window* add(bool is_horizontal, wxWindow* frame = nullptr, const std::string& debug = "");
	static void delete_window_tree(Window* window);


	Window(const Window&& other) = delete;
	Window operator=(const Window&& other) = delete;
	Window(const Window& other) = delete;
	Window operator=(const Window& other) = delete;
	~Window() {}
	explicit Window(bool is_horizontal, wxWindow* frame = nullptr);
};

Window* window_test(wxSize size, wxWindow* add, wxWindow* publish, wxWindow* text, wxWindow* list, wxRichTextCtrl* vim, wxWindow* vim_command, wxWindow* vim_history);

class C_Main;

class Command {
	wxString str;
	void(*cmd)(const wxString&, C_Main*);
public:

	Command(wxString str, 	void(*cmd)(const wxString&, C_Main*)=nullptr)
		:str(str), cmd(cmd) {}
	Command(const Command& other) { str = other.str; cmd = other.cmd; };
	Command(Command&& other)noexcept : str(std::move(other.str)), cmd(std::move(other.cmd)) {};
	Command& operator =(const Command& other) { str = other.str; cmd = other.cmd; return *this; };
	Command& operator =(Command&& other)noexcept { str = (std::move(other.str)); cmd = (std::move(other.cmd)); return *this; };
	~Command() {}
	bool exec(const wxString& param, C_Main* ptr)const;
	bool operator == (const Command& search)const {
		return str == search.str;
	}
	bool operator < (const Command& other)const {
		return std::string(str) < std::string(other.str);
	}
};


class C_Main : public wxFrame
{
	Window* resize_tree{nullptr};

	wxButton* add{nullptr};
	wxButton* publish{nullptr};
	wxRichTextCtrl* vim_editor{nullptr};
	wxListBox* list{nullptr};
	wxTextCtrl* txt_1{nullptr};

	wxSharedPtr<wxTextCtrl> vim_command_line;
	wxSharedPtr<wxTextCtrl> vim_command_history;

	void save();
	void update_accell_table();
	void clear_accell_table();
	void resize(wxSizeEvent& evt);

	std::function<void(int)> vim_font_resize;
	void menu_callback(wxCommandEvent& evt);
	bool mode{ 0 };

	std::set<Command> commands;


	void command_callback(const wxString& str);

public:
    C_Main operator=(const C_Main& other)=delete;
    C_Main(const C_Main& other)=delete;
    C_Main operator=(C_Main&& other)=delete;
    C_Main(C_Main&& other)=delete;
	C_Main();
	~C_Main();
	DECLARE_EVENT_TABLE()
};




#endif
