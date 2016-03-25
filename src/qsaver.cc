#include "qsaver.h"

struct imageType image_type [] =
{
	{0,(char*)"jpg"},
	{1,(char*)"png"},
	{2,(char*)"bmp"},
	{3,(char*)"gif"},
	{4,(char*)"tif"},
	{5,(char*)"jpg"}
};

QSaver::QSaver() :
	m_sock_svr(-1),
	I_SEND_FILE_MARK(*(uint64_t*)"@#@#@#@#"),
	m_start_flag(false),
	m_exit_flag(false),
	m_ptr_trd_info(NULL),
	m_thread_max(0),
	m_ptr_trigger(NULL),
	m_ptr_cfg_info(NULL),
	m_ptr_logger(NULL),
	m_svr_sum(0)
{}

QSaver::~QSaver()
{
	q_close_socket(m_sock_svr);

	m_exit_flag=true;
	m_ptr_trigger->signal();

	for(int32_t i=0; i<m_thread_max; ++i) {
		while(m_ptr_trd_info[i].run_flag!=-1)
			q_sleep(1000);

		q_delete_array<char>(m_ptr_trd_info[i].ptr_buf);

	}

	q_delete<QLogger>(m_ptr_logger);
	q_delete<QTrigger>(m_ptr_trigger);
	q_delete<CONFIG_INFO>(m_ptr_cfg_info);
	q_delete_array<THREAD_INFO>(m_ptr_trd_info);
}

int32_t QSaver::init(const char* cfg_file)
{
	Q_INFO("Hello, QSC_NXL...");

	int32_t ret=0;

	m_ptr_logger=q_new<QLogger>();
	if(m_ptr_logger==NULL) {
		Q_FATAL("m_ptr_logger alloc error, null value!");
		return -1;
	}

	ret=m_ptr_logger->init("../log", NULL, 1<<10);
	if(ret<0) {
		Q_FATAL("m_ptr_logger init error, ret = (%d)", ret);
		return -2;
	}

	m_ptr_cfg_info=q_new<CONFIG_INFO>();
	if(m_ptr_cfg_info==NULL) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "m_ptr_cfg_info is null!");
		return -3;
	}

	QConfigReader conf;
	if(conf.init(cfg_file)!=0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "config init error, cfg_file = (%s)", cfg_file);
		return -4;
	}

	ret=conf.getFieldString("SERVER_NAME", m_ptr_cfg_info->server_name, sizeof(m_ptr_cfg_info->server_name));
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init SERVER_NAME error, ret = (%d)", ret);
		return -5;
	}

	ret=conf.getFieldString("SERVER_IP", m_ptr_cfg_info->server_ip, sizeof(m_ptr_cfg_info->server_ip));
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init SERVER_IP error, ret = (%d)", ret);
		return -6;
	}

	ret=conf.getFieldInt32("SERVER_PORT", m_ptr_cfg_info->server_port);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init SERVER_PORT error, ret = (%d)", ret);
		return -7;
	}

	ret=conf.getFieldInt32("MONITOR_PORT", m_ptr_cfg_info->monitor_port);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init MONITOR_PORT error, ret = (%d)", ret);
		return -8;
	}

	ret=conf.getFieldInt32("SOCK_TIMEOUT", m_ptr_cfg_info->sock_timeout);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init SOCK_TIMEOUT error, ret = (%d)", ret);
		return -9;
	}

	ret=conf.getFieldInt32("COMM_THREAD_MAX", m_ptr_cfg_info->comm_thread_max);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init COMM_THREAD_MAX error, ret = (%d)", ret);
		return -10;
	}

	ret=conf.getFieldInt32("WORK_THREAD_MAX", m_ptr_cfg_info->work_thread_max);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init WORK_THREAD_MAX error, ret = (%d)", ret);
		return -11;
	}

	ret=conf.getFieldInt32("COMM_BUFFER_SIZE", m_ptr_cfg_info->comm_buffer_size);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init COMM_BUFFER_SIZE error, ret = (%d)", ret);
		return -12;
	}

	ret=conf.getFieldInt32("WORK_BUFFER_SIZE", m_ptr_cfg_info->work_buffer_size);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init WORK_BUFFER_SIZE error, ret = (%d)", ret);
		return -13;
	}

	ret=conf.getFieldInt32("COMM_THREAD_TIMEOUT", m_ptr_cfg_info->comm_thread_timeout);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init COMM_THREAD_TIMEOUT error, ret = (%d)", ret);
		return -14;
	}

	ret=conf.getFieldInt32("WORK_THREAD_TIMEOUT", m_ptr_cfg_info->work_thread_timeout);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init WORK_THREAD_TIMEOUT error, ret = (%d)", ret);
		return -15;
	}
	
	ret=conf.getFieldInt32("TASK_QUEUE_SIZE", m_ptr_cfg_info->task_queue_size);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init TASK_QUEUE_SIZE error, ret = (%d)", ret);
		return -17;
	}

	ret=conf.getFieldString("IMAGE_SAVE_PATH", m_ptr_cfg_info->image_save_path, sizeof(m_ptr_cfg_info->image_save_path));
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init IMAGE_SAVE_PATH error, ret = (%d)", ret);
		return -18;
	}

	ret=conf.getFieldInt32("IMAGE_DIR_NUMS", m_ptr_cfg_info->image_dir_nums);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init IMAGE_DIR_NUMS error, ret = (%d)", ret);
		return -19;
	}
	

