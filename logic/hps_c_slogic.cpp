//===--------------------------- logic/hps_c_slogic.cpp - [HP-Server] -----------------------------------*- C++ -*-===//
// brief :
//
//
// author: YongDu
// date  : 2021-09-23
//===--------------------------------------------------------------------------------------------------------------===//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "hps_c_conf.h"
#include "hps_macro.h"
#include "hps_global.h"
#include "hps_func.h"
#include "hps_c_memory.h"
#include "hps_c_crc32.h"
#include "hps_c_slogic.h"
#include "hps_logiccomm.h"

// 成员函数指针
typedef bool (CLogicSocket::*handler)(lphps_connection_t pConn,      // 连接池中连接
                                      LPSTRUC_MSG_HEADER pMsgHeader, // 消息头
                                      char *             pPkgBody,   // 包体
                                      unsigned short     iBodyLength);   // 包体长度

// 保存 成员函数指针 的数组
static const handler statusHandler[] = {
    // 前5个元素保留，以备将来增加一些基本服务器功能
    NULL, // 【0】
    NULL, // 【1】
    NULL, // 【2】
    NULL, // 【3】
    NULL, // 【4】

    // 具体的业务逻辑
    &CLogicSocket::_HandleRegister, // 【5】 注册功能
    &CLogicSocket::_HandleLogIn,    // 【6】 登录功能
                                    // ... 待扩展

};

#define AUTH_TOTAL_COMMANDS sizeof(statusHandler) / sizeof(handler) // 所有逻辑功能处理个数

CLogicSocket::CLogicSocket() {}

CLogicSocket::~CLogicSocket() {}

bool CLogicSocket::Initialize() {
  bool bParentInit = CSocekt::Initialize();
  return bParentInit;
}

// 处理收到的数据包
void CLogicSocket::threadRecvProcFunc(char *pMsgBuf) {
  LPSTRUC_MSG_HEADER pMsgHeader = (LPSTRUC_MSG_HEADER)pMsgBuf;
  LPCOMM_PKG_HEADER  pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuf + m_iLenMsgHeader);
  void *             pPkgBody = NULL;
  unsigned short     pkglen = ntohs(pPkgHeader->pkgLen); // 客户端指明的包宽度"包头+包体"

  if (m_iLenPkgHeader == pkglen) {
    // 只有包头
    if (pPkgHeader->crc32 != 0) {
      // 包头的 crc = 0
      return;
    }
    pPkgBody = NULL;
  } else {
    pPkgHeader->crc32 = ntohl(pPkgHeader->crc32);
    pPkgBody = (void *)(pMsgBuf + m_iLenMsgHeader + m_iLenPkgHeader);

    // 纯包体 CRC 检验
    int calccrc = CCRC32::GetInstance()->Get_CRC((unsigned char *)pPkgBody, pkglen - m_iLenPkgHeader);
    if (calccrc != pPkgHeader->crc32) {
      hps_log_stderr(0, "CLogicSocket::threadRecvProcFunc()中CRC错误，丢弃数据!");
      return;
    }
  }

  unsigned short     imsgCode = ntohs(pPkgHeader->msgCode);
  lphps_connection_t p_Conn = pMsgHeader->pConn;

  // 客户端断开，过滤过期包
  if (p_Conn->iCurrsequence != pMsgHeader->iCurrsequence) {
    return;
  }

  // 过滤恶意包
  if (imsgCode >= AUTH_TOTAL_COMMANDS) {
    hps_log_stderr(0, "CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码不对!", imsgCode);
    return;
  }
  if (statusHandler[imsgCode] == NULL) {
    hps_log_stderr(0, "CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码找不到对应的处理函数!", imsgCode);
    return;
  }

  (this->*statusHandler[imsgCode])(p_Conn, pMsgHeader, (char *)pPkgBody, pkglen - m_iLenPkgHeader);
  return;
}

bool CLogicSocket::_HandleRegister(lphps_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody,
                                   unsigned short iBodyLength) {
  hps_log_stderr(0, "执行了CLogicSocket::_HandleRegister()!");
  return true;
}
bool CLogicSocket::_HandleLogIn(lphps_connection_t pConn, LPSTRUC_MSG_HEADER pMsgHeader, char *pPkgBody,
                                unsigned short iBodyLength) {
  hps_log_stderr(0, "执行了CLogicSocket::_HandleLogIn()!");
  return true;
}
