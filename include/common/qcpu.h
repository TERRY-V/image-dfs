/********************************************************************************************
**
** Copyright (C) 2010-2015 Terry Niu (Beijing, China)
** Filename:	qcpu.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2015/02/18
**
*********************************************************************************************/

#ifndef __QCPU_H_
#define __QCPU_H_

#include "qglobal.h"

Q_BEGIN_NAMESPACE

// 服务器CPU负载信息类(负载均衡)
// 均衡器会定时向实际服务器发送收集负载信息的请求, 实际服务器会将CPU负载信息回传给均衡器.
// 该类会取系统负载均值, 磁盘信息, 内存信息和系统当前进程数目
class QCPU {
	public:
		// @函数名: 服务器CPU负载信息类构造函数
		QCPU() :
			I_PROC_CPU_MARK(*(uint64_t*)("@CPU^-^@"))
		{}

		virtual ~QCPU()
		{}

		// @函数名: 初始化函数
		// @参数01: 默认绑定端口号信息
		// @参数02: SOCKET超时时间值
		// @参数03: 该服务器输入负载预估值(单位时间内服务器收到的新连接数与平均连接数的比例)
		// @参数03: 是否当前log日志到屏幕
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(uint16_t cpu_port=5345, int32_t timeout=3000, float64_t conn_ratio=1, bool8_t display_log=true)
		{
			m_cpu_port=cpu_port;
			m_timeout=timeout;
			m_conn_ratio=conn_ratio;
			m_display_log=display_log;

			m_success_flag=0;

			if(q_init_socket()) {
				Q_DEBUG("QCPU: socket init error!");
				return -1;
			}

			if(q_TCP_server(m_listen, m_cpu_port)) {
				Q_DEBUG("QCPU: socket listen (%d) error!", m_cpu_port);
				return -2;
			}

			if(q_create_thread(thread_cpu, this)) {
				Q_DEBUG("QCPU: create cpu thread error!");
				return -3;
			}

			while(m_success_flag==0)
				q_sleep(1);

			if(m_success_flag<0) {
				Q_DEBUG("QCPU: cpu thread start error!");
				return -4;
			}

			return 0;
		}

	private:
		static Q_THREAD_T thread_cpu(void* ptr_info)
		{
			QCPU* cpu=reinterpret_cast<QCPU*>(ptr_info);
			Q_CHECK_PTR(cpu);

			char buffer[1<<6]={0};

			Q_SOCKET_T client_sock;		// 均衡器请求连接套接字
			char client_ip[16];		// 均衡器请求IP地址
			int32_t client_port;		// 均衡器请求PORT端口号

			float64_t load_average=0;	// 服务器负载均值

			uint64_t used_disk_bytes=0;	// 服务器已使用磁盘空间字节数
			uint64_t total_disk_bytes=0;	// 服务器总磁盘空间字节数

			uint64_t used_mem_bytes=0;	// 服务器已使用内存字节数
			uint64_t total_mem_bytes=0;	// 服务器总内存字节数

			uint16_t proc_num=0;		// 服务器系统当前进程数
			float64_t proc_ratio=0;		// 服务器系统进程数目指标权重

			cpu->m_success_flag=1;

			for(;;) {
				// 接收均衡器socket请求
				if(q_accept_socket(cpu->m_listen, client_sock, client_ip, client_port)) {
					Q_INFO("QCPU: accept request error!");
					continue;
				}

				try {
					// 设置socket请求超时时间
					if(q_set_overtime(client_sock, cpu->m_timeout)) {
						Q_INFO("QCPU: set overtime error!");
						throw -1;
					}

					// 接收均衡器请求报文信息
					if(q_recvbuf(client_sock, buffer, 8)||*(uint64_t*)buffer!=cpu->I_PROC_CPU_MARK) {
						Q_INFO("QCPU: recv data error!");
						throw -2;
					}

					// 获取服务器负载均值
					if(cpu->get_load_average(&load_average)) {
						Q_INFO("QCPU: get load average error!");
						throw -3;
					}

					// 获取服务器磁盘使用信息
					if(cpu->get_disk_usage("./", &used_disk_bytes, &total_disk_bytes)) {
						Q_INFO("QCPU: get disk usage error!");
						throw -4;
					}

					// 获取服务器内存使用信息
					if(cpu->get_mem_usage(&used_mem_bytes, &total_mem_bytes)) {
						Q_INFO("QCPU: get mem usage error!");
						throw -5;
					}

					// 获取服务器进程信息
					if(cpu->get_proc_num(&proc_num, &proc_ratio)) {
						Q_INFO("QCPU: get proc num error!");
						throw -6;
					}

					// 服务器响应包头信息
					*(uint64_t*)(buffer)=cpu->I_PROC_CPU_MARK;
					*(uint32_t*)(buffer+8)=sizeof(int64_t)*5;

					// 服务器响应指标权重
					*(float64_t*)(buffer+12)=cpu->m_conn_ratio;
					*(float64_t*)(buffer+20)=load_average;
					*(float64_t*)(buffer+28)=static_cast<float64_t>(used_disk_bytes)/static_cast<float64_t>(total_disk_bytes+1);
					*(float64_t*)(buffer+36)=static_cast<float64_t>(used_mem_bytes)/static_cast<float64_t>(total_mem_bytes+1);
					*(float64_t*)(buffer+44)=proc_ratio;

					// 发送服务器响应报文
					if(q_sendbuf(client_sock, buffer, 52)) {
						Q_INFO("QCPU: send data error!");
						throw -7;
					}

					// 关闭本次对话
					q_close_socket(client_sock);

					if(cpu->m_display_log) {
						Q_INFO("QCPU: [%s:%05d] server load average(%lf), used disk bytes(%lu), total disk bytes(%lu), " \
								"used mem bytes(%lu), total mem bytes(%lu), proc num(%d)", \
								client_ip, client_port, \
								load_average, \
								used_disk_bytes, total_disk_bytes, \
								used_mem_bytes, total_mem_bytes, \
								proc_num);
					}
				} catch(const int32_t /* error */) {
					// 请求失败时不返回报文信息
					q_close_socket(client_sock);
				}
			}

			cpu->m_success_flag=-1;
			return NULL;
		}

