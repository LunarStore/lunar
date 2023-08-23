#include"base/thread.h"
#include<string.h>
#include "base/macro.h"
namespace lunar{
    static Logger::ptr g_logger = LUNAR_LOG_NAME("system");
    //线程对象生命周期可能和线程本身生命周期不同，所以不推荐每个线程用线程全局变量保存线程对象的地址，智能指针不能泛滥使用!!!
    // static thread_local Thread* t_thread = nullptr;

    //参考Muduo做法
    static thread_local std::string t_thread_name("unknow");
    static thread_local pid_t t_thread_pid = 0;

    std::atomic<int32_t> Thread::ThreadCount = { 0 };
    
    Thread::Thread(const CallBackType& cb, const std::string& name): m_id(-1), 
        m_pthread(0), 
        m_name(name), 
        m_cb(cb), 
        m_sem(0){

        setDefaultName();
        int rt = pthread_create(&m_pthread, nullptr, &Thread::Main, this);
        if(rt){
            LUNAR_LOG_ERROR(g_logger) << "pthread_create(...) fail, rt = "
                << rt << "<errorstr=" << strerror(rt) 
                << ">,thread name = " << m_name;
            throw std::logic_error("pthread_create error");
        }

        m_sem.wait();

        LUNAR_ASSERT(m_id > 0);
    }

    const std::string& Thread::GetName(){
        return t_thread_name;
    }

    pid_t Thread::GetTid(){
        if(t_thread_pid == 0){
            //参考Muduo cach线程pid
            t_thread_pid = GetThreadId();
        }
        return t_thread_pid;
    }

    void Thread::join(){
        LUNAR_ASSERT(m_pthread != 0);
        if(m_pthread){
            int rt = pthread_join(m_pthread, nullptr);
            if(rt){
                LUNAR_LOG_ERROR(g_logger) << "pthread_join(...) fail, rt = "
                << rt << "<errorstr=" << strerror(rt) 
                << ">,thread name = " << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_pthread = 0;
        }
    }
    const std::string& Thread::getName(){
        return m_name;
    }
    Thread::~Thread(){
        if(m_pthread){
            int rt = pthread_detach(m_pthread);
            if(rt){
                LUNAR_LOG_ERROR(g_logger) << "pthread_detach(...) fail, rt = "
                    << rt << "thread name = " 
                    << m_name << "<" << strerror(rt) << ">";
            }
            LUNAR_LOG_DEBUG(g_logger) << "Thread::Detach()";
        }
        LUNAR_LOG_DEBUG(g_logger) << "Thread::~Thread()";
    }
    void Thread::setDefaultName(){
        int32_t number = ++ThreadCount;
        if(m_name.empty() == true){
            char buf[32];
            snprintf(buf, sizeof buf, "Thread%d", number);
            m_name = buf;
        }
    }
    void* Thread::Main(void* arg){
        Thread* thread = ((Thread *)arg);
        std::string threadName;
        t_thread_name = thread->m_name;
        thread->m_id = Thread::GetTid();
        //同::prctl(PR_SET_NAME, thread->m_name.c_str());
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        CallBackType cb;
        cb.swap(thread->m_cb);
        thread->m_sem.post();

        threadName = t_thread_name;
        try{
            cb();
            t_thread_name = "finished";
        }catch(const std::exception& ex){
            t_thread_name = "crashed";
            LUNAR_LOG_FATAL(g_logger) << "exception caught in Thread " + threadName
                << "\n"
                << "reason: " 
                << ex.what()
                << std::endl;

            abort();
        }catch(...){
            t_thread_name = "crashed";
            LUNAR_LOG_FATAL(g_logger) << "exception caught in Thread " + threadName
                << "\n"
                << std::endl;

            throw;
        }

        return nullptr;
    }

}
