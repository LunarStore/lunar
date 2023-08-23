#include"base/mutex.h"

#include"base/log.h"
#include "macro.h"
namespace lunar{
    static Logger::ptr g_logger = LUNAR_LOG_NAME("system");
    Semaphore::Semaphore(uint32_t count){
        if(sem_init(&m_semaphore, 0, count)){
            LUNAR_LOG_ERROR(g_logger) << "Semaphore::Semaphore(..) sem_init error : errno = " 
                << errno
                << strerror(errno);
            throw std::logic_error("sem_init error");
        }
    }
    Semaphore::~Semaphore(){
        LUNAR_ASSERT(sem_destroy(&m_semaphore) == 0 );
    }
    void Semaphore::wait(){
        int rt = 0;
        while((rt = sem_wait(&m_semaphore)) == EINTR);

        if(rt){
            LUNAR_LOG_ERROR(g_logger) << "Semaphore::wait(..) sem_wait error : errno = " 
                << errno
                << strerror(errno);
            throw std::logic_error("sem_wait error");
        }
    }
    bool Semaphore::waitForSeconds(time_t seconds){
        struct timespec timeout;
        timeout.tv_sec = time(NULL) + seconds; // 设置超时时间为当前时间加seconds秒
        timeout.tv_nsec = 0;

        int rt = sem_timedwait(&m_semaphore, &timeout); // 等待信号量，最多等待seconds秒

        return (rt == -1) && (errno == ETIMEDOUT);
    }
    void Semaphore::post(){
        if(sem_post(&m_semaphore)){
            LUNAR_LOG_ERROR(g_logger) << "Semaphore::post(..) sem_post error : errno = " 
                << errno
                << strerror(errno);
            throw std::logic_error("sem_post error");
        }
    }
}