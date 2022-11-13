#include <pthread.h>

//简单封装pthread库的互斥锁和条件变量，方便使用
class Mutex {
    public:
        Mutex() {
            pthread_mutex_init(&_lock, NULL);
            _is_locked = false;
        }
        ~Mutex() {
            while(_is_locked);
            unlock(); // Unlock Mutex after shared resource is safe
            pthread_mutex_destroy(&_lock);
        }
        void lock() {
            pthread_mutex_lock(&_lock);
            _is_locked = true;
        }
        void unlock() {
            _is_locked = false; // do it BEFORE unlocking to avoid race condition
            pthread_mutex_unlock(&_lock);
        }
        pthread_mutex_t* get_mutex_ptr() {
            return &_lock;
        }
    private:
        pthread_mutex_t _lock;
        volatile bool _is_locked;
};

class CondVar {
    public:
        CondVar() { pthread_cond_init(&m_cond_var, NULL); }
        ~CondVar() { pthread_cond_destroy(&m_cond_var); }
        void wait(pthread_mutex_t* mutex) {pthread_cond_wait(&m_cond_var, mutex); }
        void signal() { pthread_cond_signal(&m_cond_var); }
        void broadcast() { pthread_cond_broadcast(&m_cond_var); }
    private:
        pthread_cond_t m_cond_var;
};