#if defined (DEBUG)
	Q_INFO("SERVER_NAME         = %s", m_ptr_cfg_info->server_name);

	Q_INFO("SERVER_IP           = %s", m_ptr_cfg_info->server_ip);
	Q_INFO("SERVER_PORT         = %d", m_ptr_cfg_info->server_port);
	Q_INFO("MONITOR_PORT        = %d", m_ptr_cfg_info->monitor_port);

	Q_INFO("SOCK_TIMEOUT        = %d", m_ptr_cfg_info->sock_timeout);

	Q_INFO("COMM_THREAD_MAX     = %d", m_ptr_cfg_info->comm_thread_max);
	Q_INFO("WORK_THREAD_MAX     = %d", m_ptr_cfg_info->work_thread_max);

	Q_INFO("COMM_BUFFER_SIZE    = %d", m_ptr_cfg_info->comm_buffer_size);
	Q_INFO("WORK_BUFFER_SIZE    = %d", m_ptr_cfg_info->work_buffer_size);

	Q_INFO("COMM_THREAD_TIMEOUT = %d", m_ptr_cfg_info->comm_thread_timeout);
	Q_INFO("WORK_THREAD_TIMEOUT = %d", m_ptr_cfg_info->work_thread_timeout);

	Q_INFO("TASK_QUEUE_SIZE     = %d", m_ptr_cfg_info->task_queue_size);

	Q_INFO("IMAGE_SAVE_PATH     = %s", m_ptr_cfg_info->image_save_path);
	Q_INFO("IMAGE_DIR_NUMS      = %d", m_ptr_cfg_info->image_dir_nums);
