# 简介
Linux下C语言编写的静态Web服务器
- 使用线程池降低资源消耗，提高相应速度，提高线程可管理性。
- 使用非阻塞socket+epoll(ET模式)+事件处理(Reactor)并发模型
- 使用异步线程池日志系统，记录线程池的运行状态。
- 使用webbench测试，可实现上万的并发连接数据交换


counted with cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                                3             51             35            446
C/C++ Header                     2             49            130             81
Markdown                         1              6              0             13
-------------------------------------------------------------------------------
SUM:                             6            106            165            540
-------------------------------------------------------------------------------

# TODO
- [x] 线程池
- [x] 线程池日志系统
- [ ] 服务器日志
- [ ] 服务器配置解析
- [ ] 服务器命令读取
- [ ] 后台服务权限提升

