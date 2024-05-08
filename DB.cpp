#include "DB.h"
#include "sqlite3.h"
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#include <algorithm>
#include <set>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <cassert>

static std::string tags_select_str(int entry_ref)                                         { return stringbuilder() << "SELECT Id, Tag FROM Tags WHERE EntryRef = " << entry_ref + ";"; }
static std::string tags_add_str(int entry_ref, const std::string& tag)                    {	return stringbuilder() << "INSERT INTO Tags(Tag, EntryRef) VALUES('" << tag << "' , '" << entry_ref << "');"; }
static std::string tags_delete_str(int entry_ref)                                         {	return stringbuilder() << "DELETE FROM Tags WHERE EntryRef = " 	<< entry_ref << ";"; }
static std::string entries_text_update_str(int entry_id, const std::string& text)         {	return stringbuilder() << "UPDATE Entries SET Text = '" << text << "' WHERE Id = " << entry_id << ";"; }
static std::string entries_select_str(int entry_id)                                       { return stringbuilder() << "SELECT Date, Text FROM Entries WHERE Id = " << entry_id << ";"; }
static std::string entries_add_str(const std::string& date, const std::string& text)      {	return stringbuilder() << "INSERT INTO Entries(Date, Text) VALUES('" << date << "' , '" << text << "');"; }
static std::string entries_delete_str(int id)                                             {	return stringbuilder() << "DELETE FROM Entries WHERE Id = " 	<< id << ";"; }

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

template<template<class>class S, typename T>
static S<T> sql_return_value(sqlite3* db, const std::string& str, T(func(sqlite3_stmt*))) {
    S<T> s;
    sqlite3_stmt* stmt;
    const char* select = str.c_str();
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    if (!ret) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if constexpr(std::is_same<S<T>, std::vector<T>>::value)
                s.emplace_back(func(stmt));
            else 
                s.emplace(func(stmt));
        }
    }
    sqlite3_finalize(stmt);

    return s;
}

template<template<class, class>class S, typename T>
static S<int, T> sql_return_value(sqlite3* db, const std::string& str, std::pair<int, T>(func(sqlite3_stmt*))) {
    S<int, T> s;
    sqlite3_stmt* stmt;
    const char* select = str.c_str();
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    if (!ret) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            s.insert(func(stmt));
        }
    }
    sqlite3_finalize(stmt);

    return s;
}

template<template<class, class>class S, typename T>
static S<int, std::vector<T>> sql_return_value_vector(sqlite3* db, const std::string& str, std::pair<int, T>(func(sqlite3_stmt*))) {
    static_assert(!std::is_same<S<int, std::vector<T>>, std::multimap<int, std::vector<T>>>::value);
    S<int, std::vector<T>> s;
    sqlite3_stmt* stmt;
    const char* select = str.c_str();
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    if (!ret) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
			auto [x, y] = func(stmt);
			s[x].push_back(y);
        }
    }
    sqlite3_finalize(stmt);
    return s;
}

template<typename T>
static std::pair<bool, T> sql_return_value_single(sqlite3* db, const std::string& str, T(func(sqlite3_stmt*))) {
    bool return_status = false;
    const char* select = str.c_str();
    T out;
    sqlite3_stmt* stmt;
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    if (!ret) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            out = func(stmt);
            return_status = true;
        }
    }
    sqlite3_finalize(stmt);

    return std::make_pair(return_status, out);
}



static std::string tags_select_entryref_tag_str(const std::vector<std::string>& tags){
    if(tags.size() == 0) return "";
    stringbuilder out;
    out << "SELECT EntryRef, Tag FROM Tags WHERE Tag = ";
    for(int i = 0; i < tags.size(); ++i){
        out << "'";
        out << tags[i];
        out << "'";

        if(i != tags.size() - 1){
            out << " OR Tag = ";
        }
    }
    out << ";";
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
}