#endif

	ret=m_task_queue.init(m_ptr_cfg_info->task_queue_size);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init m_task_queue error, ret = (%d)", ret);
		return -51;
	}

	m_ptr_trigger=q_new<QTrigger>();
	if(m_ptr_trigger==NULL) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "m_ptr_trigger is null!");
		return -52;
	}

	ret=initImageDir(m_ptr_cfg_info->image_save_path, m_ptr_cfg_info->image_dir_nums);
	if(ret<0){
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "creat image dir error, ret = (%d)", ret);
		return -521;
	}

	ret=q_init_socket();
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init socket error, ret = (%d)", ret);
		return -53;
	}

	ret=q_TCP_server(m_sock_svr, m_ptr_cfg_info->server_port);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "TCP server error, ret = (%d)", ret);
		return -54;
	}

	m_thread_max=m_ptr_cfg_info->comm_thread_max+m_ptr_cfg_info->work_thread_max;
	m_ptr_trd_info=q_new_array<THREAD_INFO>(m_thread_max);
	if(m_ptr_trd_info==NULL) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "create m_ptr_trd_info error, ret = (%d)", ret);
		return -71;
	}

	THREAD_INFO* ptr_trd=m_ptr_trd_info;
	for(int32_t i=0; i!=m_ptr_cfg_info->comm_thread_max; ++i)
	{
		ptr_trd[i].ptr_this=this;
		ptr_trd[i].id=i;
		ptr_trd[i].status=0;
		ptr_trd[i].run_flag=0;
		ptr_trd[i].timeout=m_ptr_cfg_info->comm_thread_timeout;
		ptr_trd[i].buf_size=m_ptr_cfg_info->comm_buffer_size;
		ptr_trd[i].ptr_buf=q_new_array<char>(ptr_trd[i].buf_size);
		if(ptr_trd[i].ptr_buf==NULL) {
			m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "create comm_thread (%d) buffer error", i+1);
			return -72;
		}

		ret=q_create_thread(QSaver::comm_thread, ptr_trd+i);
		if(ret<0) {
			m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "create comm_thread (%d) error", i+1);
			return -73;
		} else {
			Q_INFO("Thread comm_thread (%02d) runs now......", i+1);
		}
	}

	ptr_trd+=m_ptr_cfg_info->comm_thread_max;
	for(int32_t i=0; i!=m_ptr_cfg_info->work_thread_max; ++i)
	{
		ptr_trd[i].ptr_this=this;
		ptr_trd[i].id=i;
		ptr_trd[i].status=0;
		ptr_trd[i].run_flag=0;
		ptr_trd[i].timeout=m_ptr_cfg_info->work_thread_timeout;
		ptr_trd[i].buf_size=m_ptr_cfg_info->work_buffer_size;
		ptr_trd[i].ptr_buf=q_new_array<char>(ptr_trd[i].buf_size);
		if(ptr_trd[i].ptr_buf==NULL) {
			m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "create work_thread (%d) buffer error", i+1);
			return -74;
		}

		ret=q_create_thread(QSaver::work_thread, ptr_trd+i);
		if(ret<0) {
			m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "create work_thread (%d) error", i+1);
			return -77;
		} else {
			Q_INFO("Thread work_thread (%02d) runs now......", i+1);
		}
	}

	ptr_trd+=m_ptr_cfg_info->work_thread_max;

	// 等待所有线程准备就绪
	for(int32_t i=0; i!=m_thread_max; ++i)
	{
		int32_t count=0;
		while(m_ptr_trd_info[i].run_flag!=1)
		{
			q_sleep(100);
			if(++count>50) {
				m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "Thread (%d) starts failed......", i);
				return -78;
			}
		}
	}

	// 启动监控服务
	ret=m_remote_monitor.init(m_ptr_cfg_info->monitor_port, get_thread_state, this, 1);
	if(ret<0) {
		m_ptr_logger->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, 1, "init monitor error, ret = (%d)", ret);
		return -91;
	}

	m_start_flag=true;

	return 0;
}

int32_t QSaver::run()
{
	Q_INFO("input s or S to stop the QSaver!");

	char ch_;
	std::cin>>ch_;
	while(ch_!='s'&&ch_!='S') {
		Q_INFO("input s or S to stop the QSaver!");
		std::cin>>ch_;
	}

	Q_INFO("QSaver stop!");

	return 0;
}

