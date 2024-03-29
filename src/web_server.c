#include "web_server.h"
#include "thread_pool.h"

void server() {
    ThreadPool *pool = create_thread_pool(12, 24, 500);
    int lfd;
    int opt = 1;
    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(struct sockaddr_in));
    addr_server.sin_family = AF_INET;
    addr_server.sin_port = htons(PORT);
    addr_server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 1. socket
    lfd = Socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void *) &opt, sizeof(opt));

    // 2. bind
    Bind(lfd, (struct sockaddr *) &addr_server, sizeof(addr_server));

    // 3. listen
    Listen(lfd, 64);

    // 4. epoll
    int epfd = Epoll_create(10);
    struct epoll_event ev;
    struct epoll_event evs[1024];
    ev.data.fd = lfd;
    ev.events = EPOLLIN |EPOLLET;
    Epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

    // 5. init variable
    while(1){
        int num = epoll_wait(epfd, evs, 1024, -1);
        SocketInfo * socketinfo;
        printf("num = %d\n",num);
        for (int i = 0; i < num; ++i) {
            socketinfo = (SocketInfo*)malloc(sizeof(SocketInfo));
            socketinfo->fd = evs[i].data.fd;
            socketinfo->epfd = epfd;
            if (evs[i].data.fd == lfd)
                add_task(pool, accept_conn, socketinfo);
            else
                add_task(pool, communication, socketinfo);
        }
    }
    destroy_thread_pool(pool);
}


int Socket(int __domain, int __type, int __protocol) {
    int fd = socket(__domain, __type, __protocol);
    if (fd == -1)
        sys_err("socket");
    return fd;
}

void Bind(int __fd, __CONST_SOCKADDR_ARG __addr, socklen_t __len) {
    if (bind(__fd, __addr, __len) == -1)
        sys_err("bind");
}

void Listen(int __fd, int __n) {
    if (listen(__fd, __n) == -1)
        sys_err("listen");
}

int Accept(int __fd, __SOCKADDR_ARG __addr, socklen_t *__restrict __addr_len) {
    int cfd = accept(__fd, __addr, __addr_len);
    if (cfd == -1)
        sys_err("accept");
    return cfd;
}

int Epoll_create(int __size) {
    int epfd = epoll_create(__size);
    if (epfd == -1)
        sys_err("epoll_create");
    return epfd;
}
void Epoll_ctl(int __epfd, int __op, int __fd, struct epoll_event *__event) {
    if (epoll_ctl(__epfd, __op, __fd, __event) == -1)
        sys_err("epoll_ctl");
}

void *accept_conn(void *arg) {
    SocketInfo *socketinfo = (SocketInfo *) arg;
    int cfd = Accept(socketinfo->fd, NULL, NULL);
    set_non_block(cfd);
    struct epoll_event ev;
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET |EPOLLONESHOT;
    Epoll_ctl(socketinfo->epfd, EPOLL_CTL_ADD, cfd, &ev);
    free(socketinfo);
    return NULL;
}
void *communication(void *arg){
    SocketInfo *socketinfo = (SocketInfo *) arg;
    int fd = socketinfo->fd;
    int epfd = socketinfo->epfd;
    free(socketinfo);
    int len = 0;
    char buf[512],method[10], url[256],temp[4096];;
    // 1. 读取请求行
    len = get_line(fd,buf,512);
    if(len<0){
        close(fd);
        return NULL;
    }

    //2. 解析method 和 url
    int i = 0, j = 0;
    while (!isspace(buf[j]) && i < sizeof(method) - 1)
        method[i++] = buf[j++];
    method[i] = '\0';
    while (isspace(buf[j++]));//remove  space
    i = 0;
    while (!isspace(buf[j]) && i < sizeof(url) - 1)
        url[i++] = buf[j++];
    url[i] = '\0';

    if (strncasecmp(method, "GET", i) != 0){
        write(fd,"HTTP/1.1 400 Bad Request\r\n\r\n",sizeof("HTTP/1.1 400 Bad Request\r\n\r\n"));
        close(fd);
        return NULL;
    }
    read(fd,temp,4096);

    char *pos = strchr(url, '?');//get real url
    if (pos)
        *pos = '\0';
    printf("method:%s\n",method);
    printf("url:%s\n",url);
    // 文件不存在
    struct stat stUrl;
    if (stat(url, &stUrl) == -1) {
        write(fd,"HTTP/1.1 404 Not Found\r\n\r\n",sizeof("HTTP/1.1 404 Not Found\r\n\r\n"));
        close(fd);
        return NULL;
    }

    memset(temp,0,4096);
    sprintf(temp, "HTTP/1.1 200 OK\r\n"
                       "Server:SuHeng Server\r\n"
                       "Connection: close\r\n"
                       "Content-Length: %ld\r\n", stUrl.st_size);
    const char *type;
    int modes = 0;
    type = strrchr(url, '.');
    if (strcmp(type, ".html") == 0)
        strcat(temp, "Content-Type: text/html;charset=UTF-8\r\n\r\n");
    else if (strcmp(type, ".css") == 0)
        strcat(temp, "Content-Type: text/css;charset=UTF-8\r\n\r\n");
    else if (strcmp(type, ".js") == 0)
        strcat(temp, "Content-Type: text/javascript;charset=UTF-8\r\n\r\n");
    else if (strcmp(type, ".png") == 0) {
        strcat(temp, "Content-Type: image/png\r\n\r\n");
        modes = 1;
    } else if (strcmp(type, ".jpg") == 0) {
        strcat(temp, "Content-Type: image/jpg\r\n\r\n");
        modes = 1;
    } else if (strcmp(type, ".ico") == 0) {
        strcat(temp, "Content-Type: image/x-icon\r\n\r\n");
        modes = 1;
    } else if (strcmp(type, ".gif") == 0) {
        strcat(temp, "Content-Type: image/gif\r\n\r\n");
        modes = 1;
    }

    write(fd,temp,strlen(temp));
    int filefd = open(url,O_RDWR);
    int ret = sendfile(fd,filefd,NULL,stUrl.st_size);
    close(filefd);
    close(fd);
    return NULL;



}
void set_non_block(int fd) {
    int flag = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}
void sys_err(char *msg) {
    perror(msg);
    exit(0);
}

int get_line(int sock, char *buf, int size) {
    int cnt = 0;//count
    char ch = '\0';
    int len = 0;
    while (cnt < size - 1) {
        len = read(sock, &ch, 1);
        if (len == 1) {
            if (ch == '\r')
                continue;
            else if (ch == '\n')
                break;
            buf[cnt] = ch;
            cnt++;

        } else {
            cnt = -1;
            break;
        }
    }
    if (cnt >= 0)
        buf[cnt] = '\0';
    return cnt;
}