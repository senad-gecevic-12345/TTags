#include "DB.h"
#include "sqlite3.h"
#include <time.h>
#include <algorithm>
#include <set>
#include <functional>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <array>
#include <cassert>

#define __STDC_WANT_LIB_EXT1__ 1

struct SQL_RETURN {
	enum {
		OK = 0
	};
};

static int sql_bind_spec(sqlite3_stmt* stmt, int num, int count = 1){
     return sqlite3_bind_int(stmt, count, num);
}

static int sql_bind_spec(sqlite3_stmt* stmt, const std::string& str, int count = 1){
    return sqlite3_bind_text(stmt, count, str.c_str(), -1, SQLITE_STATIC);
}

template<typename... Args>
static bool sql_bind(sqlite3_stmt* stmt, const Args&... args){
    int count{0};
	return ((SQL_RETURN::OK == sql_bind_spec(stmt, args, ++count)) == ...);
}

template<typename T>
static bool sql_bind(sqlite3_stmt* stmt, const std::vector<T>& vec){
    static_assert(std::is_same<T, int>::value || std::is_same<T, std::string>::value);
    int count{0};
    for(const auto& x : vec){
		if (bool val = sql_bind_spec(stmt, x, ++count); val != SQL_RETURN::OK) return false;
    }
    return true;
}

static std::string char_double(const std::string& str, int num, ...){
	assert(num <= 8);
	std::array<char, 8> list;
	va_list args;
	va_start(args, num);
	for (int i = 0; i < num; ++i) {
		list[i] = va_arg(args, char);
	}
	va_end(args);
	std::string out; 
	out.reserve(str.size()); 
	for (auto x : str) { 
		for (int y = 0; y < num; ++y) {
			if (x == list[y]) {
				out += x;
				break;
			} 
		}
		out += x;
	}
	return out;
}

static std::string tags_add_str(int entry_ref, const std::string& tag)                    {	return stringbuilder() << "INSERT INTO Tags(Tag, EntryRef) VALUES('" << tag << "' , '" << entry_ref << "');"; }
static std::string tags_delete_str(int entry_ref)                                         {	return stringbuilder() << "DELETE FROM Tags WHERE EntryRef = " 	<< entry_ref << ";"; }
static std::string entries_text_update_str(int entry_id, const std::string& text)         {	return stringbuilder() << "UPDATE Entries SET Text = '" << text << "' WHERE Id = " << entry_id << ";"; }
static std::string entries_add_str(const std::string& date, const std::string& text)      {	return stringbuilder() << "INSERT INTO Entries(Date, Text) VALUES('" << date << "' , '" << text << "');"; }

Date current_date() {
    time_t a = time(nullptr);
    tm x;
    localtime_s(&x, &a);
    return Date{x.tm_year + 1900, x.tm_mon + 1, x.tm_mday};
}

static std::string to_str(const unsigned char* ptr) {
    int i{ 0 };
    std::string str;
    while (ptr[i] != '\0') {
        str += ptr[i];
        ++i;
    }
    return str;
}

template<typename... Args>
static std::string str_gen(Args... args){
    stringbuilder s;
    (s << ... << args);
    return s;
}

// for the read entries, dont really want a return value. want just to write out the values to the display. therefore need no return value. can just not use it. but think should have another named for each or something
// write the bind and non bind to call the other function, and then call it with nullptr or not
template<template<class...>class S, class... T>
static S<T...> sql_return_value(sqlite3* db, const std::string& str, std::function<void(sqlite3_stmt*, S<T...>*)> add, std::function<bool(sqlite3_stmt*)> bind = nullptr) {
    S<T...> s;
    sqlite3_stmt* stmt;
    const char* select = str.c_str();
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
	bool bind_ok{true};
	if (bind != nullptr)
		bind_ok = bind(stmt);
    if (!ret && bind_ok) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            add(stmt, &s);
        }
    }
    sqlite3_finalize(stmt);

    return s;
}

