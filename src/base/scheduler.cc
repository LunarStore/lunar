#include"base/scheduler.h"
#include"base/hook.h"
namespace lunar{
    static Logger::ptr g_logger = LUNAR_LOG_NAME("system");
    //执行run方法的协程
    static thread_local Fiber::ptr t_shceRunFiber(nullptr);

    static thread_local Scheduler* t_scheduler = nullptr;
    Scheduler::Scheduler(uint32_t threadCount, const std::string& name, bool useCur): m_name(name), m_stop(true){
        Fiber::GetThis();

        if(useCur){
            //V1不能使用shared_from_this!!!，否则会循环引用！！！
            //V2可以用shared_from_this，bind只会增加智能指针的弱引用
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), true));

            //t_shceRunFiber = m_rootFiber;
            m_threadIds.push_back(m_rootThreadId = Thread::GetTid());
            threadCount--;
        }else{
            m_rootThreadId = -1;
        }

        m_threadCount = threadCount;
    }
    Scheduler::~Scheduler(){
        LUNAR_ASSERT(m_stop);

        //析构函数内不能调用虚函数
    }
    void Scheduler::start(){
        MutexType::Lock lock(m_mutex);
        if(m_stop == false){
            return;
        }
        m_stop = false;
        //启动线程;
        m_threadPool.resize(m_threadCount);
        for(uint32_t i = 0; 
            i < m_threadCount; 
            i++){
            m_threadPool[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + std::to_string(i)));

            m_threadIds.push_back(m_threadPool[i]->getTid());
        }
    }
    void Scheduler::stop(){

        {
            MutexType::Lock lock(m_mutex);
            if(m_stop == true){
                return;
            }
            if(m_rootThreadId == -1){
                //(useCur == false 当前线程不需要充当工作线程)stop由t_scheduler不等于this的线程调用即可。
                LUNAR_ASSERT(GetThis() != this);
            }else{
                //(useCur == true 当前线程充当一个工作线程)stop由t_scheduler等于this的线程调用，一定会调用m_rootFiber切换当前线程为工作线程。
                LUNAR_ASSERT(/*GetThis().get() == this || */GetThis() == nullptr);
            }

            m_stop = true;
        }

        for(size_t i = 0; i < m_threadCount; i++){
            tickle();
        }
        if(m_rootFiber && !isStoped()){
            tickle();
            m_rootFiber->swapIn();
        }

        std::vector<Thread::ptr> thrs;
        
        {
            MutexType::Lock lock(m_mutex);
            swap(m_threadPool, thrs);
        }


        for(size_t i = 0; i < thrs.size(); i++){
            thrs[i]->join();
        }
    }

    Scheduler::FiberAndThread Scheduler::takeTask(pid_t tId){
        FiberAndThread res;
        bool needTickle = false;

        {
            MutexType::Lock lock(m_mutex);

            for(auto it = m_tasks.begin(); it != m_tasks.end(); it++){
                if(it->m_tId == -1 || tId == it->m_tId){
                    //当前任务无线程限制 或者 有限制且当前线程号等于任务执行的线程号
                    if(it->m_cb || it->m_fiber){
                        res = *it;
                        m_tasks.erase(it);
                        needTickle = true;
                        m_activeThreadCount++;
                        break;
                    }else{
                        //任务为空;
                        m_tasks.erase(it);
                    }
                }
            }
        }
        if(needTickle){
                tickle();
        }

        return res;
    }

    void Scheduler::run(){
        //Fiber init
        pid_t curTId = -1;
        set_hook_enable(true);
        t_scheduler = this;
        if((curTId = Thread::GetTid()) != m_rootThreadId){
            t_shceRunFiber = Fiber::GetThis();
        }else{
            t_shceRunFiber = m_rootFiber;
        }
        //use this or shared_from_this
        Fiber::ptr idleFi(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cbFi(new Fiber(nullptr));
        while(true){
            //取任务
            FiberAndThread fiThr = takeTask(curTId);
            
            //当任务队列有任务时fiThr要么是协程，要么是回调，代表无任务
            if(fiThr.m_fiber && 
                (fiThr.m_fiber->getState() == Fiber::INIT ||
                fiThr.m_fiber->getState() == Fiber::HOLD || 
                fiThr.m_fiber->getState() == Fiber::READY)){
                //是协程
                // m_activeThreadCount++;
                fiThr.m_fiber->swapIn();
                m_activeThreadCount--;
                
                if(fiThr.m_fiber->getState() == Fiber::READY){
                    schedule(&fiThr.m_fiber);
                    LUNAR_ASSERT(cbFi == nullptr);
                }
            }else if(fiThr.m_cb){
                //是回调函数
                if(cbFi == nullptr ||
                    cbFi->getState() == Fiber::HOLD || 
                    cbFi->getState() == Fiber::EXEC ||
                    cbFi->getState() == Fiber::READY){
                    //旧的没运行完，需要重新分派Fiber
                    //转让所有权
                    cbFi.reset(new Fiber(fiThr.m_cb));
                }else{
                    cbFi->reset(fiThr.m_cb);
                }
                

                // m_activeThreadCount++;
                cbFi->swapIn();
                m_activeThreadCount--;

                if(cbFi->getState() == Fiber::READY){
                    schedule(&cbFi);

                    LUNAR_ASSERT(cbFi == nullptr);
                }

            }else{
                //没有任务
                m_idleThreadCount++;
                idleFi->swapIn();
                m_idleThreadCount--;

                if(idleFi->getState() == Fiber::TERM ||
                    idleFi->getState() == Fiber::EXCE){
                    //空闲处理都结束，说名协程调度器结束。
                    break;
                }
            }
        }
        
        t_scheduler = nullptr;
        set_hook_enable(false);
    }

    bool Scheduler::isStoped(){
        MutexType::Lock lock(m_mutex);
        return m_stop && m_tasks.empty() &&
            m_activeThreadCount == 0;
    }
    void Scheduler::idle(){
        while(!isStoped()){
            LUNAR_LOG_INFO(g_logger) << "idle";

            Fiber::YieldToHold();
        }
    }

    void Scheduler::tickle(){
        LUNAR_LOG_INFO(g_logger) << "tickle";
    }

    Scheduler* Scheduler::GetThis(){
        return t_scheduler;
    }

    Fiber::ptr Scheduler::GetScheRunFiber(){
        return t_shceRunFiber;
    }

    std::ostream& Scheduler::dump(std::ostream& os) {
        os << "[Scheduler name=" << m_name
        << " size=" << m_threadCount
        << " active_count=" << m_activeThreadCount
        << " idle_count=" << m_idleThreadCount
        << " stop=" << m_stop
        << " ]" << std::endl << "    ";
        for(size_t i = 0; i < m_threadIds.size(); ++i) {
            if(i) {
                os << ", ";
            }
            os << m_threadIds[i];
        }
        return os;
    }
}