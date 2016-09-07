/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	idfsserver.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2012/11/03
**
*********************************************************************************************/

#ifndef __IDFSSERVER_H_
#define __IDFSSERVER_H_

#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>

#include "MD5.h"

#include "qmongoclient.h"
#include "qglobal.h"
#include "qnetworkaccessmanager.h"
#include "qopencv.h"
#include "qtcpsocket.h"

#define IDFS_IMG_MAX_SIZE (3<<20)

Q_USING_NAMESPACE

class IDFSServer : public QTcpServer {
	public:
		// @函数名: 继承类资源初始化函数
		virtual int32_t initialize();

		// @函数名: 业务逻辑初始化函数
		virtual int32_t server_init(void*& handle);

		// @函数名: 消息头解析函数
		virtual int32_t server_header(const char* in_buf, int32_t in_buf_size, const void* handle=NULL);

		// @函数名: 消息体解析函数
		virtual int32_t server_process(const char* rquest_buffer, int32_t request_len, char* reply_buffer, int32_t reply_size, \
				const void* handle=NULL);

		// @函数名: 业务逻辑处理函数
		virtual int32_t server_main(uint16_t type, const char* ptr_data, int32_t data_len, char* ptr_out, int32_t out_size, \
				const void* handle=NULL);

		// @函数名: 业务逻辑析构函数
		virtual int32_t server_free(const void* handle=NULL);

		// @函数名: 继承类资源释放函数
		virtual int32_t release();

	private:
		// @函数名: 图片存储函数
		int32_t save_image(const char* path, const char* data, int32_t len);

		// @函数名: 获取图片类型名
		const char* get_image_type_name(int32_t type);

	private:
		/* img directory */
		char*           img_path_;
		char*           img_dir_;
		int32_t         img_subdir_num_;
		/* mongo */
		QMongoClient*   mongo_client_;
		char*           mongo_uri_;
		char*           mongo_img_collection_;
		QMutexLock      mongo_mutex_;
};

#endif // __IDFSSERVER_H_
