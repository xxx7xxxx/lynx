#ifndef _HTTP_H
#define _HTTP_H

#include "rio.h"

#define LISTENQ 100

#define	DATA_PATH "./www-data"
#define RECORD_FILE "http_record"

void *handle_http(int fd);

#endif