template<typename T>
static std::pair<bool, T> sql_return_value_single(sqlite3* db, const std::string& str, T(func(sqlite3_stmt*)), std::function<bool(sqlite3_stmt*)> bind = nullptr) { 
    bool return_status{false};
    const char* select = str.c_str();
    T out;
    sqlite3_stmt* stmt;
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    bool bind_ok = true;
    if(bind != nullptr)
        bind_ok = bind(stmt);
    if (!ret && bind_ok) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            out = func(stmt);
            return_status = true; 
        }
    }
    sqlite3_finalize(stmt);

    return std::make_pair(return_status, out);
}


std::string sql_parameters(int count, std::string sep){
    stringbuilder out;
    for(int i = 0; i < count; ++i){
        out << "?" << i + 1;
        if(i != count - 1){
            out << sep;
        }
    }
    return out;
}

bool DB::create_tables(){
    int a = sql("CREATE TABLE IF NOT EXISTS Entries(Id INTEGER PRIMARY KEY AUTOINCREMENT, Date TEXT, Text TEXT);");
    int b = sql("CREATE TABLE IF NOT EXISTS Tags(Id INTEGER PRIMARY KEY, Tag TEXT, EntryRef INTEGER, FOREIGN KEY(EntryRef) REFERENCES Entries(Id));");
    return a && b;
}

bool DB::open_db(const char* file_loc_name) {
    int status = sqlite3_open(file_loc_name, &db);
    if(status != SQLITE_OK) { db = nullptr; return false; }
    if(!create_tables())     { db = nullptr; return false; }
    return true;
 } 

void DB::close_db() {
    if(db != nullptr)
        sqlite3_close(db);
    db = nullptr;
}

int DB::new_entry_to_database(const std::string& text, const std::vector<std::string>& tags) {
    if(!is_open())return -1;
    
	auto date = current_date().to_str();
    if(!sql(entries_add_str(date, text))) return -1;
    auto val = sql_return_value_single<int>(db, "SELECT LAST_INSERT_ROWID();", [](sqlite3_stmt* stmt){
        return sqlite3_column_int(stmt, 0);
    }, nullptr);
    if(val.first == false){return -1;}

    for(const auto& x : tags){
        sql(tags_add_str(val.second, x));
    }

    return val.second;
}

int DB::save_entry(int entry, const std::string& text, const std::vector<std::string>& tags){
	auto s = char_double(text, 1, '\'');

    if(entry < 0){
        entry = new_entry_to_database(s, tags);
    }
    else{
        sql(entries_text_update_str(entry, s));
        sql(tags_delete_str(entry));
		for(const auto& x : tags)
			sql(tags_add_str(entry, x));
    }
	return entry;
}

int DB::new_entry(){
    return entry_id_assign--;
}


std::vector<Entry> DB::get_entries_with_tags(const std::vector<std::string>& _tags_list) {
    if(!is_open())return std::vector<Entry>();

    std::string select_entryref_tag_query  = stringbuilder() << 
		"SELECT EntryRef, Tag FROM Tags WHERE Tag IN (" << sql_parameters(_tags_list.size()) << ")";
    auto tags = sql_return_value<std::unordered_map, int, std::vector<std::string>>(db, select_entryref_tag_query,
        [](sqlite3_stmt* stmt, std::unordered_map<int, std::vector<std::string>>* m){
            (*m)[sqlite3_column_int(stmt, 0)].emplace_back(to_str(sqlite3_column_text(stmt, 1)));
        },
        [&_tags_list](sqlite3_stmt* stmt){
            return sql_bind(stmt, _tags_list);
        }
    );

    std::string select_entries_query  = stringbuilder() <<
		"SELECT Entries.Id, Entries.Date, Entries.Text FROM Tags " <<
		"INNER JOIN Entries ON (Tags.EntryRef = Entries.Id) " <<
		"WHERE Tag IN (" << sql_parameters(_tags_list.size()) << ")";
    return sql_return_value<std::vector, Entry>(db, select_entries_query,
        [&tags](sqlite3_stmt* stmt, std::vector<Entry>* vec){
            int id = sqlite3_column_int(stmt, 0);
            vec->emplace_back(
                Entry{
                    id,
                    to_str(sqlite3_column_text(stmt, 1)),
                    to_str(sqlite3_column_text(stmt, 2)),
                    std::move(tags[id])
                    }
            );
        },
        [&_tags_list](sqlite3_stmt* stmt){
            return sql_bind(stmt, _tags_list);
        }
    );

}


