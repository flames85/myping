#include "myping.h"

int main(int argc,char *argv[])
{
    struct hostent *host;
    struct protoent *protocol;
    unsigned long inaddr = 0l;
    int size = 50 * 1024;

    if(argc < 2)
    {
        printf("usage:%s hostname/IP address\n",argv[0]);
        exit(1);
    }

    if( (protocol = getprotobyname("icmp") )==NULL)
    {
        perror("getprotobyname");
        exit(1);
    }
    /*生成使用ICMP的原始套接字,这种套接字只有root才能生成*/
    if( (sockfd_ = socket(AF_INET,SOCK_RAW, protocol->p_proto) )<0)
    {
        perror("socket error");
        exit(1);
    }
    /* 回收root权限,设置当前用户权限*/
    setuid(getuid());
    /*扩大套接字接收缓冲区到50K这样做主要为了减小接收缓冲区溢出的
      的可能性,若无意中ping一个广播地址或多播地址,将会引来大量应答*/
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size) );
    bzero(&dest_addr_,sizeof(dest_addr_));
    dest_addr_.sin_family = AF_INET;

    /*判断是主机名还是ip地址*/
    if( (inaddr = inet_addr(argv[1]) ) == INADDR_NONE)
    {
        if((host = gethostbyname(argv[1]) ) == NULL) /*是主机名*/
        {
            perror("gethostbyname error");
            exit(1);
        }
        memcpy( (char *)&dest_addr_.sin_addr, host->h_addr, host->h_length);
    }
    else /*是ip地址*/
        memcpy( (char *)&dest_addr_.sin_addr, (char *)&inaddr, sizeof(int));

    /*获取main的进程id,用于设置ICMP的标志符*/
    pid_ = getpid();
    printf("PING %s(%s): %d bytes data in ICMP packets.\n",
           argv[1],
           inet_ntoa(dest_addr_.sin_addr),
           data_len_);

    // 循环次数是发送/接收的包的数量
    for(int i = 0; i < MAX_NO_PACKETS; ++i)
    {
        if(-1 == send_packet())  /*发送1个ICMP报文*/
        {
            break;
        }
        ++send_count_;

        if(-1 == recv_packet())  /*接收1个ICMP报文*/
        {
            break;
        }
        ++received_count_;

        //  sleep(1); /*每隔一秒发送一个ICMP报文*/
    }

    statistics(SIGALRM); /*进行统计*/

    return 0;

}