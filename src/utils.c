#include <libssh/libssh.h>
#include <stdio.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Запись лога
void write_log(const char *path, const char *message) {
  FILE *log = fopen(path, "a+");
  if (log == NULL) {
    fprintf(stderr, "Unable to open  %s\n", path);
    return;
  }

  fprintf(log, message);
  fclose(log);
}

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