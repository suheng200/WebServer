#include "thread_pool.h"

int exit_num = 2;
ThreadPool *create_thread_pool(int min, int max, int queue_capacity) {
    ThreadPool *pool = (ThreadPool *) malloc(sizeof(ThreadPool));
    do {
        if (pool == NULL) {
            printf("malloc pool error\n");
            break;
        }
        int ret = 0;
        exit_num = (int) (max-min)*EXIT_NUM_RATE;
        // 1.初始化锁
        pthread_mutex_init(&pool->mutexPool, NULL);
        pthread_mutex_init(&pool->mutexBusy, NULL);
        pthread_cond_init(&pool->notEmpty, NULL);
        pthread_cond_init(&pool->notFull, NULL);

        // 2. 初始化任务队列
        pool->queueCapacity = queue_capacity;
        pool->queueRear = 0;
        pool->queueFront = 0;
        pool->taskQ = (Task *) malloc(sizeof(Task) * queue_capacity);
        if (pool->taskQ == NULL) {
            printf("malloc taskQ error\n");
            break;
        }
        memset(pool->taskQ, 0, sizeof(Task) * queue_capacity);


        // 3. 初始化线程池的一些参数
        pool->shutdown = 0;
        pool->minNum = min;
        pool->maxNum = max;
        pool->liveNum = min;
        pool->busyNum = 0;
        pool->exitNum = 0;
        pool->threadIDs = (pthread_t *) malloc(sizeof(pthread_t) * max);
        if (pool->threadIDs == NULL) {
            printf("malloc threadIDs error\n");
            break;
        }

        // 4. 初始化管理线程
        memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
        ret = pthread_create(&pool->managerID, NULL, manager, pool);
        if (ret != 0) {
            printf("%s\n", strerror(ret));
            break;
        }
        ret = pthread_detach(pool->managerID);
        if (ret != 0) {
            printf("%s\n", strerror(ret));
            break;
        }

        // 5. 初始化任务线程
        for (int i = 0; i < min; i++) {
            pthread_create(&pool->threadIDs[i], NULL, worker, pool);
            pthread_detach(pool->threadIDs[i]);
        }
        // 6.初始化日志
#if LOG
        pthread_create(&pool->logID, NULL, log_thread, pool);
#endif
        return pool;
    } while (0);
    // 7.初始化失败，释放资源
    if (pool && pool->threadIDs) free(pool->threadIDs);
    if (pool && pool->taskQ) free(pool->taskQ);
    if (pool) {
        pthread_mutex_destroy(&pool->mutexPool);
        pthread_mutex_destroy(&pool->mutexBusy);
        pthread_cond_destroy(&pool->notEmpty);
        pthread_cond_destroy(&pool->notFull);
        free(pool);
    }
    return NULL;
}

void *worker(void *arg) {
    ThreadPool *pool = (ThreadPool *) arg;
    Task task;
    while (1) {
        pthread_mutex_lock(&pool->mutexPool);
        // 1.等待条件获取新任务
        while (pool->queueSize == 0 && !pool->shutdown) {
            //wait produce task,
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
            if (pool->exitNum > 0) {
                pool->exitNum--;
                if (pool->liveNum > pool->minNum) {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    thread_exit(pool);
                }
            }
        }
        //skip this loop will have two results：have new task or shutdown
        // 2. is shutdown?
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->mutexPool);
            thread_exit(pool);
        }

        // 3. get task and notify producer
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        // 4. working
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.function(task.arg);
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pool->completedTaskNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}

