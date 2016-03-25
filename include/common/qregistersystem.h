/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qregistersystem.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2015/01/29
**
*********************************************************************************************/

#ifndef __QREGISTERSYSTEM_H_
#define __QREGISTERSYSTEM_H_

#include "qglobal.h"
#include "qdir.h"
#include "qmd5.h"
#include "qtcpsocket.h"

Q_BEGIN_NAMESPACE

// 注册系统客户端
class QRegisterClient {
	public:
		// @函数名: 初始化注册系统客户端
		// @参数01: 服务端IP
		// @参数02: 服务端PORT
		// @参数03: 超时时间, 默认为5s
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(const char* server_ip, uint16_t server_port, int32_t timeout=5000)
		{
			if(m_tcp_client.init(server_ip, server_port, timeout))
				return -1;
			return 0;
		}

		// @函数名: 向服务端注册
		// @参数01: 向服务端注册的名称
		// @参数02: 向服务端注册的名称长度
		// @参数03: 服务端返回的注册号
		// @返回值: 0表示已经到达注册系统最大数, 1新注册成功, 2表示已经注册, 小于0表示执行失败
		int32_t register_to_server(const char* register_name, int32_t register_size, uint64_t* register_key)
		{
			char buffer[128];
			int32_t buffer_len=0;

			*(uint64_t*)buffer=*(uint64_t*)"REGISTER";
			*(int32_t*)(buffer+8)=4+register_size;
			*(int32_t*)(buffer+12)=register_size;
			memcpy(buffer+16, register_name, register_size);

			buffer_len=16+register_size;

			void* handle=NULL;
			if(m_tcp_client.connect(handle)) {
				Q_DEBUG("QRegisterClient: register_to_server connect error!");
				return -1;
			}

			if(m_tcp_client.send(handle, buffer, buffer_len)) {
				Q_DEBUG("QRegisterClient: register_to_server send error!");
				m_tcp_client.close(handle);
				return -2;
			}

			// "REGISTER"(8), register_flag(4) register_key(8)
			if(m_tcp_client.recv(handle, buffer, 20)) {
				Q_DEBUG("QRegisterClient: register_to_server recv error!");
				m_tcp_client.close(handle);
				return -3;
			}

			m_tcp_client.close(handle);

			if(*(uint64_t*)buffer!=*(uint64_t*)"REGISTER") {
				Q_DEBUG("QRegisterClient: register_to_server version error!");
				return -4;
			}

			int32_t register_flag=*(int32_t*)(buffer+8);
			if(register_flag!=0&&register_flag!=1&&register_flag!=2)
				return -5;

			*register_key=*(uint64_t*)(buffer+12);

			return register_flag;
		}

	protected:
		QTcpClient m_tcp_client;
};

// 注册系统服务端
class QRigisterServer: public QTcpServer {
	public:
		// @函数名: 初始化注册系统服务端
		// @参数01: 注册文件保存路径
		// @参数02: 服务端监听端口号
		// @参数03: 当有新注册服务时, 调用的回调函数
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(const char* save_path, uint16_t listen_port, void(*func)(uint64_t reg_key, int32_t reg_len, char* reg_val)=NULL)
		{
			strcpy(m_save_path, save_path);
			m_func=func;

			sprintf(m_save_file_lib, "%s/register.info", m_save_path);
			sprintf(m_save_file_bak, "%s/register.back", m_save_path);

			m_register_num=0;

			if(!QDir::mkdir(m_save_path)) {
				Q_DEBUG("QRegisterServer: create dir(%s) error!", m_save_path);
				return -1;
			}

			if(load_register_info()) {
				Q_DEBUG("QRegisterServer: load register info error!");
				return -2;
			}

			if(QTcpServer::init(listen_port, 1, 5000, 1, 12, 32, 256, 256)) {
				Q_DEBUG("QRegisterServer: QTcpServer::init error!");
				return -3;
			}

			if(start()) {
				Q_DEBUG("QRegisterServer: start() error!");
				return -4;
			}

			return 0;
		}

		// @函数名: 数据包头解析函数
		int32_t ts_fun_package(const char* in_buf, int32_t in_buf_len, void* handle=NULL)
		{
			if(*(uint64_t*)in_buf!=*(uint64_t*)"REGISTER") {
				Q_DEBUG("QRegisterServer: version error!");
				return -1;
			}

			if(*(int32_t*)(in_buf+8)>1<<7) {
				Q_DEBUG("QRegisterServer: after length(%d) is too long!", *(int32_t*)(in_buf+8));
				return -2;
			}

			return *(int32_t*)(in_buf+8);
		}

