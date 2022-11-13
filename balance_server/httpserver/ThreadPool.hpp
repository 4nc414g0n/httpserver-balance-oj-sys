#pragma once

#include <iostream>
#include <thread>
#include <algorithm>
#include <queue>
#include <pthread.h>


#include "Log.hpp"
//#include "BlockQueue.hpp"
#include "Reactor.hpp"
#include "Task.hpp"
#include "Pthread.hpp"
#include "Protocol.hpp"

#define test_port 8081
#define TNUM 1


class Task{
    private:
        int sock;
        ns_reactor::Event* event;
        CallBack handler; //设置回调
    public:
        Task()
        {}
        Task(int _sock,ns_reactor::Event* ev):sock(_sock),event(ev)
        {}
        //处理任务
        void ProcessOn()
        {
            handler(sock,event);
        }
        ~Task()
        {}
};
// class ThreadPool{
//     private:
//         int num;
//         bool stop;
//         std::queue<Task> task_queue;
//         pthread_mutex_t lock;
//         pthread_cond_t cond;

//         ThreadPool(int _num = TNUM):num(_num),stop(false)
//         {
//             pthread_mutex_init(&lock, nullptr);
//             pthread_cond_init(&cond, nullptr);
//         }
//         ThreadPool(const ThreadPool &){}

//         static ThreadPool *single_instance;
//     public:
//         static ThreadPool* getinstance()
//         {
//             static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
//             if(single_instance == nullptr){
//                 pthread_mutex_lock(&_mutex);
//                 if(single_instance == nullptr){
//                     single_instance = new ThreadPool();
//                     single_instance->InitThreadPool();
//                 }
//                 pthread_mutex_unlock(&_mutex);
//             }
//             return single_instance;
//         }

//         bool IsStop()
//         {
//             return stop;
//         }
//         bool TaskQueueIsEmpty()
//         {
//             return task_queue.size() == 0 ? true : false;
//         }
//         void Lock()
//         {
//             pthread_mutex_lock(&lock);
//         }
//         void Unlock()
//         {
//             pthread_mutex_unlock(&lock);
//         }
//         void ThreadWait()
//         {
//             pthread_cond_wait(&cond, &lock);
//         }
//         void ThreadWakeup()
//         {
//             pthread_cond_signal(&cond);
//         }
//         static void *ThreadRoutine(void *args)
//         {
//             ThreadPool *tp = (ThreadPool*)args;

//             while(true){
//                 Task t;
//                 tp->Lock();
//                 while(tp->TaskQueueIsEmpty()){
//                     tp->ThreadWait(); //当我醒来的时候，一定是占有互斥锁的！
//                 }
//                 tp->PopTask(t);
//                 tp->Unlock();
//                 t.ProcessOn();
//             }
//         }
//         bool InitThreadPool()
//         {
//             for(int i = 0; i < num; i++){
//                 pthread_t tid;
//                 if( pthread_create(&tid, nullptr, ThreadRoutine, this) != 0){
//                     LOG(LOG_LEVEL_FATAL, "create thread pool error!");
//                     return false;
//                 }
//             }
//             LOG(LOG_LEVEL_INFO, "create thread pool success!");
//             return true;
//         }
//         void PushTask(const Task &task) //in
//         {
//             Lock();
//             task_queue.push(task);
//             Unlock();
//             ThreadWakeup();
//         }
//         void PopTask(Task &task) //out
//         {
//             task = task_queue.front();
//             task_queue.pop();
//         }
//         ~ThreadPool()
//         {
//             pthread_mutex_destroy(&lock);
//             pthread_cond_destroy(&cond);
//         }
// };

// ThreadPool* ThreadPool::single_instance = nullptr;


#define MAX(a,b) ((a)>(b)?(a):(b))					

//通常工作线程数量与std::thread::hardware_concurrency()相同
#define T_NUM 6//定长线程池大小（如果是CPU密集型应用，则线程池大小设置为N+1
                        //如果是IO密集型应用，则线程池大小设置为2N+1）


template<typename T=Task>
class ThreadPool{
    private:  
        int _num;//线程池大小
        int _cap;//阻塞队列大小
        bool stop;
        std::queue<T> task_queue;
        //RingQueue<Task> task_queue;

        pthread_mutex_t lock;//用作条件变量与互斥访问
        pthread_mutex_t c_lock;//

