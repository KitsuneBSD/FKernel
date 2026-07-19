#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>

typedef uint64_t pthread_t;
typedef uint64_t pthread_attr_t;
typedef uint64_t pthread_mutex_t;
typedef uint64_t pthread_mutexattr_t;
typedef uint64_t pthread_cond_t;
typedef uint64_t pthread_condattr_t;
typedef uint64_t pthread_key_t;
typedef uint64_t pthread_once_t;
typedef uint64_t pthread_rwlock_t;
typedef uint64_t pthread_rwlockattr_t;

#define PTHREAD_MUTEX_INITIALIZER  ((pthread_mutex_t)0)
#define PTHREAD_COND_INITIALIZER   ((pthread_cond_t)0)
#define PTHREAD_ONCE_INIT          ((pthread_once_t)0)

#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

#ifdef __cplusplus
extern "C" {
#endif

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
void pthread_exit(void *retval);
pthread_t pthread_self(void);
int pthread_equal(pthread_t t1, pthread_t t2);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate);

#ifdef __cplusplus
}
#endif