// 通信线程
Q_THREAD_T QSaver::comm_thread(void* ptr_info)
{
	THREAD_INFO* ptr_trd=static_cast<THREAD_INFO*>(ptr_info);
	Q_CHECK_PTR(ptr_trd);

	QSaver* ptr_this=static_cast<QSaver*>(ptr_trd->ptr_this);
	Q_CHECK_PTR(ptr_this);

	ptr_trd->run_flag=1;

	TASK_INFO task_info;

	while(!ptr_this->m_exit_flag)
	{
		ptr_trd->status=0;

		if(q_accept_socket(ptr_this->m_sock_svr, task_info.sock_client, task_info.client_ip, task_info.client_port)) {
			Q_INFO("TCP socket accept error!");
			q_sleep(1);
			continue;
		}

		ptr_trd->status=1;
		ptr_trd->sw.start();

		if(q_set_overtime(task_info.sock_client, ptr_this->m_ptr_cfg_info->sock_timeout)) {
			Q_INFO("TCP socket overtime, timeout = (%d)", ptr_this->m_ptr_cfg_info->sock_timeout);
			continue;
		}

		ptr_this->m_task_queue.push(task_info);
		ptr_this->m_ptr_trigger->signal();

		ptr_trd->sw.stop();
	}

	ptr_trd->run_flag=-1;
	return NULL;
}

// 工作线程
Q_THREAD_T QSaver::work_thread(void* ptr_info)
{
	THREAD_INFO* ptr_trd=static_cast<THREAD_INFO*>(ptr_info);
	Q_CHECK_PTR(ptr_trd);

	QSaver* ptr_this=static_cast<QSaver*>(ptr_trd->ptr_this);
	Q_CHECK_PTR(ptr_this);

	PROTOCOL_RESPONSE_HEAD pro_res_head;
	pro_res_head.version=PROTOCOL_HEAD_VERSION;
	pro_res_head.length=sizeof(PROTOCOL_RESPONSE_HEAD)-12;
	pro_res_head.cmd_type=0;
	pro_res_head.status_code=0;

	char* recv_buf=ptr_trd->ptr_buf;
	int32_t max_recv_size=ptr_trd->buf_size;
	int32_t recv_len=0;

	char* send_buf=ptr_trd->ptr_buf;
	int32_t max_send_size=0;
	int32_t send_len=0;

	//XML_INFO xml_info;

#if defined (__TIME_USE_TEST__)
	QStopwatch sw_download;
#endif

	ptr_trd->run_flag=1;

	TASK_INFO task_info;

	while(!ptr_this->m_exit_flag)
	{
		ptr_trd->status=0;

		ptr_this->m_ptr_trigger->wait();

		while(ptr_this->m_task_queue.pop_non_blocking(task_info)==0)
		{
			ptr_trd->status=1;
			ptr_trd->sw.start();

#if defined (__TIME_USE_TEST__)
			sw_download.start();
#endif

			recv_buf=ptr_trd->ptr_buf;
			max_recv_size=ptr_trd->buf_size;
			recv_len=0;

			try {
				if(q_recvbuf(task_info.sock_client, recv_buf, 12)) {
					Q_INFO("TCP socket recv header error!");
					throw -1;
				}

				if(*(uint64_t*)recv_buf!=PROTOCOL_HEAD_VERSION) {
					Q_INFO("TCP socket version error, version = (%.*s)", 8, recv_buf);
					throw -2;
				}

				recv_len=*(int32_t*)(recv_buf+8);
				if(recv_len<=0) {
					Q_INFO("Recv len error: recv_len = (%d)", recv_len);
					throw -3;
				}

				if(recv_len>0) {
					max_recv_size-=12;
					if(recv_len>max_recv_size) {
						Q_INFO("Recv len error: recv_len(%d) > max_recv_size(%d)", recv_len, max_recv_size);
						throw -4;
					}

					if(q_recvbuf(task_info.sock_client, recv_buf+12, recv_len)) {
						Q_INFO("TCP socket recv body error, size = (%d)", recv_len);
						throw -5;
					}
				}

				send_buf=ptr_trd->ptr_buf+12+recv_len;
				max_send_size=ptr_trd->ptr_buf+ptr_trd->buf_size-send_buf;

				int32_t ret=ptr_this->fun_process(recv_buf, recv_len+12, send_buf, max_send_size);
				if(ret<0) {
					Q_INFO("fun process error: ret = (%d)", ret);
					throw ret;
				}

				if(ret>0) {
					send_len=ret;
					if(q_sendbuf(task_info.sock_client, send_buf, send_len)) {
						Q_INFO("TCP socket send error: send_len = (%d)", send_len);
						throw -6;
					}
				}

				q_close_socket(task_info.sock_client);
			} catch(const int32_t err) {
				pro_res_head.status_code=err;
				q_sendbuf(task_info.sock_client, (char*)(&pro_res_head), sizeof(PROTOCOL_RESPONSE_HEAD));
				q_close_socket(task_info.sock_client);
				Q_INFO("Work thread [%s:%d] error, ret = %d", task_info.client_ip, task_info.client_port, err); 
			}

			q_add_and_fetch(&ptr_this->m_svr_sum);

#if defined (__TIME_USE_TEST__)
			sw_download.stop();
			Q_INFO("Work thread process task (%u) consumed: %ds", ptr_this->m_svr_num, sw_download.elapsed_s());
#endif
		}
		ptr_trd->sw.stop();
	}

	ptr_trd->run_flag=-1;
	return NULL;
}

