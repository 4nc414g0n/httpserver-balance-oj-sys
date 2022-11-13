#include <asm-generic/socket.h>
#include <cstdlib>
#include <iostream>
using std::cout;
using std::endl;

#include <cstring> //htos htol...
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h> //socket

#include <arpa/inet.h>
#include <netinet/in.h> //addr family

#include <pthread.h>
#include <fcntl.h>//non-block

#include "Log.hpp"

#define PORT 8081
#define BACKLOG 5 // 指定最多允许多少个客户连接到服务器

// Singleton(Lazy) TcpServer
class TcpServer {
private:
  int _port;
  int _listen_sock;
  static TcpServer *TcpServerInstance;
  

private:
  TcpServer(int port) : _port(port), _listen_sock(-1) {}
  TcpServer(const TcpServer &copy) = delete;
  TcpServer &operator=(const TcpServer &lvalue) = delete;

public:
  static TcpServer *GetInstance(int port=PORT) {
    static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    if (TcpServerInstance == nullptr) {
      pthread_mutex_lock(&mtx);
      if (TcpServerInstance == nullptr) {
        TcpServerInstance = new TcpServer(port);
        TcpServerInstance->Initserver(); // 创建单例后启动服务
      }
      pthread_mutex_unlock(&mtx);
    }
    return TcpServerInstance;
  }

  void Initserver() {
    Socket();
    Bind();
    Listen();
  }

  void Socket() {
    _listen_sock =
        socket(AF_INET, SOCK_STREAM, 0); //(AF_INET:IPv4, SOCK_STREAM:TCP, 0:一般指定默认协议)
    if (_listen_sock < 0)
    {
      LOG(LOG_LEVEL_FATAL, "TcpServer: socket get failed ...");
      exit(1);
    }
    
    int opt = 1;
    setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt,
               sizeof(opt)); // socket地址复用(server挂了立即重启)
    LOG(LOG_LEVEL_INFO, "TcpServer: socket get success ...");
  }

  void Bind() {

    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(_port); // 主机字节顺序转换为网络字节顺序
    local.sin_addr.s_addr = INADDR_ANY; // 云服务器不能直接绑定公网IP

    if (bind(_listen_sock, ((struct sockaddr*)&local), sizeof(local)) < 0){
      LOG(LOG_LEVEL_FATAL, "TcpServer: bind failed ...");

      // 注意强转一下（sockaddr通常作为"模板"，一般的编程中并不直接对此数据结构进行操作，而使用另一个与之等价的数据结构sockaddr_in）
      exit(2);
    }
    LOG(LOG_LEVEL_INFO, "TcpServer: bild success ...");

  }

  void Listen() {
    if (listen(_listen_sock, BACKLOG) < 0){
      LOG(LOG_LEVEL_FATAL, "TcpServer: listen failed ...");
      exit(3);
    }
    LOG(LOG_LEVEL_INFO, "TcpServer: listen success ...");
  }

  void SetNonBlock(ssize_t sock)//设置sock为非阻塞
  {
      int fl = fcntl(sock, F_GETFL);
      fcntl(sock, F_SETFL, fl|O_NONBLOCK);
  }
  int Sock()
  {
    return _listen_sock;
  }
  ~TcpServer() 
  {
    if(_listen_sock>=0)
      close(_listen_sock);
  }
};
// 初始化TcpServerInstance
TcpServer* TcpServer::TcpServerInstance = nullptr;


