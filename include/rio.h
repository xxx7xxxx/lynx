#ifndef _UTIL_H
#define _UTIL_H

#define	MAXLINE	 8192
#define MAXBUF   8192

typedef struct rio rio_t;

struct rio {
	int rio_fd;
	int rio_cnt;
	char *rio_bufptr;
	char rio_buf[MAXBUF];
};

ssize_t readn(int fd, void *usrbuf, size_t n);
ssize_t writen(int fd, void *usrbuf, size_t n);
void rio_readinit(rio_t * rp, int fd);
ssize_t rio_readn(rio_t * rp, void *usrbuf, size_t n);
ssize_t rio_readline(rio_t * rp, void *usrbuf, size_t maxlen);

#endif
