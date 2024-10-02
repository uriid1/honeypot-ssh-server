#include <sqlite3.h>
#include <stdio.h>
#include "config.h"

// Функция для выполнения SQL запроса
int sql_execute(sqlite3 *db, const char *sql) {
  char *err_msg = 0;
  int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
  
  if (rc == SQLITE_OK) {
    return 1;
  }

  fprintf(stderr, "Error SQL: %s\n", err_msg);
  sqlite3_free(err_msg);
  return rc;
}

sqlite3* sql_init() {
  sqlite3 *db;

  // Открываем соединение с базой данных
  int rc = sqlite3_open(PATH_DATABASE, &db);
  
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Sqlite3 path: %s | Err: %s\n", PATH_DATABASE, sqlite3_errmsg(db));
    return NULL;
  }

  // Создание таблицы (если она не существует)
  const char *sql_create_table = "CREATE TABLE IF NOT EXISTS logs ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                 "date INTEGER, "
                                 "ip TEXT, "
                                 "user TEXT, "
                                 "password TEXT);";

  if (sql_execute(db, sql_create_table) != 1) {
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return NULL;
  }

  return db;
}
