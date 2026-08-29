#ifndef NTP_SERVER_H
#define NTP_SERVER_H

#include <windows.h>

// 启动 NTP 服务器（后台线程），监听指定端口
// 返回线程句柄，失败返回 NULL
HANDLE StartNtpServer(unsigned short port);

// 停止 NTP 服务器
void StopNtpServer();

// 查询是否在运行
bool IsNtpServerRunning();

#endif
