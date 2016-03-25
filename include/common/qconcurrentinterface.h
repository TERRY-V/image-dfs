/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qconcurrentinterface.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2014/06/01
**
*********************************************************************************************/

#ifndef __QCONCURRENTINTERFACE_H_
#define __QCONCURRENTINTERFACE_H_

#include "qglobal.h"
#include "qqueue.h"

Q_BEGIN_NAMESPACE

// 一个简单的并发同步阻塞服务器框架
class QConcurrentInterface {
	public:
		// @函数名: 高并发同步阻塞服务器构造函数
		QConcurrentInterface()
		{}

		// @函数名: 析构函数
		virtual ~QConcurrentInterface()
		{}

		// @函数名: 初始化通讯服务端
		// @参数01: 工作线程数
		// @参数02: 队列大小
		// @参数01: 监听端口
		// @参数02: 最大监听连接数
		// @参数03: 超时时间
		// @参数04: 通讯模式 1->单次收发 0->多次收发
		// @参数05: socket数据包head大小
		// @参数06: 最大发送数据包大小
		// @参数07: 通用接收数据包大小
		// @参数08: 最大接收数据包大小
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(int32_t worker_num, int32_t queue_size, uint32_t listen_port, int32_t listen_num, int32_t time_out, \
				int32_t model_type=0, \
				int32_t head_size=0, \
				int32_t max_send_size=0, \
				int32_t min_recv_size=1<<20, \
				int32_t max_recv_size=10<<20)
		{
			m_worker_num=worker_num;
			m_queue_size=queue_size;

			m_listen_port=listen_port;
			m_listen_num=listen_num;
			m_time_out=time_out;

			m_model_type=model_type;
			m_min_recv_size=min_recv_size;
			m_max_recv_size=max_recv_size;

			if(q_init_socket()) {
				Q_DEBUG("QConcurrentInterface: init socket error!");
				return -1;
			}

			if(q_TCP_server(m_listen, m_listen_port)) {
				Q_DEBUG("QConcurrentInterface: listen (%d) error!", m_listen_port);
				return -2;
			}

			if(m_queue.init(m_queue_size)) {
				Q_DEBUG("QConcurrentInterface: init queue error, size = (%d)", m_queue_size);
				return -3;
			}
			return 0;
		}

		// @函数名: 启动服务
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t start()
		{
			// 创建工作线程
			for(int32_t i=0; i<m_worker_num; ++i) {
				m_success_flag=0;

				if(q_create_thread(cm_worker_thread, this))
					return -1;

				while(m_success_flag==0)
					q_sleep(1);

				if(m_success_flag<0)
					return -2;
			}

			// 创建通讯接收线程
			for(int32_t j=0; j<m_listen_num; ++j) {
				m_success_flag=0;

				if(q_create_thread(cm_accept_thread, this))
					return -3;

				while(m_success_flag==0)
					q_sleep(1);

				if(m_success_flag<0)
					return -4;
			}

			// 持久化该服务
			q_serve_forever();

			return 0;
		}

		// @函数名: 继承类必须实现的包头解析函数
		virtual int32_t cm_fun_package(const char* in_buf, int32_t in_buf_size, const void* handle=NULL)=0;

		// @函数名: 继承类必须实现的协议处理函数
		virtual int32_t cm_fun_process(const char* in_buf, int32_t in_buf_size, const char* out_buf, int32_t out_buf_size, const void* handle=NULL)=0;

