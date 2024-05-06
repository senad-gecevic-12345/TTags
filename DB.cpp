#include "DB.h"
#include "sqlite3.h"
#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>
#include <algorithm>
#include <set>
#include <sstream>
#include <unordered_map>
#include <cassert>

struct TagPair {
    std::string tag;
    int entry_ref;
    bool operator<(const TagPair& other)const {
        bool a = entry_ref < other.entry_ref;
        if (entry_ref == other.entry_ref) {
            return (bool)(strcmp(tag.c_str(), other.tag.c_str()) < 0);
        }
        return a;
    }
};

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

template<typename T>
static std::multiset<T> sql_return_value_multiset(sqlite3* db, const std::string& str, T(func(sqlite3_stmt*))) {
    std::multiset<T> s;
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

template<typename T>
static std::set<T> sql_return_value_set(sqlite3* db, const std::string& str, T(func(sqlite3_stmt*))) {
    std::set<T> s;
    sqlite3_stmt* stmt;
    const char* select = str.c_str();
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    if (!ret) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            auto buffer = func(stmt);
            s.insert(buffer);
        }
    }
    sqlite3_finalize(stmt);

    return s;
}

template<typename T>
static std::vector<T> sql_return_value_list(sqlite3* db, const std::string& str, T(func(sqlite3_stmt*))) {
    const char* select = str.c_str();
    std::vector<T> list;
    sqlite3_stmt* stmt;
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    if (!ret) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            list.emplace_back(func(stmt));
        }
    }
    sqlite3_finalize(stmt);
    return list;
}

template<typename T>
static std::unordered_map<int, std::vector<T>> sql_return_value_map(sqlite3* db, const std::string& str, std::pair<int, T>(func(sqlite3_stmt*))) {
    const char* select = str.c_str();
    std::unordered_map<int,std::vector<T>> list;
    sqlite3_stmt* stmt;
    auto ret = sqlite3_prepare_v2(db, select, -1, &stmt, 0);
    if (!ret) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            auto res = func(stmt);
            list[res.first].push_back(res.second);
        }
    }
    sqlite3_finalize(stmt);
    return list;
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


static std::string entry_add_str(const std::string& date, const std::string& text)    {	return stringbuilder() << "INSERT INTO Entries(Date, Text) VALUES('" << date << "' , '" << text << "');"; }
static std::string tags_add_str(int entry_ref, const std::string& tag)                {	return stringbuilder() << "INSERT INTO Tags(Tag, EntryRef) VALUES('" << tag << "' , '" << entry_ref << "');"; }
static std::string entry_text_update_str(int entry_id, const std::string& text)       {	return stringbuilder() << "UPDATE Entries SET Text = '" << text << "' WHERE Id = " << entry_id << ";"; }
static std::string tags_select_str(int entry_ref)                                     { return stringbuilder() << "SELECT Id, Tag FROM Tags WHERE EntryRef = " << entry_ref + ";"; }
static std::string tags_delete_str(int entry_ref)                                     {	return stringbuilder() << "DELETE FROM Tags WHERE EntryRef = " 	<< entry_ref << ";"; }
static std::string delete_entry_str(int id)                                           {	return stringbuilder() << "DELETE FROM Entries WHERE Id = " 	<< id << ";"; }
static std::string delete_tags_str(int id)                                            {	return stringbuilder() << "DELETE FROM Tags WHERE Id = " 		<< id << ";"; }