void *manager(void *arg) {
    ThreadPool *pool = (ThreadPool *) arg;
    while (!pool->shutdown) // !pool->shutdown
    {
        sleep(MANAGER_SLEEP_TIME);
        // 1. get thread status
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);
        // 2. add thread
        //task > live && live < max
        if (queueSize > liveNum && liveNum < pool->maxNum) {
            int counter = 0;
            pthread_mutex_lock(&pool->mutexPool);
            for (int i = 0; i < pool->maxNum && counter < exit_num && pool->liveNum < pool->maxNum; i++) {
                if (pool->threadIDs[i] == 0) {
                    counter++;
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    pthread_detach(pool->threadIDs[i]);
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }
        // 3.destroy thread
        //busy*2 <live && live > min
        if (busyNum * 2 < liveNum && liveNum > pool->minNum) {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = exit_num;
            pthread_mutex_unlock(&pool->mutexPool);
            for (int i = 0; i < exit_num; ++i) {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    pthread_exit(NULL);
}

void thread_exit(ThreadPool *pool) {
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->maxNum; ++i) {
        if (pool->threadIDs[i] == tid) {
            pool->threadIDs[i] = 0;
            break;
        }
    }
    pthread_exit(NULL);
}

//return  0 :success   -1:thread pool is shutdown
int add_task(ThreadPool *pool, void *(*function)(void *), void *arg) {
    pthread_mutex_lock(&pool->mutexPool);
    while (pool->queueSize == pool->queueCapacity && !pool->shutdown) {
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&pool->mutexPool);
        return -1;
    }
    pool->taskQ[pool->queueRear].function = function;
    pool->taskQ[pool->queueRear].arg = arg;
    pool->queueSize++;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pthread_cond_signal(&pool->notEmpty);
    pthread_mutex_unlock(&pool->mutexPool);
    return 0;
}

int destroy_thread_pool(ThreadPool *pool) {
    if (!pool)
        return -1;
    // 1. 等待任务队列完成任务
    printf("Thread pool waiting to exit...\n");
    while (1) {
        pthread_mutex_lock(&pool->mutexPool);
        if (pool->queueSize == 0) {
            pthread_mutex_unlock(&pool->mutexPool);
            break;
        }
        pthread_mutex_unlock(&pool->mutexPool);

        usleep(5000);
    }
    pool->shutdown = 1;

    printf("All tasks completed...\n");

    // 2. 等待所有线程退出
    while (1) {
        pthread_cond_signal(&pool->notEmpty);
        usleep(5000);
        int flag = 1;
        for (int i = 0; i < pool->maxNum; i++) {
            if (pool->threadIDs[i] != 0) {
                flag = 0;
                break;
            }
        }
        if (flag)
            break;
    }
    printf("All work thread exit...\n");
    pthread_join(pool->logID, NULL);
    if (pool->taskQ) free(pool->taskQ);
    if (pool->threadIDs) free(pool->threadIDs);
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);
    free(pool);
    printf("Thread pool is destroyed...\n");
    return 0;
}


void *log_thread(void *arg) {
    ThreadPool *pool = (ThreadPool *) arg;

    //1.检查文件夹是否存在，不存在就创建
    DIR *dir = NULL;
    dir = opendir(LOG_PATH);
    if (dir == NULL) {
        mode_t mode = umask(0000);
        mkdir(LOG_PATH, 0777);
        umask(mode);
    } else
        closedir(dir);

    //2.获取时间信息，生成文件名
    char logName[50] = {"threadpool-"};
    char time[20] = {0}; //time的长度是19
    get_current_time(time);
    sprintf(logName, "%sthreadpool-%s.log", LOG_PATH, time);
    //3.创建并打开文件
    int logFd = open(logName, O_CREAT | O_RDWR | O_TRUNC, 0777);
    if (logFd == -1) {
        perror("open log file error");
        return NULL;
    }

    // 4.定时写入日志
    int queueCapacity = pool->queueCapacity;
    int minNum = pool->minNum;
    int maxNum = pool->maxNum;
    int queueSize;
    int liveNum;
    int busyNum;
    u_int64_t completedTaskNum;
    char writeBuff[200] = {0};
    while (!pool->shutdown) {
        sleep(LOG_SLEEP_TIME);
        pthread_mutex_lock(&pool->mutexPool);
        queueSize = pool->queueSize;
        liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);
        pthread_mutex_lock(&pool->mutexBusy);
        busyNum = pool->busyNum;
        completedTaskNum = pool->completedTaskNum;
        pthread_mutex_unlock(&pool->mutexBusy);
        memset(time, 0, 20);
        memset(writeBuff, 0, 200),
                get_current_time(time);
        //[2023-08-20-19-20-60] min:10,max:20,working:15,living:20,task:86,capacity:100,completedTaskNum:99102.
        sprintf(writeBuff, "[%s]  min:%d, max:%d, working:%d, living:%d, task:%d, capacity:%d, completedTaskNum:%lu.\n",
                time, minNum, maxNum, busyNum, liveNum, queueSize, queueCapacity, completedTaskNum);
        write(logFd, writeBuff, strlen(writeBuff));
        //write(STDOUT_FILENO, writeBuff, strlen(writeBuff));
    }
    close(logFd);
    pthread_exit(NULL);
}

void get_current_time(char *str) {
    time_t currentTime;
    struct tm *timeInfo;
    time(&currentTime); // 获取当前时间
    timeInfo = localtime(&currentTime); // 将时间转换为本地时间
    sprintf(str, "%02d-%02d-%02d-%02d-%02d-%02d", timeInfo->tm_year + 1900, timeInfo->tm_mon + 1,
            timeInfo->tm_mday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
}





