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
#include "configure.h"

#define LT_MODE 1

int main()
{
	int 			listenfd, connfd;
	socklen_t		clilen;
	struct sockaddr_in	cliaddr, servaddr;
	threadpool_t		*pool;
	int			reuseaddr = 1;

	int			ret;
	int			epoll_fd;
	struct epoll_event	events[MAX_EVENT_NUMBER];

        start_conf();

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd == -1)
                fprintf(stderr, "socket error\n");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(80);

	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
        if (ret == -1)
                fprintf(stderr, "setsockopt error\n");

	ret = bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
        if (ret == -1)
                fprintf(stderr, "bind error\n");

	ret = listen(listenfd, LISTENQ);
        if (ret == -1)
                fprintf(stderr, "listen error\n");


        if (conf[MODEL_SUB].value == MODEL_REACTOR) {

                epoll_fd = epoll_create(5);
                add_to_epoll(epoll_fd, listenfd, LT_MODE ? false : true);

                for ( ;; ) {
                        ret = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
                        if (ret < 0)
                                fprintf(stderr, "epoll_wait error\n");

                        LT_MODE ? lt(events, ret, epoll_fd, listenfd) :
                                et(events, ret, epoll_fd, listenfd);
                }

        } else {
                pool = threadpool_create(4, 10);

                for ( ; ; ) {
                        clilen = sizeof(cliaddr);
                        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
                        threadpool_add(pool, (cb_fun)*handle_http, (void *)connfd);
                }

                threadpool_destory(pool, TP_LINGER);
                close(listenfd);
        }

	return 0;
}
