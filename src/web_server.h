/**
 *  @file       WebServer.h
 *  @author     suheng   tianly2024@163.com
 *  @version    1.0
 * */
#ifndef SUHENG_WEBSERVER_H
#define SUHENG_WEBSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/sendfile.h>

#define PORT  9999
#define SERVER_IP "127.0.0.1"
/**
 * @brief       combine epoll fd and sock fd
 */
typedef struct socketInfo {
    int fd;         ///< sock fd
    int epfd;       ///< epoll fd
} SocketInfo;

/**
 * @brief       main function
 */
void server();

/**
 * @brief       print error information in thread
 * @param msg   error information
 */
void sys_err(char *msg);

/**
 * @brief       Rewrite function to handle abnormal situations
 * @param       __domain
 * @param       __type
 * @param       __protocol
 * @return      sockfd
 */
int Socket(int __domain, int __type, int __protocol);

/**
 * @brief       Rewrite function to handle abnormal situations
 * @param       __fd
 * @param       __addr
 * @param       __len
 */
void Bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len);

/**
 * @brief       Rewrite function to handle abnormal situations
 * @param       __fd
 * @param       __n
 */
void Listen(int __fd, int __n);

/**
 * @brief       Rewrite function to handle abnormal situations
 * @param       __fd
 * @param       __addr
 * @param       __addr_len
 * @return      sockfd
 */
int Accept(int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len);

/**
 * @brief       Rewrite function to handle abnormal situations
 * @param       __size
 * @return      epfd
 */
int Epoll_create(int __size);

/**
 * @brief       Rewrite function to handle abnormal situations
 * @param       __epfd
 * @param       __op
 * @param       __fd
 * @param       __event
 */
void Epoll_ctl(int __epfd, int __op, int __fd, struct epoll_event *__event);

/**
 * @brief       this function is used to handle the thread of the connection
 * @details     accept connection,set to non blocking and add to epoll tree
 * @param       arg arg will be convent to a variable of type Socketinfo
 */
void *accept_conn(void *arg);


/**
 * @brief       this function is used to process thread connection requests
 * @param       arg arg will be convent to a variable of type Socketinfo
 */
void *communication(void *arg);

/**
 * @brief       set fd as a non blocking file descriptor
 * @param       fd will be set non blocking
 */
void set_non_block(int fd);

/**
 * @brief       get one line from sock read buff
 * @param       sock will be get one line from this fd buff
 * @param       buf store data
 * @param       size buff size
 * @return      read size ,if error return -1
 */
int get_line(int sock, char *buf, int size);

#endif