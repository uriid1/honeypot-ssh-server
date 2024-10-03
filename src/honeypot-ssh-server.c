/*-------------------------------------*/
/* Honeypot SSH-Server                 */
/* На основе libssh/ssh_server_fork    */
/* by uriid                            */
/*-------------------------------------*/

// Функции libssh
#include <libssh/libssh.h>

#include <libssh/callbacks.h>
#include <libssh/server.h>
// Стандартные библиотеки
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
// Обработка аргументов
#include <argp.h>
// Конфиг
#include "config.h"
// Утилиты write_log
#include "utils.h"
// Опциональная поддержка Sqlite
#ifdef SUPPORT_SQLITE
#include "sql.h"
#endif
// Константы скрипта
#define BUF_SIZE 1048576
#define SESSION_END (SSH_CLOSED | SSH_CLOSED_ERROR)

// Структура для данных канала
struct channel_data_struct {
  // PID дочернего процесса, который будет запущен в канале
  pid_t pid;
  // Для общения с дочерним процессом
  socket_t child_stdin;
  socket_t child_stdout;
  // Используется только для запросов подсистем и exec
  socket_t child_stderr;
  // Событие, которое используется для опроса вышеперечисленных дескрипторов
  ssh_event event;
};

// Структура для данных сессии
struct session_data_struct {
  // Указатель на канал, который будет выделен сессией
  ssh_channel channel;
  int auth_attempts;
  int authenticated;
  #ifdef SUPPORT_SQLITE
  sqlite3 *db;
  #endif
};

// Структура для хранения аргументов
struct arguments {
  char* port;
  char* path_ecdsa_key;
  char* path_log;
  int password_attempts_timeout;
  int password_attempts;
  int debug;
  int logging;
  int sqlite;
};
struct arguments args;

// Описание аргументов
static struct argp_option options[] = {
  {"port",                      'p',  "STRING",    0, "Port for connection (default is 22)", 0},
  {"path_ecdsa_key",            'k',  "STRING",    0, "Path to the ECDSA key (default is keys/ssh_host_ecdsa_key)", 0},
  {"path_log",                  'l',  "STRING",    0, "Path to log file (default is log/honeypot_ssh.log)", 0},
  {"password_attempts_timeout", 't',  "INT",       0, "Timeout for password attempts (default is 100ms)", 0},
  {"password_attempts",         'a',  "INT",       0, "Number of password attempts (default is 3)", 0},
  {"debug",                     'd',  "(1 or 0)",  0, "Enable debug mode, default is 0 (disabled)", 0},
  {"logging",                   'L',  "(1 or 0)",  0, "Enable logging mode, default is 0 (disabled)", 0},
  {"sqlite",                    's',  "(1 or 0)",  0, "Disable sqlite write, default is 1 (enabled)", 0},

  // Конец списка опций
  {0}
};

