/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qremotemonitor.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2014/06/01
**
*********************************************************************************************/

#ifndef __QREMOTEMONITOR_H_
#define __QREMOTEMONITOR_H_

#include "qglobal.h"

Q_BEGIN_NAMESPACE

class QRemoteMonitor {
	public:
		// @函数名: 初始化远程监控类
		// @参数01: 监控端口号
		// @参数02: 监控回调函数
		// @参数03: 监控回调函数的参数
		// @参数04: 是否打印监控系统日志
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(uint16_t monitor_port, int32_t (*fun_state)(void* argv), void* fun_argv, int32_t is_display_log=1)
		{
			m_monitor_port=monitor_port;
			m_fun_state=fun_state;
			m_fun_argv=fun_argv;
			m_is_display_log=is_display_log;

			if(q_init_socket()) {
				Q_DEBUG("QRemoteMonitor: socket init error!");
				return -1;
			}

			if(q_TCP_server(m_listen, m_monitor_port)) {
				Q_DEBUG("QRemoteMonitor: socket listen error!");
				return -2;
			}

			if(q_create_thread(thread_monitor, this)) {
				Q_DEBUG("QRemoteMonitor: create thread error!");
				return -3;
			}

			return 0;
		}

	private:
		// @函数名: 监控线程函数
		static Q_THREAD_T thread_monitor(void* ptr_info)
		{
			enum ErrorType {
				TYPE_OK=0,	// 运行正常
				TYPE_MONITOR,	// monitor错误
				TYPE_NETWORK,	// 网络错误
				TYPE_SERVICE,	// 服务错误
				TYPE_OTHER	// 其它错误
			};

			enum ErrorLevel {
				LEVEL_OK=0,	// 正常
				LEVEL_A,	// 严重
				LEVEL_B,	// 重要
				LEVEL_C,	// 一般
				LEVEL_D		// 可忽略
			};

			QRemoteMonitor* rm=reinterpret_cast<QRemoteMonitor*>(ptr_info);
			char buffer[1<<7]={0};

			Q_SOCKET_T client;
			char client_ip[32]={0};
			int32_t client_port;

			Q_FOREVER {
				if(q_accept_socket(rm->m_listen, client, client_ip, client_port)) {
					Q_INFO("QRemoteMonitor: accept request error!");
					continue;
				}

				if(q_set_overtime(client, 3000)) {
					Q_INFO("QRemoteMonitor: set overtime error!");
					q_close_socket(client);
					continue;
				}

				if(q_recvbuf(client, buffer, 4)) {
					Q_INFO("QRemoteMonitor: recv buffer error!");
					q_close_socket(client);
					continue;
				}

				int32_t status=rm->m_fun_state(rm->m_fun_argv);

				char* ptr=buffer;
				memcpy(ptr, "MonitorP1 ", 10);
				ptr+=10;

				*(int32_t*)ptr=0;
				ptr+=4;

				if(status==0) {
					*(int32_t*)ptr=TYPE_OK;
					ptr+=4;
					*(int32_t*)ptr=LEVEL_OK;
					ptr+=4;
				} else {
					*(int32_t*)ptr=TYPE_SERVICE;
					ptr+=4;
					*(int32_t*)ptr=LEVEL_A;
					ptr+=4;
				}

				*(int32_t*)(buffer+10)=(int32_t)(ptr-buffer-14);

				if(q_sendbuf(client, buffer, (int32_t)(ptr-buffer))) {
					Q_INFO("QRemoteMonitor: send buffer error!");
					q_close_socket(client);
					continue;
				}

				q_close_socket(client);
				if(rm->m_is_display_log)
					Q_INFO("QRemoteMonitor: [%s:%05d] monitor: service status = %d", client_ip, client_port, status);
			}

			return NULL;
		}

	private:
		uint16_t	m_monitor_port;			// 监控端口
		Q_SOCKET_T	m_listen;			// 监听描述符

		int32_t		(*m_fun_state)(void* argv);	// 监控外部函数
		void*		m_fun_argv;			// 监控外部函数参数

		int32_t		m_is_display_log;		// 是否打印监控日志
};

Q_END_NAMESPACE

#endif // __QREMOTEMONITOR_H_