		// @函数名: 获取处理器数量(对均衡器用处不大)
		int32_t get_processor_num()
		{
#ifdef WIN32
			SYSTEM_INFO si;
			GetSystemInfo(&si);
			return si.dwNumberOfProcessors;
#else
			return get_nprocs();
#endif
		}

		// @函数名: 获取1分钟内的系统平均负载(放大100倍)
		int32_t get_load_average(float64_t* load_average)
		{
#ifdef WIN32
			return 0;
#else
			if(::getloadavg(load_average, 1)==-1)
				return -1;

			// 每分钟系统平均负载超过1.00即超负载状态
			if(*load_average>1.00)
				*load_average=1.00;

			return 0;
#endif
		}

		// @函数名: 获取指定路径磁盘信息
		int32_t get_disk_usage(const char* path, uint64_t* used_disk_bytes, uint64_t* total_disk_bytes)
		{
#ifdef WIN32
			ULARGE_INTEGER lpfree2caller;
			ULARGE_INTEGER lptotal;
			ULARGE_INTEGER lpfree;
			if(GetDiskFreeSpaceEx(L"./", &lpfree2caller, &lptotal, &lpfree)) {
				*used_disk_bytes=lptotal.QuadPart-lpfree2caller.QuadPart;
				*total_disk_bytes=lptotal.QuadPart;
				return 0;
			}
			return -1;
#else
			struct statfs __stat;
			if(::statfs(path, &__stat)!=0)
				return -1;

			*used_disk_bytes=(uint64_t)(__stat.f_blocks-__stat.f_bfree)*__stat.f_bsize;
			*total_disk_bytes=(uint64_t)(__stat.f_blocks)*__stat.f_bsize;

			return 0;
#endif
		}

		// @函数名: 获取服务器内存相关信息
		int32_t get_mem_usage(uint64_t* used_mem_bytes, uint64_t* total_mem_bytes)
		{
#ifdef WIN32
			MEMORYSTATUS memstatus;
			GlobalMemoryStatus(&memstatus);
			*used_mem_bytes=memstatus.dwAvailPhys;
			*total_mem_bytes=memstatus.dwTotalPhys;
			return 0;
#else
			struct sysinfo __si;
			if(::sysinfo(&__si)!=0)
				return -1;

			*used_mem_bytes=__si.totalram-__si.freeram;
			*total_mem_bytes=__si.totalram;

			return 0;
#endif
		}

		// @函数名: 获取服务器总进程数目
		// @参数01: 服务器当前进程数
		// @参数02: 服务器进程数指标权重, 该指标也仅在进程数变化很大时有效
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t get_proc_num(uint16_t* proc_num, float64_t* proc_ratio)
		{
#ifdef WIN32
			return 0;
#else
			struct sysinfo __si;
			if(::sysinfo(&__si)!=0)
				return -1;

			*proc_num=__si.procs;

			// 通常服务器进程数会小于此值, 否者直接按超负载进行处理
			if(*proc_num<(1<<10)) {
				*proc_ratio=static_cast<double>(*proc_num)/(1<<10);
			} else {
				*proc_ratio=1;
			}
			return 0;
#endif
		}

	protected:
		uint64_t	I_PROC_CPU_MARK;	// CPU请求唯一标识

		Q_SOCKET_T	m_listen;		// 该服务监听套接字
		uint16_t	m_cpu_port;		// CPU信息监控端口
		int32_t		m_timeout;		// SOCKET超时时间(默认为毫秒)

		float64_t	m_conn_ratio;		// 该服务器输入负载预估值(单位时间内服务器收到的新连接数与各服务器平均连接数的比例)

		int32_t		m_success_flag;		// 线程启动状态标识
		bool8_t		m_display_log;		// 是否打印日志到屏幕
};

Q_END_NAMESPACE

#endif // __QCPU_H_