// can maybe write a join?
static std::string tags_get_str(const std::vector<std::string>& tags){
    if(tags.size() == 0) return "";
    stringbuilder out;
    out << "SELECT  Tag, EntryRef FROM Tags WHERE Tag = ";
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
static std::string tags_get_str_v2(const std::vector<std::string>& tags){
    if(tags.size() == 0) return "";
    stringbuilder out;
    out << "SELECT EntryRef FROM Tags WHERE Tag = ";
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

static std::string tags_get_str_v3(const std::vector<std::string>& tags){
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


static std::string tags_is_not_str(const std::vector<std::string>& tags){
    if(tags.size() == 0) return "";
    stringbuilder out;
    out << "SELECT  Tag, EntryRef FROM Tags WHERE Tag != ";
    for(int i = 0; i < tags.size(); ++i){
        out << "'";
        out << tags[i];
        out << "'";

        if(i != tags.size() - 1){
            out << " AND Tag != ";
        }
    }
    out << ";";
    return out;
}
// maybe use vector instead of set
static std::string tags_with_entryref_str(const std::vector<int>& tags){
    if(tags.size() == 0) return "";
    stringbuilder out;
    out << "SELECT  Tag, EntryRef FROM Tags WHERE EntryRef IN (";
    for(int i = 0; i < tags.size(); ++i){
        out << tags[i];
        if(i != tags.size() - 1){
            out << ", ";
        }
    }
    out << ");";
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
    sql(entry_add_str(date, text));
    auto val = sql_return_value_single<int>(db, "SELECT LAST_INSERT_ROWID();", [](sqlite3_stmt* stmt){
        return sqlite3_column_int(stmt, 0);
    });
    //if(val. < 1) assert(0);
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
        sql(entry_text_update_str(entry, text));
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
    
	auto sql_tags = sql_return_value_map<std::string>(db, tags_get_str_v3(_tags_list), [](sqlite3_stmt* stmt){
			int entryref = sqlite3_column_int(stmt, 0);
			auto tag = to_str(sqlite3_column_text(stmt, 1));
			return std::make_pair(entryref, tag);
	});

	for (auto x = sql_tags.begin(); x != sql_tags.end(); ++x) {
        std::string select = stringbuilder() << "SELECT Date, Text FROM Entries WHERE Id = " << x->first << ";";
        auto val = sql_return_value_single<std::pair<std::string, std::string>>(db, select, [](sqlite3_stmt* stmt){
            auto date = to_str(sqlite3_column_text(stmt, 0));
            auto text = to_str(sqlite3_column_text(stmt, 1));
            return std::make_pair(date, text);
        });
        if(val.first == true){  
            out.emplace_back(Entry{x->first, val.second.first, val.second.second, std::move(x->second)});
        }
    }

    return out;
}


std::vector<Entry> DB::get_all_entries() {
    if(!is_open()) return std::vector<Entry>();
    const char* select = "SELECT Id, Date, Text FROM Entries;";
    return sql_return_value_list<Entry>(db, select, [](sqlite3_stmt* stmt) {
        int id = sqlite3_column_int(stmt, 0);
        auto date = to_str(sqlite3_column_text(stmt, 1));
        auto text = to_str(sqlite3_column_text(stmt, 2));
        return Entry{id, date, text};
    });
}

std::vector<std::string> DB::get_tags(int id) {
    if(!is_open()) return std::vector<std::string>();
    std::string select = stringbuilder() << "SELECT Tag FROM Tags WHERE EntryRef = " << id << ";";
    return sql_return_value_list<std::string>(db, select, [](sqlite3_stmt* stmt) {
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

struct Tag{
    int entry_ref;
    std::string tag;
	bool operator=(const Tag& other) {
		return entry_ref == other.entry_ref;
	}
    bool operator<(const Tag& other)const{
        return entry_ref < other.entry_ref;
    }
};



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



/*
void DB::copy_over(){
    sql("DROP TABLE Tags;");
    sql("DROP TABLE Entries;");
    create_tables();
    auto entries = sql_return_value_list<std::tuple<int, std::string, std::string>>(db, "SELECT Id, Date, Text FROM EntriesTemp", [](sqlite3_stmt* stmt){
         return std::make_tuple(
            sqlite3_column_int(stmt, 0),
            to_str(sqlite3_column_text(stmt, 1)),
            to_str(sqlite3_column_text(stmt, 2)));
    });   

	// BUG: this dont compile
    auto tags = sql_return_value_multiset<Tag>(db, "SELECT EntryRef, Tag FROM TagsTemp", [](sqlite3_stmt* stmt){
         return Tag{ sqlite3_column_int(stmt, 0), to_str(sqlite3_column_text(stmt, 1))};
    });   

    for(auto& e : entries){
        auto [id, date, text] = e;
        // must do a find and then iterate from that returned iterator
		std::string xd = stringbuilder() << "INSERT INTO Entries(Date, Text) VALUES('" << date << "', '" << text << "')";
        sql(xd);
        auto new_ref = sql_return_value_single<int>(db, "SELECT LAST_INSERT_ROWID();", [](sqlite3_stmt* stmt){
            return sqlite3_column_int(stmt, 0);
        }).second;
		// only one of the thing is important for search
		for (auto it = tags.find(Tag{ id , ""}); it != tags.end() && it->entry_ref == id; ++it) {
            sql(stringbuilder() << "INSERT INTO Tags(Tag, EntryRef) VALUES('" << it->tag << "', " << new_ref << ")");
        }
    }
    sql("DROP TABLE TagsTemp;");
    sql("DROP TABLE EntriesTemp;");
}

void DB::db_migrate(){
    create_tables_temp();
    auto entries = sql_return_value_list<std::tuple<int, std::string, std::string>>(db, "SELECT Id, Date, Text FROM Entries", [](sqlite3_stmt* stmt){
         return std::make_tuple(
            sqlite3_column_int(stmt, 0),
            to_str(sqlite3_column_text(stmt, 1)),
            to_str(sqlite3_column_text(stmt, 2)));
    });   

    auto tags = sql_return_value_multiset<Tag>(db, "SELECT EntryRef, Tag FROM Tags", [](sqlite3_stmt* stmt){
         return Tag{ sqlite3_column_int(stmt, 0), to_str(sqlite3_column_text(stmt, 1))};
    });   
    for(auto& e : entries){
        auto [id, date, text] = e;
        // must do a find and then iterate from that returned iterator
		std::string xd = stringbuilder() << "INSERT INTO EntriesTemp(Date, Text) VALUES('" << date << "', '" << text << "')";
        sql(xd);
        auto new_ref = sql_return_value_single<int>(db, "SELECT LAST_INSERT_ROWID();", [](sqlite3_stmt* stmt){
            return sqlite3_column_int(stmt, 0);
        }).second;
		// only one of the thing is important for search
		for (auto it = tags.find(Tag{ id , ""}); it != tags.end() && it->entry_ref == id; ++it) {
            sql(stringbuilder() << "INSERT INTO TagsTemp(Tag, EntryRef) VALUES('" << it->tag << "', " << new_ref << ")");
        }
    }
}
*/

/*
void DB::date_fix(){
    // update thingy
    auto entries = sql_return_value_list<std::tuple<int, std::string, std::string>>(db, "SELECT Id, Date, Text FROM Entries", [](sqlite3_stmt* stmt){
         return std::make_tuple(
            sqlite3_column_int(stmt, 0),
            to_str(sqlite3_column_text(stmt, 1)),
            to_str(sqlite3_column_text(stmt, 2)));
    });   

    auto date_str_to_date = [](std::string str){
        int ymd[3];
        char buffer[5];
        int count{0};
        int val_count{0};
        // is one less than what it should be?
		for (auto* ptr = str.c_str(); 1; ++ptr) {
			char ch = *ptr;
            if(ch < '0' || ch > '9') {
                int val{0};
                for(int i = count - 1; i >= 0; --i){
                    val += buffer[i] * std::pow(10, (count - i) - 1);
                }
                ymd[val_count] = val;
                ++val_count;
				count = 0;
				if (ch == '\0')break;
				continue;
            }
            buffer[count] = ch - '0';
            ++count;
        }
        return Date{ymd[0], ymd[1], ymd[2]};
    };

    for(auto& e : entries){
        auto date = date_str_to_date(std::get<1>(e));
        std::string sql_update = stringbuilder() << "UPDATE Entries SET Date = '" << date.to_str() << "' WHERE Id = " << std::get<0>(e);
        sql(sql_update);
    }
}
*/
