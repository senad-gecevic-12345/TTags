#pragma once

#include <string>
#include <vector>
#include <sstream>

struct sqlite3;

struct Date {
	int year, month, day;
	std::string to_str() { 
        auto y = std::to_string(year) + '-';
        auto m = (month < 10 ? "0" : "") + std::to_string(month) + '-';
        auto d = (day < 10 ? "0" : "") + std::to_string(day);
        return y + m + d;
    }
};

struct Entry {
	int id;
	std::string date;
	std::string text;
	std::vector<std::string> tags;
};

struct stringbuilder
{
    std::stringstream ss;
    template<typename T>
    stringbuilder& operator << (const T& data)
    {
        ss << data;
        return *this;
    }
    operator std::string() { return ss.str(); }
};

Date current_date();

class DB{
	sqlite3* db{nullptr};
	bool sql(const std::string& str);
	bool open_db(const char* file_loc_name);
	void close_db();
	bool create_tables();
	bool create_tables_temp();

    std::vector<int> deletion;
	int new_entry_to_database(const std::string& text, const std::vector<std::string>& tags);
    int entry_id_assign{-1};

public:

    int new_entry();
	bool clear_table();
	int get_entry_count();
	int get_tag_count();

	int save_entry(int entry, const std::string& text, const std::vector<std::string>& tags);
    bool delete_entry(int entry_id);
    bool delete_entries_to_db();

	// returns id
	std::vector<Entry> get_entries_with_tags(const std::vector<std::string>& _tags_list);
	std::vector<Entry> get_all_entries();
	std::vector<std::string> get_tags(int id);

	bool is_open(){return db != nullptr;};

    void clear_buffer(){deletion.clear();}

	explicit DB(const char* file_loc_name);
	DB(const DB& other) = delete;
	DB(DB&& other) noexcept;
	DB& operator=(const DB& other) = delete;
	DB& operator=(DB&& other) noexcept;
	~DB();
};

std::string sql_parameters(int count, std::string sep = ", ");