	private:
		// @函数名: 工作线程
		static Q_THREAD_T cm_worker_thread(void* argv)
		{
			QConcurrentInterface* rm=static_cast<QConcurrentInterface*>(argv);

			char* recv_buf=NULL;
			int32_t recv_len=0;
			char* send_buf=NULL;
			int32_t send_len=0;
			char* temp_buf=NULL;

			if(rm->m_model_type==1) {
				recv_buf=q_new_array<char>(rm->m_min_recv_size);
				if(recv_buf==NULL) {
					rm->m_success_flag=-1;
					return NULL;
				}

				if(rm->m_max_send_size>0) {
					send_buf=q_new_array<char>(rm->m_max_send_size);
					if(send_buf==NULL) {
						rm->m_success_flag=-1;
						return NULL;
					}
				}
			}

			rm->m_success_flag=1;

			Q_FOREVER {
				Q_SOCKET_T client_socket=rm->m_queue.pop();

				if(rm->m_model_type==1) {
					if(temp_buf) {
						if(recv_buf) q_delete_array<char>(recv_buf);
						recv_buf=temp_buf;
						temp_buf=NULL;
					}

					if(q_recvbuf(client_socket, recv_buf, rm->m_head_size)) {
						q_close_socket(client_socket);
						Q_DEBUG("QConcurrentInterface: recv head error, size = (%d)", rm->m_head_size);
						continue;
					}

					recv_len=rm->cm_fun_package(recv_buf, rm->m_head_size);
					if(recv_len<0) {
						q_close_socket(client_socket);
						Q_DEBUG("QConcurrentInterface: cm_fun_package error, code = (%d)", recv_len);
						continue;
					}

					if(recv_len>0) {
						if(recv_len>rm->m_min_recv_size) {
							if(recv_len>rm->m_max_recv_size) {
								q_close_socket(client_socket);
								Q_DEBUG("QConcurrentInterface: recv length(%d) > max recv length(%d)", recv_len, rm->m_max_recv_size);
								continue;
							}

							temp_buf=recv_buf;
							recv_buf=new char[recv_len];
							if(recv_buf==NULL) {
								q_close_socket(client_socket);
								Q_DEBUG("QConcurrentInterface: alloc recv memory error, size = (%d)", recv_len);
								continue;
							}
						}

						if(q_recvbuf(client_socket, recv_buf, recv_len)) {
							q_close_socket(client_socket);
							Q_DEBUG("QConcurrentInterface: recv body error, size = (%d)", recv_len);
							continue;
						}
					}

					send_len=rm->cm_fun_process(recv_buf, recv_len, send_buf, rm->m_max_send_size);
					if(send_len<0) {
						q_close_socket(client_socket);
						Q_DEBUG("QConcurrentInterface: cm_fun_process error, code = (%d)", send_len);
						continue;
					}
					if(send_len>0) {
						if(q_sendbuf(client_socket, send_buf, send_len)) {
							q_close_socket(client_socket);
							Q_DEBUG("QConcurrentInterface: send buffer error, size = (%d)", send_len);
							continue;
						}
					}
				} else {
					rm->cm_fun_process(recv_buf, recv_len, send_buf, send_len, (void*)&client_socket);
				}

				q_close_socket(client_socket);
			}

			q_delete_array<char>(recv_buf);
			q_delete_array<char>(send_buf);

			return 0;
		}

		// @函数名: 通信线程
		static Q_THREAD_T cm_accept_thread(void* argv)
		{
			QConcurrentInterface* rm=static_cast<QConcurrentInterface*>(argv);
			rm->m_success_flag=1;

			Q_SOCKET_T client_socket;
			char client_ip[32];
			int32_t client_port;

			Q_FOREVER {
				if(q_accept_socket(rm->m_listen, client_socket, client_ip, client_port)) {
					q_sleep(1);
					continue;
				}

				if(q_set_overtime(client_socket, rm->m_time_out)) {
					q_close_socket(client_socket);
					Q_DEBUG("QConcurrentInterface: set time out error, value = (%d)", rm->m_time_out);
					continue;
				}

				rm->m_queue.push(client_socket);
			}

			return 0;
		}

	protected:
		// Socket描述符队列
		QQueue<Q_SOCKET_T> m_queue;

		// 工作线程数量
		int32_t m_worker_num;
		// 队列最大容量
		int32_t m_queue_size;

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

#endif // __QCONCURRENTINTERFACE_H_