        //Mutex c_lock;//多消费者锁

        pthread_cond_t have_space;//
        pthread_cond_t have_data;
    public:
        static void *ThreadRoutine(void *args)//非静态成员方法有个隐式参数*this static属性就没有（可以利用传参传入this）
        {
            ThreadPool<T> *tp = (ThreadPool<T>*)args;

            while(true){
                T t; 
                //tp->c_lock.lock();// 测试这里加锁会导致不能并发获取任务
                //pthread_mutex_lock(&(tp->c_lock));
                tp->PopTask(t);
                //tp->c_lock.unlock();
                //pthread_mutex_unlock(&(tp->c_lock));
                t.ProcessOn();
            }
        }
        bool InitThreadPool()
        {
            for(int i = 0; i < _num; i++){
                pthread_t tid;
                if( pthread_create(&tid, nullptr, ThreadRoutine, this) != 0){
                    LOG(LOG_LEVEL_FATAL, "create thread pool error!");
                    return false;
                }
            }
            LOG(LOG_LEVEL_INFO, "create thread pool success!");
            return true;
        } 
        bool IsFull()
        {
            return task_queue.size() == _cap ? true : false;
        }
        bool IsEmpty()
        {
            return task_queue.size() == 0 ? true : false;
        }
        bool IsStop()
        {
            return stop;
        }
        void PushTask(const T &task) //in
        { 
            //pthread_mutex_lock(&c_lock);

            pthread_mutex_lock(&lock);

            while(IsFull()){//if换为while防止broadcast伪唤醒
                LOG(LOG_LEVEL_INFO,  "=======IS FULL BLOCK start ... =======");
                pthread_cond_wait(&have_space, &lock); //满时阻塞，挂起时，自动释放锁，当你唤醒是，自动获取锁!
                //have_space.wait(&(lock._lock));
            }
            task_queue.push(task);
            if(!IsEmpty()){//有task即唤醒
                pthread_cond_signal(&have_data);//唤醒have_data
                //have_data.signal();
            }

            pthread_mutex_unlock(&lock);
           // pthread_mutex_unlock(&c_lock);
  
        }
        void PopTask(T &task) //out
        {
            pthread_mutex_lock(&lock);
            while(IsEmpty()){//if换为while防止broadcast伪唤醒
                pthread_cond_wait(&have_data, &lock);
                //have_data.wait(&(lock._lock));
            }
            task = task_queue.front();
            task_queue.pop();
            pthread_cond_signal(&have_space);//唤醒have_space
            pthread_mutex_unlock(&lock);
        }
    private://singleton
        ThreadPool(int num = T_NUM):
			_num(10)//(MAX(std::thread::hardware_concurrency(),num))//MAX(std::thread::hardware_concurrency(),num)
            ,_cap(_num)
			,stop(false)
        {
            pthread_mutex_init(&lock, nullptr);
            pthread_mutex_init(&c_lock, nullptr);
            pthread_cond_init(&have_data, nullptr);
            pthread_cond_init(&have_space, nullptr);
        }
        ThreadPool(const ThreadPool<T> &copy)=delete;
        ThreadPool& operator=(const ThreadPool<T> &lvalue)=delete;

        ~ThreadPool()
        {
            //cerr<<"~ThreadPool"<<endl;
            pthread_mutex_destroy(&lock);
            pthread_mutex_destroy(&c_lock);
            pthread_cond_destroy(&have_data);
            pthread_cond_destroy(&have_space);
        }

        static ThreadPool<T> *single_instance;	
    public:
        static ThreadPool<T>* getinstance()
        {
            static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
            if(single_instance == nullptr){
                pthread_mutex_lock(&_mutex);
                if(single_instance == nullptr){
                    single_instance = new ThreadPool<T>();
                    single_instance->InitThreadPool();
                }
                pthread_mutex_unlock(&_mutex);
            }
            return single_instance;
        }
        class CGarbo {
            public:
            ~CGarbo(){
                if (ThreadPool<>::single_instance)
                    delete ThreadPool<>::single_instance;
            }
        };
        // 定义一个静态成员变量，程序结束时，系统会自动调用它的析构函数从而释放单例对象
        static CGarbo Garbo;

};
template<> ThreadPool<>* ThreadPool<>::single_instance = nullptr;
