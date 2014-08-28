#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <strings.h>
#include <fcntl.h>

#include "reactor.h"
#include "http.h"


static void set_nonblock(int fd);

void
add_to_epoll(int epoll_fd, int add_fd, bool et)
{
	struct epoll_event event;
	event.events = EPOLLIN;
	event.data.fd = add_fd;
	if (et)
		event.events |= EPOLLET;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, add_fd, &event);
	set_nonblock(add_fd);
}

static void
set_nonblock(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;

	fcntl(fd, F_SETFL, new_option);
}

void lt(struct epoll_event *events, int num_event,
		int epoll_fd, int listen_fd)
{
	int 			i;
	int			client_fd;
	struct sockaddr_in 	client_addr;
	socklen_t		client_len;

	for (i = 0; i < num_event; i++) {
		int sock_fd = events[i].data.fd;
		if (sock_fd == listen_fd) {
			client_len = sizeof(client_addr);
			client_fd =
				accept(listen_fd,
				(struct sockaddr *)&client_addr,
				&client_len);
			if (client_fd < 0)
				fprintf(stderr, "accept error\n");
			else
				add_to_epoll(epoll_fd, client_fd, false);
		} else if (events[i].events & EPOLLIN){
			printf("event trigger once\n");
			handle_http(sock_fd);
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, NULL);
		}
	}
}

void et(struct epoll_event *events, int num_event,
		int epoll_fd, int listen_fd)
{
	int 			i;
	int			client_fd;
	struct sockaddr_in 	client_addr;
	socklen_t		client_len;

	for (i = 0; i < num_event; i++) {
		int sock_fd = events[i].data.fd;
		if (sock_fd == listen_fd) {
			client_len = sizeof(client_addr);
			client_fd =
				accept(listen_fd,
				(struct sockaddr *)&client_addr,
				&client_len);
			if (client_fd < 0)
				fprintf(stderr, "accept error");
			else
				add_to_epoll(epoll_fd, client_fd, true);//different
		} else if (events[i].events & EPOLLIN){
			printf("event trigger once\n");
			handle_http(sock_fd);
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, NULL);
		} else {
			printf("something else happen\n");
		}
	}

}
