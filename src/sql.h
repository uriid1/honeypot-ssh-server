// sql.h
#ifndef SQL_H
#define SQL_H
#include <sqlite3.h>
sqlite3* sql_init();
int sql_execute(sqlite3 *db, const char *sql);
#endif // SQL_H