std::vector<Entry> DB::get_all_entries() {
    if(!is_open()) return std::vector<Entry>();
    const char* select = "SELECT Id, Date, Text FROM Entries;";
    return sql_return_value<std::vector, Entry>(db, select, [](sqlite3_stmt* stmt, std::vector<Entry>* vec) {
        vec->emplace_back(Entry{
            sqlite3_column_int(stmt, 0), 
            to_str(sqlite3_column_text(stmt, 1)),
            to_str(sqlite3_column_text(stmt, 2))
        });
    }, nullptr);
}

std::vector<std::string> DB::get_tags(int id) {
    if(!is_open()) return std::vector<std::string>();
    auto select = "SELECT Tag FROM Tags WHERE EntryRef = ?1";
    return sql_return_value<std::vector, std::string>(db, select, 
        [](sqlite3_stmt* stmt, std::vector<std::string>* vec) {
            vec->emplace_back(to_str(sqlite3_column_text(stmt, 0)));
        }, 
        [id](sqlite3_stmt* stmt){
            return sql_bind(stmt, id);
        }
    );
}

// TODO: bad return values, for the single too. sqlite returns 0 for ok, this can be missunderstood
bool DB::sql(const std::string& str) {
    char* err_msg{ nullptr };
    auto status = sqlite3_exec(db, str.c_str(), 0, 0, &err_msg);
    if (status != SQLITE_OK) { 
        if (err_msg != nullptr)
            sqlite3_free(err_msg);
        return false;
    }
    return true;
}


bool DB::clear_table() {
    auto tags = 	"DROP TABLE Tags;";
    auto entries = 	"DROP TABLE Entries;";

    sql(tags);
    sql(entries);

	return create_tables();
}

int DB::get_entry_count(){
    std::string select = "SELECT COUNT(*) FROM Entries;";
    return sql_return_value_single<int>(db, select, [](sqlite3_stmt* stmt){
        return  sqlite3_column_int(stmt, 0);
    }, nullptr).second;
}

int DB::get_tag_count(){
    std::string select = "SELECT COUNT(*) FROM Tags;";
    return sql_return_value_single<int>(db, select, [](sqlite3_stmt* stmt){
        return  sqlite3_column_int(stmt, 0);
    }, nullptr).second;
}

bool DB::delete_entry(int entry_id){
    if(entry_id < 1) return false;
    deletion.emplace_back(entry_id);
    return 1;
}


bool DB::delete_entries_to_db(){
    for(auto x : deletion){
        std::string del_entry = stringbuilder() << "DELETE FROM Entries WHERE Id = "    << x << ";";
        std::string del_tags =  stringbuilder() << "DELETE FROM Tags WHERE EntryRef = " << x << ";";
        sql(del_entry);
        sql(del_tags);
    }
    return true;
}

DB::DB(const char* file_loc_name){
    open_db(file_loc_name);
}


DB::DB(DB&& other)noexcept:
    db(std::exchange(other.db, nullptr)),
    deletion(std::move(other.deletion)),
    entry_id_assign(std::move(other.entry_id_assign))
{}

DB& DB::operator=(DB&& other)noexcept{
    close_db();
    db = std::move(other.db);
    deletion = std::move(other.deletion);
    entry_id_assign = std::move(other.entry_id_assign);
    return *this;
}

DB::~DB() {
    close_db();
}


