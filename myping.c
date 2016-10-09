/***********************************************************
 * 说明:本程序用于演示ping命令的实现原理                   *
 ***********************************************************/
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>

#include <stdlib.h>
#include <sys/time.h>
#include <strings.h>

#define PACKET_SIZE     4096
#define MAX_WAIT_TIME   5
#define MAX_NO_PACKETS  3

char send_packet_[PACKET_SIZE]; // 发送的包
char recv_packet_[PACKET_SIZE]; // 接收的包
int sockfd_ = 0; // 描述符
int data_len_ = 56; // 长度
int send_count_ = 0; // 发送包的数量
int received_count_ = 0; // 接收包的数量
struct sockaddr_in dest_addr_; // 目标地址
pid_t pid_; // 本进程号
struct sockaddr_in from_addr_; // 远程地址


void statistics(int signo);
unsigned short cal_chksum(unsigned short *addr,int len);
int pack(int pack_no);
int send_packet(void);
int recv_packet(void);
int unpack(char *buf,int len);
struct timeval tv_sub(struct timeval *out,struct timeval *in);

void statistics(int signo)
{ 
    printf("\n--------------------PING statistics-------------------\n");
    printf("%d packets transmitted, %d received , %%%d lost\n",
            send_count_,
            received_count_,
            (send_count_-received_count_)/send_count_*100);
    close(sockfd_);
    exit(1);
}
/*校验和算法*/
unsigned short cal_chksum(unsigned short *addr,int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    /*把ICMP报头二进制数据以2字节为单位累加起来*/
    while(nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }
    /*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
    if( nleft == 1)
    { 
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += answer;
    }
    sum = (sum>>16) + (sum & 0xffff);
    sum += (sum>>16);
    answer = ~sum;
    return answer;
}

/*设置ICMP报头*/
int pack(int pack_no)
{       
    int packsize;
    struct icmp *icmp;
    struct timeval *tval;

    icmp=(struct icmp*)send_packet_;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_seq = pack_no;
    icmp->icmp_id = pid_;
    packsize = 8 + data_len_;
    tval = (struct timeval *)icmp->icmp_data;
    gettimeofday(tval, NULL);    /*记录发送时间*/
    icmp->icmp_cksum = cal_chksum( (unsigned short *)icmp,packsize); /*校验算法*/
    return packsize;
}

/*发送ICMP报文*/
int send_packet()
{    
    int packetsize;
    packetsize = pack(send_count_); /*设置ICMP报头*/
    if( sendto(sockfd_,
                send_packet_,
                packetsize,
                0,
                (struct sockaddr *)&dest_addr_,
                sizeof(dest_addr_) ) < 0)
    {
        perror("sendto error");
        return -1;
    }
    return 0;
}

/*接收所有ICMP报文*/
int recv_packet()
{       
    int n;
    extern int errno;

    signal(SIGALRM, statistics);
    unsigned int fromlen = sizeof(from_addr_);

    alarm(MAX_WAIT_TIME);

    while((n = recvfrom(sockfd_,
                        recv_packet_,
                        sizeof(recv_packet_),
                        0,
                        (struct sockaddr *)&from_addr_,
                        &fromlen) ) < 0)
    {
        if(errno == EINTR) continue;
        perror("recvfrom error");
        return -1;
    }

    if(unpack(recv_packet_, n) == -1) return -2;

    return 0;

}
/*剥去ICMP报头*/
int unpack(char *buf,int len)
{      
    int iphdrlen;
    struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;
    struct timeval tvrecv;
    double rtt;

    ip=(struct ip *)buf;
    iphdrlen=ip->ip_hl<<2;    /*求ip报头长度,即ip报头的长度标志乘4*/
    icmp=(struct icmp *)(buf+iphdrlen);  /*越过ip报头,指向ICMP报头*/
    len-=iphdrlen;            /*ICMP报头及ICMP数据报的总长度*/
    if(len < 8)                /*小于ICMP报头长度则不合理*/
    {
        printf("ICMP packets/'s length is less than 8\n");
        return -1;
    }
    /*确保所接收的是我所发的的ICMP的回应*/
    if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id == pid_) )
    {
        gettimeofday(&tvrecv, NULL);  // 获取当前时间为接收时间
        tvsend = (struct timeval *)icmp->icmp_data; // 获取包中的发送是时间
        struct timeval dt = tv_sub(&tvrecv, tvsend);  /*接收和发送的时间差*/
        rtt = dt.tv_sec*1000 + ((double)dt.tv_usec)/1000;  /*以毫秒为单位计算rtt*/
        /*显示相关信息*/
        printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
                len,
                inet_ntoa(from_addr_.sin_addr),
                icmp->icmp_seq,
                ip->ip_ttl,
                rtt);
        return 0;
    }
    else
        return -1;
}

int main(int argc,char *argv[])
{       
    struct hostent *host;
    struct protoent *protocol;
    unsigned long inaddr=0l;
    //int waittime=MAX_WAIT_TIME;
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
/*两个timeval结构相减*/
struct timeval tv_sub(struct timeval *a, struct timeval *b)
{
    struct timeval dt;
    dt.tv_sec = a->tv_sec - b->tv_sec;
    dt.tv_usec = a->tv_usec - b->tv_usec;
    if(dt.tv_usec < 0 && dt.tv_sec >= 0) // usec是负数, 而sec是非负数需要进位
    {
        --dt.tv_sec;
        dt.tv_usec += 1000000;
    }
    return dt;
}
/*------------- The End -----------*/