// 任务处理函数
int32_t QSaver::fun_process(char* recv_buf, int32_t recv_len, char* send_buf, int32_t max_send_size)
{
	char *ptr_temp=recv_buf;
	char *ptr_end=ptr_temp+recv_len;
	int16_t operate_type=0;
	int32_t img_len=0;
	int32_t ret=0;

	PROTOCOL_REQUEST_HEAD* pro_req_head=reinterpret_cast<PROTOCOL_REQUEST_HEAD*>(ptr_temp);
	ptr_temp+=sizeof(PROTOCOL_REQUEST_HEAD);

	if(pro_req_head->protocol_type!=1) {
		Q_INFO("Protocol type error, protocol_type = (%d)", pro_req_head->protocol_type);
		return -11;
	}

	if(pro_req_head->source_type!=0) {
		Q_INFO("Source type error, source_type = (%d)", pro_req_head->source_type);
		return -12;
	}

	if(!(pro_req_head->cmd_type==1||pro_req_head->cmd_type==2)) {
		Q_INFO("Command type error, cmd_type = (%d)", pro_req_head->cmd_type);
		return -13;
	}

	// 操作类型
	operate_type=*(int16_t*)ptr_temp;
	if(operate_type>=0 && operate_type<=5) {
		ptr_temp+=sizeof(int16_t);
	} else {
		Q_INFO("Operate type error, operate_type = (%d)", operate_type);
		return -14;
	}

	img_len=*(int32_t*)ptr_temp;
	if(img_len<=0) {
		Q_INFO("XML length error, xml_len = (%d)", img_len);
		return -15;
	}
	ptr_temp+=sizeof(int32_t);

	if(ptr_temp+img_len+4!=ptr_end) {
		Q_INFO("Fun process protocol format error!"); 
		return -16;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 数据下载与回送
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	PROTOCOL_RESPONSE_HEAD *pro_res_head=reinterpret_cast<PROTOCOL_RESPONSE_HEAD*>(send_buf);
	pro_res_head->version=PROTOCOL_HEAD_VERSION;
	pro_res_head->length=0;
	pro_res_head->status_code=0;
	pro_res_head->cmd_type=pro_req_head->cmd_type;

	char *ptr_send_temp=send_buf+sizeof(PROTOCOL_RESPONSE_HEAD)+4;
	char *ptr_send_end=send_buf+max_send_size;

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	if(ptr_send_temp+ret>=ptr_send_end)
		return -51;
	ptr_send_temp+=ret;

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "<doc>\n");
	if(ptr_send_temp+ret>=ptr_send_end)
		return -52;
	ptr_send_temp+=ret;

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "<base>\n");
	if(ptr_send_temp+ret>=ptr_send_end)
		return -53;
	ptr_send_temp+=ret;

	QMD5 qmd5;
	char imgfile[1<<10]={0};
	char imgpath[1<<10]={0};
	int32_t width=0;
	int32_t height=0;

	uint64_t iid=qmd5.MD5Bits64((unsigned char*)ptr_temp, img_len);
	int32_t dir_index=iid % m_ptr_cfg_info->image_dir_nums;

	ret=snprintf(imgfile, sizeof(imgfile), "%s%02d/%lx.%s", m_ptr_cfg_info->image_save_path, dir_index, iid, image_type[operate_type].type);
	if(ret<0)
		return -54;

	ret=saveImage(imgfile, ptr_temp, img_len);
	if(ret<0)
		return -55;

	if(snprintf(imgpath, sizeof(imgpath), "%02d/%lx.%s", dir_index, iid, image_type[operate_type].type)<0)
		return -56;

	ret=getImageSize(imgfile, &width, &height);
	if(ret<0)
		return -57;