		// @函数名: 数据协议处理函数
		int32_t ts_fun_process(const char* in_buf, int32_t in_buf_len, char* out_buf, int32_t out_buf_size, void* handle=NULL)
		{
			uint64_t register_key=md5.MD5Bits64((uint8_t*)(in_buf+4), *(int32_t*)in_buf);
			int32_t register_pos=0;

			for(int32_t i=0; i<m_register_num; ++i) {
				if(m_register_key[i]==register_key) {
					register_pos=i;
					break;
				}
			}

			*(uint64_t*)out_buf=*(uint64_t*)"REGISTER";

			if(register_pos==m_register_num) {
				if(m_register_num>=MAX_REG_NUM) {
					// 达到注册系统最大数
					*(int32_t*)(out_buf+8)=0;
				} else {
					// 仍然可注册
					m_register_key[m_register_num]=register_key;
					m_register_len[m_register_num]=*(int32_t*)in_buf;
					memcpy(m_register_val[m_register_num], in_buf+4, *(int32_t*)in_buf);
					m_register_num++;

					// 将注册信息落地
					if(save_register_info()) {
						m_register_num--;
						return -1;
					}

					// 备份注册信息文件
					if(q_swap_file(m_save_file_bak, m_save_file_lib)) {
						m_register_num--;
						return -2;
					}

					*(int32_t*)(out_buf+8)=1;
				}
			} else {
				// 已经注册
				*(int32_t*)(out_buf+8)=2;
			}

			// 注册码
			*(uint64_t*)(out_buf+12)=register_key;

			if(m_func&&*(int32_t*)(out_buf+8)==1)
				m_func(m_register_key[m_register_num-1], m_register_len[m_register_num-1], m_register_val[m_register_num-1]);

			return 0;
		}

	private:
		// 备份注册信息
		int32_t save_register_info()
		{
			FILE* fp=fopen(m_save_file_bak, "wb");
			if(fp==NULL) {
				Q_DEBUG("QRegisterServer: open %s error!", m_save_file_bak);
				return -1;
			}

			try {
				if(fwrite(&m_register_num, 4, 1, fp)!=1)
					throw -2;

				for(int32_t i=0; i<m_register_num; ++i)
				{
					if(fwrite(m_register_key+i, 8, 1, fp)!=1)
						throw -3;

					if(fwrite(m_register_len+i, 4, 1, fp)!=1)
						throw -4;

					if(fwrite(m_register_val[i], m_register_len[i], 1, fp)!=1)
						throw -5;
				}
			} catch(const int32_t err) {
				fclose(fp);
				return err;
			}

			fclose(fp);
			return 0;
		}

		// 初始化注册信息
		int32_t load_register_info()
		{
			if(access(m_save_file_lib, 0)) {
				Q_INFO("QRegisterServer: register.info not exist, is new register system!");
				return 0;
			}

			FILE* fp=fopen(m_save_file_lib, "rb");
			if(fp==NULL) {
				Q_INFO("QRegisterServer: open %s error!", m_save_file_lib);
				return -1;
			}

			try {
				if(fread(&m_register_num, 4, 1, fp)!=1)
					return -2;

				for(int32_t i=0; i<m_register_num; ++i)
				{
					if(fread(m_register_key+i, 8, 1, fp)!=1)
						return -3;

					if(fread(m_register_len+i, 4, 1, fp)!=1)
						return -4;

					if(fread(m_register_val[i], m_register_len[i], 1, fp)!=1)
						return -5;
				}
			} catch(const int32_t err) {
				fclose(fp);
				return err;
			}

			fclose(fp);
			return 0;
		}

	protected:
		enum {MAX_REG_NUM=512};

		char m_save_path[1<<8];
		char m_save_file_lib[1<<8];
		char m_save_file_bak[1<<8];

		uint64_t m_register_key[MAX_REG_NUM];
		int32_t m_register_len[MAX_REG_NUM];
		char m_register_val[MAX_REG_NUM][64];
		int32_t m_register_num;

		QMD5 md5;
		void (*m_func)(uint64_t reg_key, int32_t reg_len, char* reg_val);
};

Q_END_NAMESPACE

#endif // __QREGISTERSYSTEM_H_
