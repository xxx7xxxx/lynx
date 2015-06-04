#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "threadpool.h"

struct task {
	cb_fun fun;
	void *arg;
};

struct threadpool {
	pthread_mutex_t lock;
	pthread_cond_t notify;

	pthread_t *thread_ids;
	int thread_count;
	int thread_started;

	struct task *task_queue;
	int task_size;
	int task_count;
	int task_head;
	int task_tail;		/*following of last one */

	int shutdown;
	int linger;
};

static void *worker(void *arg);
static void threadpool_free(threadpool_t * pool);

threadpool_t *threadpool_create(int thread_count, int task_size)
{
	threadpool_t *pool = calloc(1, sizeof(*pool));
	int i;

	if (thread_count <= 0 || task_size <= 0 || pool == NULL) {
		return NULL;
	}

	if (pthread_mutex_init(&(pool->lock), NULL) != 0 ||
	    pthread_cond_init(&(pool->notify), NULL) != 0) {
		free(pool);
		return NULL;
	}

	pool->thread_ids = malloc(sizeof(*pool->thread_ids)
				  * thread_count);
	if (pool->thread_ids == NULL) {
		free(pool);
		return NULL;
	}

	pool->thread_started = 0;
	for (i = 0; i < thread_count; i++) {
		if (pthread_create(&pool->thread_ids[i], NULL,
				   worker, (void *) pool) < 0) {
			/*not linger */
			threadpool_destory(pool, 0);
			return NULL;
		}
		pool->thread_started++;
	}

	pool->task_queue = malloc(sizeof(*pool->task_queue)
				  * task_size);
	if (pool->task_queue == NULL) {
		free(pool);
		return NULL;
	}
	pool->task_size = task_size;
	pool->task_count = 0;
	pool->task_head = pool->task_tail = 0;

	pool->shutdown = 0;
	pool->linger = 0;

	return pool;
}

int threadpool_add(threadpool_t * pool, cb_fun fun, void *arg)
{
	int err = 0;

	if (pool == NULL || fun == NULL)
		return TP_EINVAILD;

	do {
		/*enter */
		if (pthread_mutex_lock(&pool->lock) != 0) {
			err = TP_ELOCKFAIL;
			break;
		}

		if (pool->task_size == pool->task_count) {
			err = TP_EQUEUEFULL;
			break;
		}

		if (pool->shutdown == 1) {
			err = TP_ESHUTDOWN;
			break;
		}

		pool->task_queue[pool->task_tail].fun = fun;
		pool->task_queue[pool->task_tail].arg = arg;
		if (++pool->task_tail == pool->task_size)
			pool->task_tail = 0;
		pool->task_count++;

		if (pthread_cond_signal(&pool->notify) != 0) {
			err = TP_ESIGNAL;
			break;
		}
	} while (0);

	if (pthread_mutex_unlock(&pool->lock) != 0)
		err = TP_EUNLOCKFAIL;
	return err;
}

int threadpool_destory(threadpool_t * pool, int linger)
{
	int i;
	int err = 0;

	if (pool == NULL)
		return TP_EINVAILD;
	if (pthread_mutex_lock(&pool->lock))
		return TP_ELOCKFAIL;

	do {
		if (pool->shutdown) {
			err = TP_ESHUTDOWN;
			break;
		}
		pool->shutdown = 1;
		pool->linger = linger;

		if (pthread_cond_broadcast(&pool->notify) != 0) {
			err = TP_ESIGNAL;
			break;
		}

		if (pthread_mutex_unlock(&pool->lock) != 0) {
			err = TP_ELOCKFAIL;
			break;
		}

		for (i = 0; i < pool->thread_count; i++) {
			if (pthread_join(pool->thread_ids[i], NULL)) {
				err = TP_EJOINTHREAD;
				break;
			}
		}

	} while (0);

	if (err)
		return err;

	threadpool_free(pool);
	return 0;
}

static void threadpool_free(threadpool_t * pool)
{
	if (pool == NULL)
		return;

	pthread_mutex_lock(&pool->lock);
	if (pool->thread_ids)
		free(pool->thread_ids);
	if (pool->task_queue)
		free(pool->task_queue);

	pthread_mutex_destroy(&pool->lock);
	pthread_cond_destroy(&pool->notify);

	free(pool);
}

static void *worker(void *arg)
{
	threadpool_t *pool = (threadpool_t *) arg;
	struct task task;

	printf("thread id: %lu\n", pthread_self());
	for (;;) {
		pthread_mutex_lock(&pool->lock);
		while (pool->task_count == 0 && !pool->shutdown)
			pthread_cond_wait(&pool->notify, &pool->lock);

		if (pool->shutdown && !pool->linger)
			break;

		task.fun = pool->task_queue[pool->task_head].fun;
		task.arg = pool->task_queue[pool->task_head].arg;

		if (++pool->task_head == pool->task_size)
			pool->task_head = 0;
		pool->task_count--;
		pthread_mutex_unlock(&pool->lock);
		(*task.fun) (task.arg);
	}
	pool->thread_started--;

	pthread_mutex_unlock(&pool->lock);
	pthread_exit(0);
}
