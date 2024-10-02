// utils.h
#ifndef UTILS_H
#define UTILS_H
void write_log(const char *path, const char *message);
char* get_сlient_ip(ssh_session session);
char* get_date_time();
#endif // UTILS_H
