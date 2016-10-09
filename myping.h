/***********************************************************
 * 说明:本程序用于演示ping命令的实现原理                   *
 ***********************************************************/

#ifndef _MY_PING_H_
#define _MY_PING_H_

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

extern char send_packet_[PACKET_SIZE]; // 发送的包
extern char recv_packet_[PACKET_SIZE]; // 接收的包
extern int sockfd_ ; // 描述符
extern int data_len_; // 长度
extern int send_count_; // 发送包的数量
extern int received_count_ ; // 接收包的数量
extern struct sockaddr_in dest_addr_; // 目标地址
extern pid_t pid_; // 本进程号
extern struct sockaddr_in from_addr_; // 远程地址


void statistics(int signo);
unsigned short cal_chksum(unsigned short *addr,int len);
int pack(int pack_no);
int send_packet(void);
int recv_packet(void);
int unpack(char *buf,int len);
struct timeval tv_sub(struct timeval *out,struct timeval *in);

#endif // _MY_PING_H_
