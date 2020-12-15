#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>

#define LOG_PATH "ur0:data/"
#define LOG_FILE LOG_PATH "vitabright_log.txt"

void log_reset();
void log_write(const char *buffer, size_t length);

#ifdef ENABLE_LOGGING
#define LOG(...)                                                                                   \
  do {                                                                                             \
    char buffer[256];                                                                              \
    snprintf(buffer, sizeof(buffer), ##__VA_ARGS__);                                               \
    log_write(buffer, strlen(buffer));                                                             \
  } while (0)
#else
#define LOG(...)
#endif
#endif
