/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qslavecrawler.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2015/09/08
**
*********************************************************************************************/

#ifndef __QSAVER_H_
#define __QSAVER_H_

#include <qglobal.h>
#include <qconfigreader.h>
#include <qdatetime.h>
#include <qdir.h>
#include <qfile.h>
#include <qfunc.h>
#include <qlogger.h>
#include <qmd5.h>
#include <qopencv.h>
#include <qqueue.h>
#include <qremotemonitor.h>
#include <qservice.h>
#include <qvector.h>

Q_USING_NAMESPACE

const uint64_t PROTOCOL_HEAD_VERSION=*(uint64_t*)"YST1.0.0";

#pragma pack(1)

// 线程信息结构体
typedef struct __thread_info
{
	void		*ptr_this;		// 线程回调指针
	uint32_t	id;			// 线程id
	int8_t 		status;			// 线程工作状态: 0不工作 1工作
	int8_t		run_flag;		// 线程启动成功标识: 0未启动 1启动成功 -1启动失败
	int32_t		buf_size;		// 线程中使用内存大小
	char*		ptr_buf;		// 线程中使用内存
	int32_t		timeout;		// 线程超时时间
	QStopwatch	sw;			// 线程运行时间

	void*		for_worker;		// 工作线程专用

	__thread_info() :
		ptr_this(NULL),
		id(0),
		status(0),
		run_flag(0),
		buf_size(0),
		ptr_buf(NULL),
		timeout(10000),
		for_worker(NULL)
	{}
} THREAD_INFO;

// 外部配置文件信息
typedef struct __config_info
{
	char		server_name[256];	// 服务名称

	char		server_ip[16];		// 通信服务本机IP地址
	int32_t		server_port;		// 通信服务端口号
	int32_t		monitor_port;		// 监控服务端口号

	int32_t		sock_timeout;		// socket超时时间

	int32_t		comm_thread_max;	// 通信线程数
	int32_t		comm_buffer_size;	// 通信线程所使用内存大小
	int32_t		comm_thread_timeout;	// 通信线程监控超时时间

	int32_t		work_thread_max;	// 工作线程数
	int32_t		work_buffer_size;	// 工作线程所使用内存大小
	int32_t		work_thread_timeout;	// 工作线程监控超时时间

	int32_t		task_queue_size;	// 任务队列最大空间

	char		image_save_path[1<<8];	// 图片保存路径
	int32_t		image_dir_nums;		// 生成目录数
} CONFIG_INFO;

// 外部数据请求协议头信息
typedef struct __protocol_request_head
{
	uint64_t	version;		// 验证码
	int32_t		length;			// 后续数据总长度
	int16_t		protocol_type;		// 协议类型
	int16_t		source_type;		// 来源类型
	char		save[14];		// 保留字段
	int16_t		cmd_type;		// 命令类型
} PROTOCOL_REQUEST_HEAD;

// 外部数据应答协议头信息
typedef struct __protocol_response_head
{
	uint64_t	version;		// 验证码
	int32_t		length;			// 后续数据总长度
	char		save[14];		// 保留字段
	int32_t		status_code;		// 返回状态码
	int16_t		cmd_type;		// 命令类型
} PROTOCOL_RESPONSE_HEAD;

// 通信任务信息结构体
typedef struct __task_info
{
	Q_SOCKET_T	sock_client;		// 通信套接字描述符
	char		client_ip[16];		// 客户端请求IP地址
	int32_t		client_port;		// 客户端请求端口号
} TASK_INFO;

struct imageType {
	int 	id;				// id
	char*	type;				// 图片格式
};

#pragma pack()

class QSaver : public noncopyable {
	public:
		// @函数名: 构造函数
		QSaver();

		// @函数名: 析构函数
		virtual ~QSaver();

		// @函数名: 初始化函数
		int32_t init(const char* cfg_file);

		// @函数名: 启动函数
		int32_t start();

	private:
		// @函数名: 通信线程
		static Q_THREAD_T comm_thread(void* ptr_info);

		// @函数名: 工作线程
		static Q_THREAD_T work_thread(void* ptr_info);
		
		// @函数名: 任务处理函数
		int32_t fun_process(char* recv_buf, int32_t recv_len, char* send_buf, int32_t max_send_size);
		
		// @函数名: 获取各线程运行状态
		static int32_t get_thread_state(void* ptr_info);

		// @函数名：创建图片存储目录
		int32_t initImageDir(const char* img_path, int32_t dir_num);

		// @参数名：图片保存函数
		int32_t saveImage(const char* path, const char * data, int32_t len);

	protected:
		/* general */
		Q_SOCKET_T		m_sock_svr;
		uint64_t		I_SEND_FILE_MARK;
		bool			m_start_flag;
		bool			m_exit_flag;
		/* threads */
		THREAD_INFO*		m_ptr_trd_info;
		int32_t			m_thread_max;
		/* queue */
		QQueue<TASK_INFO>	m_task_queue;
		QTrigger*		m_ptr_trigger;
		/* config */
		CONFIG_INFO*		m_ptr_cfg_info;
		QLogger*		m_ptr_logger;
		uint32_t		m_svr_sum;
		QRandom			m_random;
		QRemoteMonitor		m_remote_monitor;
};

#endif // __QSAVER_H_