int DB::new_entry_to_database(const std::string& text, const std::vector<std::string>& tags) {
    if(!is_open())return -1;
    
	auto date = current_date().to_str();
    sql(entries_add_str(date, text));
    auto val = sql_return_value_single<int>(db, "SELECT LAST_INSERT_ROWID();", [](sqlite3_stmt* stmt){
        return sqlite3_column_int(stmt, 0);
    });
    if(val.first == false){return -1;}

    for(const auto& x : tags){
        sql(tags_add_str(val.second, x));
    }

    return val.second;
}


int DB::save_entry(int entry, const std::string& text, const std::vector<std::string>& tags){
    if(entry < 0){
        entry = new_entry_to_database(text, tags);
    }
    else{
        sql(entries_text_update_str(entry, text));
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

    std::vector<Entry> out;
    
	auto sql_tags = sql_return_value_vector<std::unordered_map, std::string>(db, tags_select_entryref_tag_str(_tags_list), [](sqlite3_stmt* stmt)->std::pair<int, std::string>{
			int entryref = sqlite3_column_int(stmt, 0);
			auto tag = to_str(sqlite3_column_text(stmt, 1));
			return std::make_pair(entryref, tag);
	});

	for (auto x = sql_tags.begin(); x != sql_tags.end(); ++x) {
        auto val = sql_return_value_single<std::pair<std::string, std::string>>(db, entries_select_str(x->first), [](sqlite3_stmt* stmt) {
            auto date = to_str(sqlite3_column_text(stmt, 0));
            auto text = to_str(sqlite3_column_text(stmt, 1));
            return std::make_pair(date, text);
        });
        if(val.first == true){  
            out.emplace_back( Entry{ x->first, std::move(val.second.first), std::move(val.second.second), std::move(x->second)});
        }
    }

    return out;
}


std::vector<Entry> DB::get_all_entries() {
    if(!is_open()) return std::vector<Entry>();
    const char* select = "SELECT Id, Date, Text FROM Entries;";
    return sql_return_value<std::vector, Entry>(db, select, [](sqlite3_stmt* stmt) {
        int id = sqlite3_column_int(stmt, 0);
        auto date = to_str(sqlite3_column_text(stmt, 1));
        auto text = to_str(sqlite3_column_text(stmt, 2));
        return Entry{id, date, text};
    });
}

std::vector<std::string> DB::get_tags(int id) {
    if(!is_open()) return std::vector<std::string>();
    std::string select = stringbuilder() << "SELECT Tag FROM Tags WHERE EntryRef = " << id << ";";
    return sql_return_value<std::vector, std::string>(db, select, [](sqlite3_stmt* stmt) {
        return std::string{to_str(sqlite3_column_text(stmt, 0))};
    });
}

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

    if(!create_tables()) return false;

    return true;
}

int DB::get_entry_count(){
    std::string select = "SELECT COUNT(*) FROM Entries;";
    return sql_return_value_single<int>(db, select, [](sqlite3_stmt* stmt){
        return  sqlite3_column_int(stmt, 0);
    }).second;
}

int DB::get_tag_count(){
    std::string select = "SELECT COUNT(*) FROM Tags;";
    return sql_return_value_single<int>(db, select, [](sqlite3_stmt* stmt){
        return  sqlite3_column_int(stmt, 0);
    }).second;
}

bool DB::delete_entry(int entry_id){
    if(entry_id < 1) return false;
    deletion.emplace_back(entry_id);
    return 1;
}


bool DB::delete_entries_to_db(){
    for(auto x : deletion){
        std::string del_entry = stringbuilder() << "DELETE FROM Entries WHERE Id = " << x << ";";
        std::string del_tags =  stringbuilder() << "DELETE FROM Tags WHERE EntryRef = " << x << ";";
        sql(del_entry);
        sql(del_tags);
    }
    return true;
}

DB::DB(const char* file_loc_name){
    open_db(file_loc_name);
}


DB::DB(DB&& other)noexcept:db(std::exchange(other.db, nullptr)){entry_id_assign = other.entry_id_assign;}

DB& DB::operator=(DB&& other)noexcept{
    db = std::exchange(other.db, nullptr);
    entry_id_assign = other.entry_id_assign;
    return *this;
}

DB::~DB() {
    close_db();
}


