/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qtcpsocket.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2012/11/03
**
*********************************************************************************************/

#ifndef __QTCPSOCKET_H_
#define __QTCPSOCKET_H_

#include "qglobal.h"

Q_BEGIN_NAMESPACE

// TCP通讯客户端
class QTcpClient {
	public:
		// @函数名: 初始化函数
		// @参数01: 服务端IP
		// @参数02: 服务端port
		// @参数03: socket超时时间
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(const char* server_ip, const uint16_t server_port, const int32_t time_out)
		{
			strcpy(m_server_ip, server_ip);
			m_server_port=server_port;
			m_time_out=time_out;

			if(q_init_socket()) {
				Q_DEBUG("QTcpClient: socket init error!");
				return -1;
			}
			return 0;
		}

		// @函数名: 连接socket服务端
		// @参数01: socket句柄
		// @参数02: 服务端IP地址
		// @参数03: 服务端port
		// @参数04: socket超时时间
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t connect(void*& handle, char* server_ip=NULL, uint16_t server_port=0, int32_t time_out=-1)
		{
			Q_SOCKET_T* socket=q_new<Q_SOCKET_T>();
			if(socket==NULL)
				return -1;

			if(server_ip)
				strcpy(m_server_ip, server_ip);
			if(server_port)
				m_server_port=server_port;
			if(time_out>=0)
				m_time_out=time_out;

			if(q_connect_socket(*socket, m_server_ip, m_server_port)) {
				Q_DEBUG("QTcpClient: connect %s:%d error!", m_server_ip, m_server_port);
				return -2;
			}
			if(q_set_overtime(*socket, m_time_out)) {
				Q_DEBUG("QTcpClient: set overtime %d error!", m_time_out);
				return -3;
			}

			handle=(void*)socket;
			return 0;
		}

		// @函数名: 数据发送函数
		// @参数01: socket连接句柄
		// @参数02: 待发送数据
		// @参数03: 待发送数据长度
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t send(void* handle, char* send_buf, int32_t send_len)
		{
			if(q_sendbuf(*(Q_SOCKET_T*)handle, send_buf, send_len))
				return -1;
			return 0;
		}

		// @函数名: 数据接收函数
		// @参数01: socket连接句柄
		// @参数02: 待接收数据
		// @参数03: 待接收数据长度
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t recv(void* handle, char* recv_buf, int32_t recv_len)
		{
			if(q_recvbuf(*(Q_SOCKET_T*)handle, recv_buf, recv_len))
				return -1;
			return 0;
		}

		// @函数名: 关闭客户端连接
		// @参数01: socket连接句柄
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t close(void* handle)
		{
			Q_SOCKET_T* socket=reinterpret_cast<Q_SOCKET_T*>(handle);
			q_close_socket(*socket);
			q_delete<Q_SOCKET_T>(socket);
			return 0;
		}

	protected:
		char m_server_ip[16];
		uint16_t m_server_port;
		int32_t m_time_out;
};

// TCP通讯服务端
class QTcpServer {
	public:
		// @函数名: 初始化TCP通讯服务端
		// @参数01: 监听端口
		// @参数02: 最大监听连接数
		// @参数03: 超时时间
		// @参数04: 通讯模式 1->单次收发 0->多次收发
		// @参数05: socket数据包head大小
		// @参数06: 最大发送数据包大小
		// @参数07: 通用接收数据包大小
		// @参数08: 最大接收数据包大小
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(uint16_t listen_port, int32_t listen_num, int32_t time_out, int32_t model_type=0, int32_t head_size=0, int32_t max_send_size=0, \
				int32_t min_recv_size=1<<10, int32_t max_recv_size=1<<10)
		{
			m_listen_port=listen_port;
			m_listen_num=listen_num;
			m_time_out=time_out;
			m_head_size=head_size;
			m_max_send_size=max_send_size;
			m_model_type=model_type;
			m_min_recv_size=min_recv_size;
			m_max_recv_size=max_recv_size;

			if(q_init_socket()) {
				Q_DEBUG("QTcpServer: socket init error!");
				return -1;
			}

			if(q_TCP_server(m_listen, m_listen_port)) {
				Q_DEBUG("QTcpServer: socket listen (%d) error!", m_listen_port);
				return -2;
			}
			return 0;
		}

		// @函数名: 启动服务
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t start()
		{
			for(int32_t i=0; i<m_listen_num; ++i) {
				m_success_flag=0;
				if(q_create_thread(ts_daemon_thread, this))
					return -1;
				while(m_success_flag==0)
					q_sleep(1);
				if(m_success_flag<0)
					return -1;
			}
			return 0;
		}

		// @函数名: 数据发送函数
		// @参数01: socket连接句柄
		// @参数02: 待发送数据
		// @参数03: 待发送数据长度
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t send(void* handle, char* send_buf, int32_t send_len)
		{
			if(q_sendbuf(*(Q_SOCKET_T*)handle, send_buf, send_len))
				return -1;
			return 0;
		}

