#ifndef NETWORK_H
#define NETWORK_H

#include <windows.h>

// 从 time.windows.com 获取网络时间
// 成功返回 true 并填充 st，否则返回 false
bool GetNetworkTime(SYSTEMTIME& st);

#endif // NETWORK_H
