/********************************************************************************************
**
** Copyright (C) 2010-2015 Terry Niu (Beijing, China)
** Filename:	qprotobuffer.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2015/04/05
**
*********************************************************************************************/

#ifndef __QPROTOBUFFER_H_
#define __QPROTOBUFFER_H_

#include "qglobal.h"

Q_BEGIN_NAMESPACE

class QProtoBuffer {
	public:
		inline QProtoBuffer(const int64_t length=1<<10)
		{if(length>0) buffer_.expand(length);}
		virtual ~QProtoBuffer()
		{}

		char* get_data() const
		{return buffer_.get_data();}
		int64_t get_data_length() const
		{return buffer_.get_data_length();}

		char* get_free() const
		{return buffer_.get_free();}
		int64_t get_free_length() const
		{return buffer_.get_free_length();}

		inline int32_t drain(const int64_t length)
		{return buffer_.drain(length);}
		inline int32_t pour(const int64_t length)
		{return buffer_.pour(length);}

		int32_t set_protocol_version(const uint64_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(uint64_t))
				buffer_.expand(sizeof(uint64_t));
			int32_t iret=QSerialization::set_int64(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(uint64_t));
			return iret;
		}

		int32_t get_protocol_version(int64_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int64(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int64_t));
			return iret;
		}

		int32_t set_protocol_body_length(const int32_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(int32_t))
				buffer_.expand(sizeof(int32_t));
			int32_t iret=QSerialization::set_int32(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(int32_t));
			return iret;
		}

		int32_t get_protocol_body_length(int32_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int32(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int32_t));
			return iret;
		}

		int32_t set_protocol_type(const int16_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(int16_t))
				buffer_.expand(sizeof(int16_t));
			int32_t iret=QSerialization::set_int16(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(int16_t));
			return iret;
		}

		int32_t get_protocol_type(int16_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int16(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int16_t));
			return iret;
		}

		int32_t set_source_type(const int16_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(int16_t))
				buffer_.expand(sizeof(int16_t));
			int32_t iret=QSerialization::set_int16(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(int16_t));
			return iret;
		}

		int32_t get_source_type(int16_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int16(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int16_t));
			return iret;
		}

		int32_t set_bytes(const void* data, const int64_t length)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<length)
				buffer_.expand(length);
			int32_t iret=QSerialization::set_bytes(buffer_.get_free(), buffer_.get_free_length(), pos, data, length);
			if(iret==0)
				buffer_.pour(length);
			return iret;
		}

		int32_t get_bytes(void* data, const int64_t length)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_bytes(buffer_.get_data(), buffer_.get_data_length(), pos, data, length);
			if(iret==0)
				buffer_.drain(length);
			return iret;
		}

		int32_t set_command_type(const int16_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(int16_t))
				buffer_.expand(sizeof(int16_t));
			int32_t iret=QSerialization::set_int16(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(int16_t));
			return iret;
		}

		int32_t get_command_type(int16_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int16(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int16_t));
			return iret;
		}

		int32_t set_operate_type(const int16_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(int16_t))
				buffer_.expand(sizeof(int16_t));
			int32_t iret=QSerialization::set_int16(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(int16_t));
			return iret;
		}

		int32_t get_operate_type(int16_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int16(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int16_t));
			return iret;
		}

		int32_t set_content_length(const int32_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(int32_t))
				buffer_.expand(sizeof(int32_t));
			int32_t iret=QSerialization::set_int32(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(int32_t));
			return iret;
		}

		int32_t get_content_type(int16_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int16(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int16_t));
			return iret;
		}

		int32_t set_content_bytes(const void* data, const int64_t length)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<length)
				buffer_.expand(length);
			int32_t iret=QSerialization::set_bytes(buffer_.get_free(), buffer_.get_free_length(), pos, data, length);
			if(iret==0)
				buffer_.pour(length);
			return iret;
		}

		int32_t get_content_bytes(void* data, const int64_t length)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_bytes(buffer_.get_data(), buffer_.get_data_length(), pos, data, length);
			if(iret==0)
				buffer_.drain(length);
			return iret;
		}

		int32_t set_extended_length(const int32_t value)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<(int64_t)sizeof(int32_t))
				buffer_.expand(sizeof(int32_t));
			int32_t iret=QSerialization::set_int32(buffer_.get_free(), buffer_.get_free_length(), pos, value);
			if(iret==0)
				buffer_.pour(sizeof(int32_t));
			return iret;
		}

		int32_t get_extended_type(int16_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int16(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int16_t));
			return iret;
		}

		int32_t set_extended_bytes(const void* data, const int64_t length)
		{
			int64_t pos=0;
			if(buffer_.get_free_length()<length)
				buffer_.expand(length);
			int32_t iret=QSerialization::set_bytes(buffer_.get_free(), buffer_.get_free_length(), pos, data, length);
			if(iret==0)
				buffer_.pour(length);
			return iret;
		}

		int32_t get_extended_bytes(void* data, const int64_t length)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_bytes(buffer_.get_data(), buffer_.get_data_length(), pos, data, length);
			if(iret==0)
				buffer_.drain(length);
			return iret;
		}

		int32_t get_status(int32_t* value)
		{
			int64_t pos=0;
			int32_t iret=QSerialization::get_int32(buffer_.get_data(), buffer_.get_data_length(), pos, value);
			if(iret==0)
				buffer_.drain(sizeof(int32_t));
			return iret;
		}

	private:
		void expand(const int64_t length)
		{buffer_.expand(length);}

		void clear()
		{buffer_.clear();}

	protected:
		Q_DISABLE_COPY(QProtoBuffer);
		QBuffer buffer_;
};

Q_END_NAMESPACE

#endif // __QPROTOBUFFER_H_
