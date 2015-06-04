#ifndef _REACTOR_H
#define _REACTOR_H

#define MAX_EVENT_NUMBER 	4096

void add_to_epoll(int epoll_fd, int add_fd, bool et);

void lt(struct epoll_event *events, int num_event,
		int epoll_fd, int listen_fd);

void et(struct epoll_event *events, int num_event,
		int epoll_fd, int listen_fd);

#endif

