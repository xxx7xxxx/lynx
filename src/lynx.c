#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <strings.h>

#include "threadpool.h"
#include "reactor.h"
#include "http.h"

#define REACOR    1

/*in default, the I/O model is threadpool*/
#ifndef REACOR

int main()
{
	int 			listenfd, connfd;
	socklen_t		clilen;
	struct sockaddr_in	cliaddr, servaddr;
	threadpool_t		*pool;
	int			reuseaddr = 1;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(8888);

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);

	pool = threadpool_create(4, 10);

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
		threadpool_add(pool, (cb_fun)*handle_http, (void *)connfd);
	}

	threadpool_destory(pool, TP_LINGER);
	close(listenfd);

	return 0;
}

#else

#define LT_MODE 1

int main()
{
	int			ret;
	int 			listen_fd;
	int			epoll_fd;
	struct epoll_event	events[MAX_EVENT_NUMBER];
	struct sockaddr_in	servaddr;
	int			reuseaddr = 1;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(8888);

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
	bind(listen_fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listen_fd, 5);

	epoll_fd = epoll_create(5);
#ifdef LT_MODE
	add_to_epoll(epoll_fd, listen_fd, false);
#else
	add_to_epoll(epoll_fd, listen_fd, true);
#endif

	for ( ;; ) {
		ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
		if (ret < 0)
			fprintf(stderr, "epoll_wait error\n");
#ifdef LT_MODE
		lt(events, ret, epoll_fd, listen_fd);
#else
		et(events, ret, epoll_fd, listen_fd);
#endif
	}

	return 0;
}

#endif
