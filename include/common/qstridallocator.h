/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qstridallocator.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2014/05/23
**
*********************************************************************************************/

#ifndef __QSTRIDALLOCATOR_H_
#define __QSTRIDALLOCATOR_H_

#include "qglobal.h"
#include "qhashsearch.h"
#include "qallocator.h"
#include "qmd5.h"

Q_BEGIN_NAMESPACE

class QStrIDAllocator {
	public:
		QStrIDAllocator()
		{}

		virtual ~QStrIDAllocator()
		{}

		// @函数名: 初始化函数
		// @参数01: 允许的最多标签个数
		// @参数02: 数据文件路径
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(uint32_t max_str_num, const char* save_file)
		{
			if(max_str_num==0||save_file==NULL)
				return -1;

			m_max_str_num=max_str_num;
			strcpy(m_save_file, save_file);

			m_i2s_map=new(std::nothrow) char*[m_max_str_num];
			if(m_i2s_map==NULL)
				return -2;
			memset(m_i2s_map, 0, m_max_str_num*sizeof(void*));

			int32_t hash_size=(m_max_str_num/2)+1;
			if(m_s2i_map.init(hash_size, sizeof(void*)))
				return -3;

			m_file_len=0;
			m_cur_str_num=0;

			if(access(m_save_file, 0)) {
				m_save_fp=fopen(m_save_file, "wb+");
				if(m_save_fp==NULL)
					return -4;
				return 0;
			}

			m_save_fp=fopen(m_save_file, "rb+");
			if(m_save_fp==NULL)
				return -5;

			char read_buf[16];
			Q_FOREVER {
				if(fread(read_buf, sizeof(read_buf), 1, m_save_fp)!=1)
					break;

				int32_t size=16+*(int32_t*)(read_buf+12)+8;
				char* ptr=m_alloc.alloc(size);
				if(ptr==NULL)
					return -6;
				memcpy(ptr, read_buf, 16);

				if(fread(ptr+16, *(int32_t*)(ptr+12)+8, 1, m_save_fp)!=1)
					return -7;

				assert(m_cur_str_num==*(uint32_t*)ptr);

				if(m_s2i_map.addKey_FL(*(uint64_t*)(ptr+4), &ptr)<0)
					return -8;

				m_i2s_map[m_cur_str_num++]=ptr;
				m_file_len+=size;
			}

			assert(m_file_len==ftell(m_save_fp));

			return 0;
		}

		// @函数名: 通过字符串获取映射ID
		// @参数01: 字符串指针
		// @参数02: 字符串长度
		// @参数03: 映射ID
		// @返回值: 成功返回0, 失败返回<0的错误码, 重复返回1
		int32_t str2id(const char* str, uint32_t str_len, uint32_t& id)
		{
			if(str==NULL||str_len==0)
				return -1;

			uint64_t md5=m_md5.MD5Bits64((unsigned char*)str, str_len);

			m_locker.lock();

			void* val=NULL;
			if(m_s2i_map.searchKey_FL(md5, &val)==0) {
				id=**(uint32_t**)val;
				m_locker.unlock();
				return 1;
			}

			if(m_cur_str_num>=m_max_str_num) {
				m_locker.unlock();
				return -2;
			}

			uint32_t size=4+8+4+str_len+8;	// id, md5, len, str, flg

			char* buf=m_alloc.alloc(size);
			if(buf==NULL) {
				m_locker.unlock();
				return -3;
			}

			*(uint32_t*)buf=m_cur_str_num;
			*(uint64_t*)(buf+4)=md5;
			*(uint32_t*)(buf+12)=str_len;
			memcpy(buf+16, str, str_len);
			*(uint64_t*)(buf+size-8)=*(uint64_t*)"!@#$$#@!";

			if(fwrite(buf, size, 1, m_save_fp)!=1) {
				m_locker.unlock();
				return -4;
			}

			if(fflush(m_save_fp)) {
				m_locker.unlock();
				return -5;
			}

			if(m_s2i_map.addKey_FL(md5, (void*)&buf)) {
				m_locker.unlock();
				return -6;
			}

			m_file_len+=size;
			m_i2s_map[m_cur_str_num]=buf;
			id=m_cur_str_num++;
			m_locker.unlock();

			return 0;
		}

		// @函数名: 通过ID获取映射字符串
		// @参数01: ID号
		// @参数02: 字符串指针
		// @参数03: 字符串长度
		// @参数04: md5值
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t id2str(uint32_t id, const char*& str, uint32_t& str_len, uint64_t& md5)
		{
			if(id>=m_cur_str_num)
				return -1;
			
			char* buf=m_i2s_map[id];
			if(buf==NULL)
				return -2;

			assert(id==*(uint32_t*)buf);

			md5=*(uint64_t*)(buf+4);
			str_len=*(uint32_t*)(buf+12);
			str=(char*)(buf+16);

			return 0;
		}

	private:
		uint32_t m_max_str_num;
		uint32_t m_cur_str_num;

		char m_save_file[256];
		FILE* m_save_fp;

		int64_t m_file_len;

		char** m_i2s_map;
		QHashSearch<uint64_t> m_s2i_map;

		QAllocator<char> m_alloc;

		QMutexLock m_locker;
		QMD5 m_md5;
};

Q_END_NAMESPACE

#endif // __QSTRIDALLOCATOR_H_