// Функция для обработки аргументов
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *args = state->input;

  switch (key) {
    case 'd':
      args->debug = atoi(arg);
      break;
    case 'p':
      args->port = arg;
      break;
    case 't':
      args->password_attempts_timeout = atoi(arg);
      break;
    case 'a':
      args->password_attempts = atoi(arg);
      break;
    case 'k':
      args->path_ecdsa_key = arg;
      break;
    case 'l':
      args->path_log = arg;
      break;
    case 'L':
      args->logging = atoi(arg);
      break;
    case 's':
      args->sqlite = atoi(arg);
      break;
    case ARGP_KEY_END:
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

// Описание argp
static char doc[] = "Honeypot ssh server based on LibSSH";
static char args_doc[] = "ARGS";

// Парсинг аргументов
void parse_args(int argc, char *argv[]) {
  // Установка значений по умолчанию
  args.port = PORT;
  args.path_ecdsa_key = PATH_ECDSA_KEY;
  args.path_log = PATH_LOG;
  args.password_attempts_timeout = PASSWORD_INPUT_TIMEOUT_MS;
  args.password_attempts = PASSWORD_ATTEMPTS;
  args.debug = DEBUG;
  args.logging = LOGGING;
  args.sqlite = WRITE_SQLITE;

  struct argp argp = {options, parse_opt, args_doc, doc, NULL, NULL, NULL};

  // Обработка аргументов
  argp_parse(&argp, argc, argv, 0, 0, &args);
}

// Коллбэк ввода пароля
static int auth_password(ssh_session session, const char *user, const char *pass, void *userdata) {
  struct session_data_struct *sdata = (struct session_data_struct *) userdata;

  // Запись в лог
  const char* ip = get_сlient_ip(session);
  if (ip == NULL) ip = "null";
  const char* date = get_date_time();
  const char* fmts = "[%s] Client enter password | IP: %s | User: %s | Pass: %s\n";

  int size_buff = sizeof(date)+sizeof(ip)+sizeof(user)+sizeof(pass);
  char buffer[256+size_buff];
  snprintf(buffer, sizeof(buffer), fmts, date, ip, user, pass);
  
  // Логирование
  if (args.logging)
    write_log(args.path_log, buffer);

  // Отладка
  if (args.debug)
    fprintf(stdout, fmts, date, ip, user, pass);

  // Запись в базу данных
  #ifdef SUPPORT_SQLITE
  if (args.sqlite) {
    sqlite3 *db = sdata->db;

    const char *sql_insert_template = "INSERT INTO logs (date, ip, user, password) VALUES ('%s', '%s', '%s', '%s');";
    char sql_insert[512+size_buff];

    time_t unixtime = time(NULL);
    snprintf(sql_insert, sizeof(sql_insert), sql_insert_template, ctime(&unixtime), ip, user, pass);

    // Запрос на вставку данных
    sql_execute(db, sql_insert);
  }
  #endif

  sdata->auth_attempts++;
  return SSH_AUTH_DENIED;
}

// Клиент запросил запрос на новый канал
static ssh_channel channel_open(ssh_session session, void *userdata) {
  struct session_data_struct *sdata = (struct session_data_struct *) userdata;

  // Запись в лог
  const char* ip = get_сlient_ip(session);
  if (ip == NULL) ip = "null";
  const char* date = get_date_time();
  const char* fmts = "[%s] Clien request to open channel | IP: %s\n";

  char buffer[256+sizeof(date)+sizeof(ip)];
  snprintf(buffer, sizeof(buffer), fmts, date, ip);

  // Логирование
  if (args.logging)
    write_log(args.path_log, buffer);

  // Отладка
  if (args.debug)
    fprintf(stdout, fmts, date, ip);

  sdata->channel = ssh_channel_new(session);
  return sdata->channel;
}

static int process_stdout(socket_t fd, int revents, void *userdata) {
  char buf[BUF_SIZE];
  int n = -1;
  ssh_channel channel = (ssh_channel) userdata;

  if (channel != NULL && (revents & POLLIN) != 0) {
    n = read(fd, buf, BUF_SIZE);
    if (n > 0) {
      ssh_channel_write(channel, buf, n);
    }
  }

  return n;
}

static int process_stderr(socket_t fd, int revents, void *userdata) {
  char buf[BUF_SIZE];
  int n = -1;
  ssh_channel channel = (ssh_channel) userdata;

  if (channel != NULL && (revents & POLLIN) != 0) {
    n = read(fd, buf, BUF_SIZE);
    if (n > 0) {
      ssh_channel_write_stderr(channel, buf, n);
    }
  }

  return n;
}

static void handle_session(ssh_event event, ssh_session session, sqlite3 *db) {
  // Наша структура, содержащая информацию о канале
  struct channel_data_struct cdata = {
    .pid = 0,
    .child_stdin = -1,
    .child_stdout = -1,
    .child_stderr = -1,
    .event = NULL,
  };

  // Наша структура, содержащая информацию о сессии
  struct session_data_struct sdata = {
    .channel = NULL,
    .auth_attempts = 0,
    .authenticated = 0,
    #ifdef SUPPORT_SQLITE
    .db = db,
    #endif
  };

  struct ssh_server_callbacks_struct server_cb = {
    .userdata = &sdata,
    .auth_password_function = auth_password,
    .channel_open_request_session_function = channel_open,
  };

  // Запись в лог
  const char* ip = get_сlient_ip(session);
  if (ip == NULL) ip = "null";
  const char* date = get_date_time();
  const char* fmts = "[%s] New session | IP: %s\n";

  char buffer[256+sizeof(date)+sizeof(ip)];
  snprintf(buffer, sizeof(buffer), fmts, date, ip);

  // Логирование
  if (args.logging)
    write_log(args.path_log, buffer);

  // Отладка
  if (args.debug)
    fprintf(stdout, fmts, date, ip);

  ssh_callbacks_init(&server_cb);
  ssh_set_server_callbacks(session, &server_cb);
  ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD);

  if (ssh_handle_key_exchange(session) != SSH_OK) {
    fprintf(stderr, "%s\n", ssh_get_error(session));
    return;
  }

  ssh_event_add_session(event, session);

  int n = 0;
  while (sdata.authenticated == 0 || sdata.channel == NULL) {
    // Если пользователь использовал все попытки, или он не смог -
    // аутентифицироваться в течение 10 секунд (n * 100 мс), отключаем.
    if (sdata.auth_attempts >= PASSWORD_ATTEMPTS || n >= args.password_attempts_timeout) {
      return;
    }

    if (ssh_event_dopoll(event, 100) == SSH_ERROR) {
      char* date = get_date_time();
      const char* err = ssh_get_error(session);
      const char* fmts = "[%s] Client message | IP: %s | %s\n";

      // Запись в лог
      char buffer[256+sizeof(date)+sizeof(ip)+sizeof(err)];
      snprintf(buffer, sizeof(buffer), fmts, date, ip, err);

      // Логирование
      if (args.logging)
        write_log(args.path_log, buffer);

      if (args.debug)
        fprintf(stdout, fmts, date, ip, err);

      return;
    }
    n++;
  }

  int rc;
  do {
    // Опрос основного события, которое отвечает за сессию, канал и
    // даже стандартный вывод/ошибки нашего дочернего процесса (как только он будет запущен).
    if (ssh_event_dopoll(event, -1) == SSH_ERROR) {
      ssh_channel_close(sdata.channel);
    }

    // Если стандартный вывод/ошибки дочернего процесса были зарегистрированы с событием,
    // или дочерний процесс еще не запущен, продолжаем
    if (cdata.event != NULL || cdata.pid == 0) {
      continue;
    }
    // Выполняется только один раз, как только дочерний процесс запущен
    cdata.event = event;
    // Если стандартный вывод действителен, добавляем стандартный вывод для мониторинга событием
    if (cdata.child_stdout != -1) {
      if (ssh_event_add_fd(event, cdata.child_stdout, POLLIN, process_stdout, sdata.channel) != SSH_OK) {
        fprintf(stderr, "Failed to register stdout to poll context\n");
        ssh_channel_close(sdata.channel);
      }
    }

    // Если стандартный вывод ошибок действителен, добавляем стандартный вывод ошибок для мониторинга событием.
    if (cdata.child_stderr != -1){
      if (ssh_event_add_fd(event, cdata.child_stderr, POLLIN, process_stderr, sdata.channel) != SSH_OK) {
        fprintf(stderr, "Failed to register stderr to poll context\n");
        ssh_channel_close(sdata.channel);
      }
    }
  } while(ssh_channel_is_open(sdata.channel) && (cdata.pid == 0 || waitpid(cdata.pid, &rc, WNOHANG) == 0));

  close(cdata.child_stdin);
  close(cdata.child_stdout);
  close(cdata.child_stderr);

  // Удаляем дескрипторы из контекста опроса, так как они теперь  -
  // закрыты, они всегда будут вызывать срабатывание во время опросов
  ssh_event_remove_fd(event, cdata.child_stdout);
  ssh_event_remove_fd(event, cdata.child_stderr);

  // Если дочерний процесс завершился
  if (kill(cdata.pid, 0) < 0 && WIFEXITED(rc)) {
    rc = WEXITSTATUS(rc);
    ssh_channel_request_send_exit_status(sdata.channel, rc);
  } else if (cdata.pid > 0) {
    // Если клиент закрыл канал или процесс не завершился корректно,
    // но только если что-то было запущено
    kill(cdata.pid, SIGKILL);
  }

  ssh_channel_send_eof(sdata.channel);
  ssh_channel_close(sdata.channel);

  // Ждем до 5 секунд, чтобы клиент завершил сессию
  for (n = 0; n < 50 && (ssh_get_status(session) & SESSION_END) == 0; n++) {
    ssh_event_dopoll(event, 100);
  }
}

