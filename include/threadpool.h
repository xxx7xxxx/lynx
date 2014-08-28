#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

/*error number*/
enum {
	TP_EINVAILD = -1,
	TP_ELOCKFAIL = -2,
	TP_EUNLOCKFAIL = -3,
	TP_ESIGNAL = -4,
	TP_EQUEUEFULL = -5,
	TP_ESHUTDOWN = -6,
	TP_EJOINTHREAD = -7,
};

/*Does it finish all added tasks before shutdown*/
enum {
	TP_LINGER = 1
};

typedef struct threadpool threadpool_t;
typedef void *(*cb_fun) (void *);

threadpool_t *threadpool_create(int thread_count, int task_size);

int threadpool_add(threadpool_t * pool, cb_fun fun, void *arg);

/*flag = TP_SHUTDOWN_LINGER or 0*/
int threadpool_destory(threadpool_t * pool, int linger);

#endif