#if defined (DEBUG)
	Q_INFO("image path = %s <%s>", imgpath, (ret==1) ? "Dup" : "New");
#endif

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "<imgid><![CDATA[%lu]]></imgid>\n", iid);
	if(ptr_send_temp+ret>=ptr_send_end)
		return -58;
	ptr_send_temp+=ret;

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "<imgpath><![CDATA[%s]]></imgpath>\n", imgpath);
	if(ptr_send_temp+ret>=ptr_send_end)
		return -59;
	ptr_send_temp+=ret;

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "<imgsize><![CDATA[%d*%d]]></imgpath>\n", width, height);
	if(ptr_send_temp+ret>=ptr_send_end)
		return -60;
	ptr_send_temp+=ret;

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "</base>\n");
	if(ptr_send_temp+ret>=ptr_send_end)
		return -61;
	ptr_send_temp+=ret;

	ret=snprintf(ptr_send_temp, ptr_send_end-ptr_send_temp, "</doc>\n");
	if(ptr_send_temp+ret>=ptr_send_end)
		return -62;
	ptr_send_temp+=ret;

	ptrdiff_t send_len=ptr_send_temp-send_buf;
	pro_res_head->length=send_len-sizeof(PROTOCOL_HEAD_VERSION)-4;
	*(int32_t*)(send_buf+sizeof(PROTOCOL_RESPONSE_HEAD))=send_len-sizeof(PROTOCOL_RESPONSE_HEAD)-4;

	return send_len;
}

int32_t QSaver::get_thread_state(void* ptr_info)
{
	QSaver* ptr_this=static_cast<QSaver*>(ptr_info);
	Q_CHECK_PTR(ptr_this);

	int32_t timeout_thread_num=0;
	for(int32_t i=0; i<ptr_this->m_thread_max; ++i) {
		if(ptr_this->m_ptr_trd_info[i].status!=1)
			continue;

		ptr_this->m_ptr_trd_info[i].sw.stop();

		if(ptr_this->m_ptr_trd_info[i].sw.elapsed_ms()>ptr_this->m_ptr_trd_info[i].timeout)
			++timeout_thread_num;
	}

	return timeout_thread_num;
}

int32_t QSaver::initImageDir(const char* img_path, int32_t dir_num)
{
	for(int32_t i=0; i<dir_num; i++)
	{
		char img_dir[1<<10] = {0};
		if(snprintf(img_dir, sizeof(img_dir), "%s%02d", img_path, i) <0)
			throw -90;

		if(!QDir::mkdir(img_dir))
			return -91;
	}
	return 0;
}

int32_t QSaver::saveImage(const char* path, const char* data, int32_t len)
{
	if(path==NULL || data==NULL || len<=0)
		return -1;
	if(access(path, F_OK)!=0)
	{
		FILE* fp = fopen(path, "w");
		if(fp==NULL)
			return -2;
		if(fwrite(data, len, 1, fp)!=1){
			fclose(fp);
			fp=NULL;
			return -3;
		}
		fclose(fp);
		fp=NULL;
	}else{
		return 1;
	}
	return 0;
}

