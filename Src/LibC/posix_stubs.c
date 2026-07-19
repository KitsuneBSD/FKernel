#include <LibC/fcntl.h>
#include <LibC/dirent.h>
#include <LibC/sys/stat.h>
#include <LibC/stdlib.h>

/* These are user-space POSIX stubs. The kernel uses VFS directly. */

int open(const char *path, int flags, ...) {
  (void)path; (void)flags;
  abort();
}

int close(int fd) {
  (void)fd;
  abort();
}

int fcntl(int fd, int cmd, ...) {
  (void)fd; (void)cmd;
  abort();
}

int stat(const char *path, struct stat *buf) {
  (void)path; (void)buf;
  abort();
}

int fstat(int fd, struct stat *buf) {
  (void)fd; (void)buf;
  abort();
}

int lstat(const char *path, struct stat *buf) {
  (void)path; (void)buf;
  abort();
}

int mkdir(const char *path, unsigned int mode) {
  (void)path; (void)mode;
  abort();
}

int chmod(const char *path, unsigned int mode) {
  (void)path; (void)mode;
  abort();
}

DIR *opendir(const char *path) {
  (void)path;
  abort();
}

struct dirent *readdir(DIR *dir) {
  (void)dir;
  abort();
}

int closedir(DIR *dir) {
  (void)dir;
  abort();
}

void rewinddir(DIR *dir) {
  (void)dir;
  abort();
}

/* termios stubs */
#include <LibC/termios.h>

int tcgetattr(int fd, struct termios *termios_p) {
  (void)fd; (void)termios_p;
  return -1;
}

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
  (void)fd; (void)optional_actions; (void)termios_p;
  return 0;
}

int tcflush(int fd, int queue_selector) {
  (void)fd; (void)queue_selector;
  return 0;
}

int tcflow(int fd, int action) {
  (void)fd; (void)action;
  return 0;
}

speed_t cfgetispeed(const struct termios *termios_p) {
  (void)termios_p;
  return B9600;
}

speed_t cfgetospeed(const struct termios *termios_p) {
  (void)termios_p;
  return B9600;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
  if (termios_p) termios_p->c_ispeed = speed;
  return 0;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
  if (termios_p) termios_p->c_ospeed = speed;
  return 0;
}

/* pthread stubs — single-threaded stub implementation */
#include <LibC/pthread.h>

static void* s_tls_slots[128] = {0};

int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void*), void *arg) {
  (void)t; (void)a; (void)fn; (void)arg; return -1;
}
int pthread_join(pthread_t t, void **r)         { (void)t; (void)r; return 0; }
int pthread_detach(pthread_t t)                 { (void)t; return 0; }
void pthread_exit(void *r)                      { (void)r; abort(); }
pthread_t pthread_self(void)                    { return (pthread_t)1; }
int pthread_equal(pthread_t a, pthread_t b)     { return a == b; }

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a) { (void)a; if(m) *m=0; return 0; }
int pthread_mutex_destroy(pthread_mutex_t *m)   { (void)m; return 0; }
int pthread_mutex_lock(pthread_mutex_t *m)      { (void)m; return 0; }
int pthread_mutex_trylock(pthread_mutex_t *m)   { (void)m; return 0; }
int pthread_mutex_unlock(pthread_mutex_t *m)    { (void)m; return 0; }

int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a) { (void)a; if(c) *c=0; return 0; }
int pthread_cond_destroy(pthread_cond_t *c)     { (void)c; return 0; }
int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m) { (void)c; (void)m; return 0; }
int pthread_cond_signal(pthread_cond_t *c)      { (void)c; return 0; }
int pthread_cond_broadcast(pthread_cond_t *c)   { (void)c; return 0; }

int pthread_once(pthread_once_t *o, void (*fn)(void)) {
  if (!o || *o) return 0;
  *o = 1; if (fn) fn(); return 0;
}

int pthread_key_create(pthread_key_t *k, void (*d)(void*)) {
  static pthread_key_t next = 0;
  (void)d; if (k) *k = next++; return 0;
}
int pthread_key_delete(pthread_key_t k)         { (void)k; return 0; }
void *pthread_getspecific(pthread_key_t k)      { return k < 128 ? s_tls_slots[k] : 0; }
int pthread_setspecific(pthread_key_t k, const void *v) {
  if (k < 128) { s_tls_slots[k] = (void*)v; return 0; } return -1;
}

int pthread_attr_init(pthread_attr_t *a)                    { if(a) *a=0; return 0; }
int pthread_attr_destroy(pthread_attr_t *a)                 { (void)a; return 0; }
int pthread_attr_setdetachstate(pthread_attr_t *a, int d)   { (void)a; (void)d; return 0; }
int pthread_attr_getdetachstate(const pthread_attr_t *a, int *d) { (void)a; if(d) *d=0; return 0; }