// Обработчик SIGCHLD для очистки завершившихся дочерних процессов
static void sigchld_handler(int signo) {
  (void) signo;
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, char *argv[]) {
  // Обработка аргументов
  parse_args(argc, argv);

  // Инициализация Sqlite
  #ifdef SUPPORT_SQLITE
  sqlite3 *db = sql_init();
  if (db == NULL) {
    return EXIT_FAILURE;
  }
  #endif

  ssh_bind sshbind;
  ssh_session session;
  ssh_event event;
  struct sigaction sa;

  // Установка обработчика SIGCHLD
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if (sigaction(SIGCHLD, &sa, NULL) != 0) {
    fprintf(stderr, "Failed to register SIGCHLD handler\n");
    return EXIT_FAILURE;
  }

  ssh_init();
  sshbind = ssh_bind_new();

  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, args.port);
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_ECDSAKEY, args.path_ecdsa_key);
  // Подмена баннера SSH-2.0-libssh_0.11.1 -> SSH-2.0-OpenSSH_8.9
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BANNER, BANNER);

  if(ssh_bind_listen(sshbind) < 0) {
    fprintf(stderr, "%s\n", ssh_get_error(sshbind));
    return EXIT_FAILURE;
  }

  fprintf(stdout, "Honeypot SSH-Server Started 0.0.0.0:%s\n", args.port);
  while (1) {
    session = ssh_new();
    if (session == NULL) {
      fprintf(stderr, "Failed to allocate session\n");
      continue;
    }

    // Таймаут для сессии
    unsigned int timeout = 100;
    ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout);

    // Блокирует выполнение до появления нового входящего соединения
    if(ssh_bind_accept(sshbind, session) != SSH_ERROR) {
      switch(fork()) {
        case 0:
          // Убираем обработчик SIGCHLD, унаследованный от родителя
          sa.sa_handler = SIG_DFL;
          sigaction(SIGCHLD, &sa, NULL);

          // Убираем привязку сокета, что позволяет нам перезапустить -
          // родительский процесс, не завершив существующие сессии
          ssh_bind_free(sshbind);

          event = ssh_event_new();
          if (event != NULL) {
            // Блокирует выполнение до завершения SSH-сессии, либо -
            // когда дочерний процесс завершится, либо клиент отключится
            handle_session(event, session, db);
            ssh_event_free(event);
          } else {
            fprintf(stderr, "Could not create polling context\n");
          }
          ssh_disconnect(session);
          ssh_free(session);

          exit(EXIT_SUCCESS);
        case -1:
          fprintf(stderr, "Failed to fork\n");
      }
    } else {
      fprintf(stderr, "%s\n", ssh_get_error(sshbind));
    }
    // Поскольку сессия была передана дочернему процессу, производим очистку -
    // в родительском процессе.
    ssh_disconnect(session);
    ssh_free(session);
  }

  #ifdef SUPPORT_SQLITE
  sqlite3_close(db);
  #endif

  ssh_bind_free(sshbind);
  ssh_finalize();
  return EXIT_SUCCESS;
}
