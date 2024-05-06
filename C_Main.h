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
		int get_width()const { return width; }
		int get_height()const { return height; }
		int get_posx()const { return posx; }
		int get_posy()const { return posy; }
		WindowSizeData operator + (const WindowSizeData& other) {
			WindowSizeData out = *this;
			out.width += other.width;
			out.height += other.height;
			out.posx += other.posx;
			out.posy += other.posy;
			return out;
		}
	}window_size, border_size;
	struct Memory { 
		int count{0};
		int alloc_count{0};
		Window** windows{nullptr};
        int* sizes{nullptr};

		const int get_size(int x)const { return sizes[x]; }
		const int get_count()const { return count; }

		bool empty() { return count > 0; }
		void allocate(int no_new_items = 1) {
			if (alloc_count == 0) alloc_count = 8 + no_new_items;

			{
				bool newly_allocated = false;
				if (windows == nullptr) {
					windows = new Window * [alloc_count];
					newly_allocated = true;
				}
				if (sizes == nullptr) {
					sizes = new int[alloc_count];
					memset(sizes, -1, alloc_count * sizeof(sizes[0]));
					newly_allocated = true;
				}
				if (newly_allocated) return;
			}

			if (count < alloc_count) return;

			alloc_count *= 2;
            alloc_count += no_new_items;

			auto* new_win = new Window* [alloc_count];
			memcpy(new_win, windows, (count) * sizeof(windows[0]));

			auto* new_sizes = new int[alloc_count];
			memcpy(new_sizes, sizes, count * sizeof(sizes[0]));
			memset(new_sizes + count, -1, (alloc_count - count) * sizeof(sizes[0]));

			delete[] windows;
            delete[] sizes;
			windows = new_win;
            sizes = new_sizes;
		}
		Window* add(Window* window, unsigned int extra_alloc = 1) {
			if(count + extra_alloc > alloc_count)
				allocate(extra_alloc);
			return windows[count++] = window;
		}
		Window* last() {
			if (count < 1) return nullptr;
			return windows[count - 1];
		}
		~Memory() {
            if(windows != nullptr)
                delete[] windows;
            if(sizes != nullptr)
                delete[] sizes;
		}
	}mem;

    bool size_args(int num,...);
	class Sizer {
		const WindowSizeData*const window_size;
		const Memory*const memory;
		const int is_horizontal;

		int norm_count{0};
		int specified_size{0};
		int last_norm{-1};
		bool use_window_size{1};
		bool is_last(int x)const { return memory->get_count() - 1 == x; }
		bool is_norm(int x)const { return (memory->get_size(x) == -1); }
		bool is_last_norm(int x)const { return x == last_norm; }
		double get_percent(int x)const		{ return double(memory->get_size(x)) / 100.0; } 

		std::pair<int, int> get_offset_position_to_center(int width, int height);
	public:
		int get_count()const { return memory->get_count(); }
		int get_area()const {
			if (is_horizontal)
				return window_size->get_width();
			return window_size->get_height();
		}
		bool has_any_specified_size()const	{ return specified_size > 0; } 
		int get_norm_count()const			{ return norm_count; } 
		int get_specified_space()const		{ return specified_size; } 
		int get_specified_count()const		{ return memory->get_count() - norm_count; }
		int get_unspecified_space()const	{ return get_area() - get_specified_space(); }
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


    void set_offset_values(WindowSizeData* arr, int offset, int it, const Sizer& args);
	int resize(WindowSizeData p_size);
	Window* add(bool is_horizontal, wxWindow* frame = nullptr, const std::string& debug = "", int extra_alloc = 1);
	Window(const Window& other) = delete;
	Window operator=(const Window& other) = delete;
	~Window() {}
	explicit Window(bool is_horizontal, wxWindow* frame = nullptr, unsigned int count = 8);
};


void delete_window_tree(Window* window);
void print_window_tree_to_file(Window* window, FILE* file, int depth = 0);
void print_window_helper(Window* window, const std::string& file_name_loc);
Window* window_test(wxSize size, wxWindow* add, wxWindow* publish, wxWindow* text, wxWindow* list, wxRichTextCtrl* vim, wxWindow* vim_command, wxWindow* vim_history);

class C_Main : public wxFrame
{
public:
	wxMenuBar* menu;
	wxMenu* nav;
	wxMenu* file;
	Window* resize_tree{nullptr};



	wxButton* add = nullptr;
	wxButton* publish = nullptr;
	wxObject* focus = nullptr;

	wxRichTextCtrl* vim_editor = nullptr;
	wxListBox* list = nullptr;
	wxTextCtrl* txt_1 = nullptr;
	wxTextCtrl* vim_input = nullptr;

	wxSharedPtr<wxTextCtrl> vim_command_line;
	wxSharedPtr<wxTextCtrl> vim_command_history;
    wxNotebook* log;
	

	void save();
	void update_accell_table();
	void clear_accell_table();
	void resize(wxSizeEvent& evt);

	std::function<void(int)> vim_font_resize;
	void menu_callback(wxCommandEvent& evt);
	bool mode{ 0 };
	C_Main();
	DECLARE_EVENT_TABLE()
};

#endif