		// @函数名: 数据接收函数
		// @参数01: socket连接句柄
		// @参数02: 待接收数据
		// @参数03: 待接收数据长度
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t recv(void* handle, char* recv_buf, int32_t recv_len)
		{
			if(q_recvbuf(*(Q_SOCKET_T*)handle, recv_buf, recv_len))
				return -1;
			return 0;
		}

		// @函数名: 关闭客户端连接
		// @参数01: socket连接句柄
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t close(void* handle)
		{
			q_close_socket(*(Q_SOCKET_T*)handle);
			return 0;
		}

		// @函数名: 继承类必须实现的包头解析函数
		virtual int32_t ts_fun_package(const char* in_buf, int32_t in_buf_size, const void* handle=NULL)=0;

		// @函数名: 继承类必须实现的协议处理函数
		virtual int32_t ts_fun_process(const char* in_buf, int32_t in_buf_size, const char* out_buf, int32_t out_buf_size, const void* handle=NULL)=0;

	private:
		static Q_THREAD_T ts_daemon_thread(void* argv)
		{
			QTcpServer* ts=static_cast<QTcpServer*>(argv);

			char* recv_buf=NULL;
			int32_t recv_len=0;
			char* send_buf=NULL;
			int32_t send_len=0;
			char* temp_buf=NULL;

			if(ts->m_model_type==1) {
				recv_buf=new(std::nothrow) char[ts->m_min_recv_size];
				if(recv_buf==NULL) {
					ts->m_success_flag=-1;
					return NULL;
				}
				if(ts->m_max_send_size>0) {
					send_buf=new(std::nothrow) char[ts->m_max_send_size];
					if(send_buf==NULL) {
						ts->m_success_flag=-1;
						return NULL;
					}
				}
			}

			ts->m_success_flag=1;

			Q_FOREVER {
				Q_SOCKET_T socket;
				char client_ip[32];
				int32_t client_port;

				if(q_accept_socket(ts->m_listen, socket, client_ip, client_port)) {
					q_sleep(1);
					continue;
				}

				if(q_set_overtime(socket, ts->m_time_out)) {
					q_close_socket(socket);
					Q_DEBUG("QTcpServer: set time out (%d) error!", ts->m_time_out);
					continue;
				}

				if(ts->m_model_type==1) {
					if(temp_buf) {
						if(recv_buf) delete recv_buf;
						recv_buf=temp_buf;
						temp_buf=NULL;
					}

					if(q_recvbuf(socket, recv_buf, ts->m_head_size)) {
						q_close_socket(socket);
						Q_DEBUG("QTcpServer: recv head error, size = (%d)", ts->m_head_size);
						continue;
					}

					recv_len=ts->ts_fun_package(recv_buf, ts->m_head_size);
					if(recv_len<0) {
						q_close_socket(socket);
						Q_DEBUG("QTcpServer: ts_fun_package error, code = (%d)", recv_len);
						continue;
					}

					if(recv_len>0) {
						if(recv_len>ts->m_min_recv_size) {
							if(recv_len>ts->m_max_recv_size) {
								q_close_socket(socket);
								Q_DEBUG("QTcpServer: recv length(%d) > max recv length(%d)", recv_len, ts->m_max_recv_size);
								continue;
							}

							temp_buf=recv_buf;
							recv_buf=new(std::nothrow) char[recv_len];
							if(recv_buf==NULL) {
								q_close_socket(socket);
								Q_DEBUG("QTcpServer: alloc recv memory error, size = (%d)", recv_len);
								continue;
							}
						}

						if(q_recvbuf(socket, recv_buf, recv_len)) {
							q_close_socket(socket);
							Q_DEBUG("QTcpServer: recv body error, size = (%d)", recv_len);
							continue;
						}
					}

					send_len=ts->ts_fun_process(recv_buf, recv_len, send_buf, ts->m_max_send_size);
					if(send_len<0) {
						q_close_socket(socket);
						Q_DEBUG("QTcpServer: ts_fun_process error, code = (%d)", send_len);
						continue;
					}
					if(send_len>0) {
						if(q_sendbuf(socket, send_buf, send_len)) {
							q_close_socket(socket);
							Q_DEBUG("QTcpServer: send buffer error, size = (%d)", send_len);
							continue;
						}
					}
				} else {
					ts->ts_fun_process(recv_buf, recv_len, send_buf, send_len, (void*)&socket);
				}
				
				q_close_socket(socket);
			}

			return 0;
		}

	protected:
		// 服务端监听端口号
		uint16_t m_listen_port;
		// 监听线程数
		int32_t m_listen_num;
		// 连接超时时间(毫秒)
		int32_t m_time_out;
		// 通讯模式 1->单次收发 0->多次收发
		int32_t m_model_type;
		// 协议头长度
		int32_t m_head_size;
		// 最大发送长度
		int32_t m_max_send_size;
		// 通用接收长度
		int32_t m_min_recv_size;
		// 最大接收长度
		int32_t m_max_recv_size;

		// 监听描述符
		Q_SOCKET_T m_listen;

		// 状态标识符
		int32_t m_success_flag;
};

Q_END_NAMESPACE

#endif // __QTCPSOCKET_H_
