# coserver



## coserver -- a support tcp/simple http netserver base on corountine(context or boost)

- Linux: 

​    1、Use make to compile and install: 

```shell
$ make
$ sudo make install

# or add debug
$ make FLAGS="-DCO_DEBUG"
# or http debug
$ make FLAGS="-DCO_LOG_HTTP_DEBUG"
# or hook mutex
$ make FLAGS="-DCO_HOOK_MUTEX"
# or hook mutex && use mutex sleep
$ make FLAGS="-DCO_HOOK_MUTEX -DCO_MUTEX_SLEEP"
#
# make install
$ sudo make install
```

​    2、Use dynamic link (put libcoserver at the front of link list)

```shell
g++ tutorial.cpp -g -std=c++11 -lcoserver -lpthread -ldl -L/usr/local/lib64 -I/usr/local/include/coserver
```



### 一、设计思路

------

- 目前请求处理流程：收到请求，解析请求，处理业务，发送响应，连接回收
- hook支持coserver内部线程的阻塞函数调用，外部线程相关阻塞不hook
- 接收完客户端请求数据后，不再监听客户端输入，但是仍需要监听异常，防止客户端断开连接等
- 子请求请求响应数据：父请求创建子请求，在完成解析子请求响应后，协议单独保存在 业务子请求状态相关结构体中，子请求使用的连接等相关信息释放
- 子请求：原始请求同时创建出多个upstream请求后，每个upstream请求事件独自出发，在出发后分别进行自己的coroutine处理，所以可以无用相互影响，每个子请求处理完毕后 在回调父请求告知父请求自己已经处理完毕
- 子请求：依赖子请求返回数据的使用upstream，不依赖子请求数据的add_upstream_detach
- 子请求层级：目前支持一层子请求，不支持子请求继续产生子请求


### 二、注意事项

------

- 编译

  > 以动态链接的方式使用时，一定要最先链接libcoserver.so，再其他库，比如libdl.so，g++ server_tutorial.cpp -std=c++11 -lcoserver -ldl [-lother_libs]

  > 不建议使用静态链接方式

- 加锁和协程切换导致的死锁问题

  > 加锁后，读写socket阻塞导致切出协程，其他协程切入后，阻塞在获取锁的代码，导致死锁问题产生
  > 默认使用方案2, 增加编译宏: CO_MUTEX_SLEEP使用方案1
  >
  > 解决方案1:
  > hook pthread_mutex_lock, 尝试pthread_mutex_trylock, 如果失败 等待一定时间后继续尝试pthread_mutex_trylock, 直到成功加锁.
  >
  > 解决方案2:
  > hook pthread_mutex_lock和pthread_mutex_unlock, 尝试pthread_mutex_trylock, 如果失败 将mutex全局存储, 然后等待一定时间后继续尝试pthread_mutex_trylock, 在此期间 如果mutex  pthread_mutex_unlock, 则直接唤醒之前最开始pthread_mutex_lock的mutex继续执行
  > 这里trylock失败是等待一段时间仍然继续尝试trylock, 是因为多线程lock和unlock容易情况较多, 很难全部捕获并全局处理(或者代价较大). 有可能线程1 trylock失败, 在全局存储mutex等待unlock之前, 线程2 unlock锁, 但是线程1还未等待就绪, 这个时候再切换回线程1 全局存储mutex成功, 等待锁unlock, 此时就产生了死锁. 

- DNS相关函数目前不支持hook+协程

- 多线程listen

  > 目前多线程监听同一socket, 使用的是SO_REUSEADDR、SO_REUSEPORT处理socket, 每个线程都将listen socket添加到epoll中, 内核调度唤醒那个epoll, 所以可能会出现线程1 max_connections耗尽, 线程2还有剩余, 但是新请求仍可能会到线程1中, 所以可以尽可能大的分配max_connections配置.

  > 一种解决方案: 一个线程达到最大max_connections时, 关闭listen socket, 等到有空闲时再listen socket.

- mutex和sleep相关函数hook

  > 为了符合函数功能, 在 mutex加锁成功前/sleep超时前, 其他错误（网络异常/处理超时）不会影响连接上的请求.

### 三、版本

------

#### v1.0

- 支持基础的网络函数hook
- 支持socket hook, sleep/mutex hook
- 支持子请求
- 支持detach请求（在coserver线程中）
- 支持多server监听配置
- 支持多线程

#### v1.1

- 协议: http支持chunked


## ToDo

1、coroutine use boost, add config coroutine

2、active request(all thread)

3、similar to http2 transport support

- 请求单独协程?
- 是否有必要，upstream有keepalive

