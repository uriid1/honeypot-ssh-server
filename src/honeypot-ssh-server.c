/////////////////////////////////////
// SSH honeypot
// На основе libssh/ssh_server_fork
// by uriid
/////////////////////////////////////

// Функции libssh
#include <libssh/callbacks.h>
#include <libssh/server.h>
// Работа с Ос
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
// Для получение данных клиента
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
// Конфиг
#include "config.h"
// Лог
#include "write_log.h"
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
};

// Ip адресс клиента строкой
char* get_сlient_ip(ssh_session session) {
  // Достаточно для хранения IPv4-адреса
  static char ip[INET_ADDRSTRLEN];
  struct sockaddr_storage tmp;
  socklen_t len = sizeof(tmp);

  getpeername(ssh_get_fd(session), (struct sockaddr*)&tmp, &len);
  
  // Обработка ipV4 адреса
  if (tmp.ss_family == AF_INET) {
    struct sockaddr_in *sock = (struct sockaddr_in *)&tmp;
    inet_ntop(AF_INET, &sock->sin_addr, ip, sizeof(ip));
    
    return ip;
  } else {
    // Для V6 пока нет
    return NULL;
  }

  return NULL;
}

// Дата и время строкой
char* get_date_time() {
  // Текущее время
  time_t t = time(NULL);
  // Преобразуем в локальное время
  struct tm *tm_info = localtime(&t);

  // Форматируем дату и время в строку
  static char buffer[80];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

  return buffer;
}

// Коллбэк ввода пароля
static int auth_password(ssh_session session, const char *user, const char *pass, void *userdata) {
  struct session_data_struct *sdata = (struct session_data_struct *) userdata;

  // Отладка
  char* ip = get_сlient_ip(session);
  if (ip == NULL) ip = "null";
  char* date = get_date_time();
  if (DEBUG) {
    fprintf(stdout,
      "[%s] Client enter password | IP: %s | User: %s | Pass: %s\n", date, ip, user, pass);
  } else {
    // Заись в лог
    char buffer[256+sizeof(date)+sizeof(ip)+sizeof(user)+sizeof(pass)];
    snprintf(buffer, sizeof(buffer),
      "[%s] Client enter password | IP: %s | User: %s | Pass: %s\n", date, ip, user, pass);
    write_log(PATH_LOG, buffer);
  }

  sdata->auth_attempts++;
  return SSH_AUTH_DENIED;
}

// Клиент запросил запрос на новый канал
static ssh_channel channel_open(ssh_session session, void *userdata) {
  struct session_data_struct *sdata = (struct session_data_struct *) userdata;

  // Отладка
  char* ip = get_сlient_ip(session);
  if (ip == NULL) ip = "null";
  char* date = get_date_time();

  if (DEBUG) {
    fprintf(stdout, "[%s] Clien request to open channel | IP: %s\n", date, ip);
  } else {
    // Запись в лог
    char buffer[256+sizeof(date)+sizeof(ip)];
    snprintf(buffer, sizeof(buffer),
      "[%s] Clien request to open channel | IP: %s\n", date, ip);
    write_log(PATH_LOG, buffer); 
  }

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

static void handle_session(ssh_event event, ssh_session session) {
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
    .authenticated = 0
  };

  struct ssh_server_callbacks_struct server_cb = {
    .userdata = &sdata,
    .auth_password_function = auth_password,
    .channel_open_request_session_function = channel_open,
  };

  // Отладка
  char* ip = get_сlient_ip(session);
  if (ip == NULL) { ip = "null"; }
  char* date = get_date_time();

  if (DEBUG) {
    fprintf(stdout, "[%s] New session | IP: %s\n", date, ip);
  } else {
    // Запись в лог
    char buffer[256+sizeof(date)+sizeof(ip)];
    snprintf(buffer, sizeof(buffer),
      "[%s] New session | IP: %s\n", date, ip);
    write_log(PATH_LOG, buffer); 
  }
  //

  ssh_callbacks_init(&server_cb);
  ssh_set_server_callbacks(session, &server_cb);

  if (ssh_handle_key_exchange(session) != SSH_OK) {
    fprintf(stderr, "%s\n", ssh_get_error(session));
    return;
  }

  ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD);
  ssh_event_add_session(event, session);

  int rc;
  int n = 0;
  while (sdata.authenticated == 0 || sdata.channel == NULL) {
    // Если пользователь использовал все попытки, или он не смог -
    // аутентифицироваться в течение 10 секунд (n * 100 мс), отключаем.
    if (sdata.auth_attempts >= 3 || n >= 100) {
      return;
    }

    if (ssh_event_dopoll(event, 100) == SSH_ERROR) {
      char* date = get_date_time();
      const char* err = ssh_get_error(session);
      if (DEBUG) {
        fprintf(stdout, "[%s] Client message | IP: %s | %s\n", date, ip, err);
      } else {
        // Запись в лог
        char buffer[256+sizeof(date)+sizeof(ip)+sizeof(err)];
        snprintf(buffer, sizeof(buffer),
         "[%s] Client message | IP: %s | %s\n", date, ip, err);
        write_log(PATH_LOG, buffer); 
      }
      return;
    }
    n++;
  }

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

int main() {
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
    return 1;
  }

  ssh_init();
  sshbind = ssh_bind_new();

  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, PORT);
  ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_ECDSAKEY, PATH_ECDSA_KEY);

  if(ssh_bind_listen(sshbind) < 0) {
    fprintf(stderr, "%s\n", ssh_get_error(sshbind));
    return 1;
  }

  fprintf(stdout, "SSH Server Started 0.0.0.0:%s\n", PORT);

  while (1) {
    session = ssh_new();
    if (session == NULL) {
      fprintf(stderr, "Failed to allocate session\n");
      continue;
    }

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
            handle_session(event, session);
            ssh_event_free(event);
          } else {
            fprintf(stderr, "Could not create polling context\n");
          }
          ssh_disconnect(session);
          ssh_free(session);

          exit(0);
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

  ssh_bind_free(sshbind);
  ssh_finalize();
  return 0;
}
