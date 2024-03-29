/**
 *  @file       threadpool.h
 *  @author     suheng   tianly2024@163.com
 *  @version    1.0
 * */
#ifndef SUHENG_THREAD_POOL_H
#define SUHENG_THREAD_POOL_H
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define LOG 1
#define LOG_PATH "../log/"
#define MANAGER_SLEEP_TIME 2
#define LOG_SLEEP_TIME 2
# define EXIT_NUM_RATE 0.2

/**
 *  @brief function and arg
 */
typedef struct Task {
    void *(*function)(void *arg);
    void *arg;
} Task;


/**
 *  @brief control all variables in the thread pool
 */
typedef struct ThreadPool {
    Task *taskQ;            ///< task queue
    int queueCapacity;      ///< capacity of queue
    int queueSize;          ///< number of task in queue
    int queueFront;         ///< head pointer
    int queueRear;          ///< rear pointer

    int minNum;             ///< minimum number of threads in the thread pool
    int maxNum;             ///< maximum number of threads in the thread pool
    int liveNum;            ///< number of surviving threads in the thread pool
    int busyNum;            ///< number of working threads in the thread pool
    int exitNum;            ///< number of threads that need destroyed
    pthread_t *threadIDs;   ///< working thread IDs
    pthread_t managerID;    ///< manager thread id
    pthread_mutex_t mutexPool;  ///< used to lock the entire thread pool
    pthread_mutex_t mutexBusy;  ///< used to lock busyNum
    pthread_cond_t notFull;     ///< used to block adding task
    pthread_cond_t notEmpty;    ///< used to block getting task
    int shutdown;               ///< close thread pool
    pthread_t logID;            ///< log thread id
    u_int64_t completedTaskNum; ///< total task count
} ThreadPool;


/**
 * @brief           create a thread pool
 * @param           min minimum number of threads in the thread pool
 * @param           max maximum number of threads in the thread pool
 * @param           queue_capacity capacity of queue
 * @retval          NULL    error occurred
 * @retval          else  success
 * */
ThreadPool *create_thread_pool(int min, int max, int queue_capacity);

/**
 * @brief           add a task to the thread pool
 * @param           pool a pointer of type ThreadPool,add task to this pool
 * @param           function functions to be executed
 * @param           arg parameter of function
 * @retval          0 success
 * @retval          -1  thread pool is shutdown
 * */
int add_task(ThreadPool *pool, void *(*function)(void *), void *arg);

/**
 * @brief           work thread,continuously get task from task queue
 * @param           arg actually,arg will be converted to pool of type ThreadPool
 * @retval          void
 * */
void *worker(void *arg);

/**
 * @brief           manager thread,dynamically creating or releasing threads
 * @param           arg actually,arg will be converted to pool of type ThreadPool
 * @retval          void
 * */
void *manager(void *arg);

/**
 * @brief           when a thread call this function,the thread will be released
 * @param           pool a pointer of type ThreadPool,get thread ID from pool
 * @retval          void
 * */
void thread_exit(ThreadPool *pool);


/**
 * @brief           write logs
 * @param           pool a pointer of type ThreadPool
 * @retval          void
 * */
void *log_thread(void *arg);


/**
 * @brief           get current time,2023-07-01-19-15-00
 * @param[out]      str  write time to str.
 * @retval          void
 * */
void get_current_time(char *str);

/**
 * @brief           destroy thread pool,releasing memory
 * @param           pool will be released pool
 * @retval          0 success
 * @retval          -1 pool is NULL
 * */
int destroy_thread_pool(ThreadPool *pool);

#endif
















