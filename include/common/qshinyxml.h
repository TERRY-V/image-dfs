/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qshinyxml.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2014/11/06
**
*********************************************************************************************/

#ifndef __QSHINYXML_H_
#define __QSHINYXML_H_

#include "qglobal.h"

Q_BEGIN_NAMESPACE

#define DEFAULT_SHINY_ELEMENT_SIZE (1<<10)
#define DEFAULT_ERROR_SIZE (1<<10)

#define SHINY_CDATA_BEGIN ("<![CDATA[")
#define SHINY_CDATA_END ("]]>")
#define SHINY_CDATA_BEGIN_LEN (9)
#define SHINY_CDATA_END_LEN (3)

// XML结点定义
class ShinyNode {
		friend class QShinyXML;
	public:
		inline ShinyNode() :
			__ptr_item(0),
			__item_len(0),
			__ptr_label(0),
			__label_len(0),
			__ptr_text(0),
			__text_len(0)
		{}

		virtual ~ShinyNode()
		{}

		const char* GetLabel() const
		{return __ptr_label;}

		int32_t GetLabelLength() const
		{return __label_len;}

		const char* GetText() const
		{return __ptr_text;}

		int32_t GetTextLength() const
		{return __text_len;}

		void clear()
		{
			__ptr_item=NULL;
			__item_len=0;

			__ptr_label=NULL;
			__label_len=0;

			__ptr_text=NULL;
			__text_len=0;
		}

	private:
		char* __ptr_item;
		int32_t __item_len;

		char* __ptr_label;
		int32_t __label_len;

		char* __ptr_text;
		int32_t __text_len;
};

// QShinyXML数据解析类
class QShinyXML: public noncopyable {
	public:
		explicit QShinyXML(int32_t __size=DEFAULT_SHINY_ELEMENT_SIZE) :
			__now_node_size(0),
			__max_node_size(__size)
		{
			__error_str[0]='\0';
			__ptr_node_array=q_new_array<ShinyNode>(__max_node_size);
			Q_CHECK_PTR(__ptr_node_array);
		}

		virtual ~QShinyXML()
		{q_delete_array<ShinyNode>(__ptr_node_array);}

		int32_t parse(const char* __ptr_xml, int32_t __xml_len)
		{
			if(__ptr_xml==NULL) {
				__set_error("__ptr_xml is null!");
				return -1;
			}

			if(__xml_len<=0) {
				__set_error("__xml_len = (%d)", __xml_len);
				return -2;
			}

			char* __ptr=const_cast<char*>(__ptr_xml);
			char* __ptr_end=__ptr+__xml_len;

			int32_t __iret=0;
			__now_node_size=0;

			while(__ptr<__ptr_end) {
				if(*__ptr!='<') {++__ptr; continue;}

				__ptr_node_array[__now_node_size].clear();
				__iret=__parse_xml_item(__ptr, (int32_t)(__ptr_end-__ptr), &__ptr_node_array[__now_node_size]);
				if(__iret!=0)
					return __iret;

				__ptr+=__ptr_node_array[__now_node_size].__item_len;
				++__now_node_size;

				if(__now_node_size+1>__max_node_size) {
					__set_error("__max_node_size (%d) is too small!", __max_node_size);
					return -3;
				}
			}

			return __iret;
		}

		const char* strerr() const
		{return __error_str;}

		int32_t max_size() const
		{return __max_node_size;}

		int32_t size() const
		{return __now_node_size;}

		ShinyNode& operator[](int32_t __pos)
		{
			Q_ASSERT(__pos>=0&&__pos<=__now_node_size, "QShinyXML: out of range, __pos = (%d)", __pos);
			return __ptr_node_array[__pos];
		}

	private:
		int32_t __parse_xml_item(const char* __ptr_pack, int32_t __pack_len, ShinyNode* __ptr_node)
		{
			char* __ptr=const_cast<char*>(__ptr_pack);
			char* __ptr_end=__ptr+__pack_len;

			if(*__ptr!='<')
				return -31;

			__ptr_node->__ptr_item=__ptr;
			__ptr++;
			__ptr_node->__ptr_label=__ptr;

			// xml结点标签可能带参数
			while(__ptr<__ptr_end&&*__ptr!='>'&&*__ptr!=0x20)
				__ptr++;
			if(__ptr>=__ptr_end) {
				__set_error("lack of '>' to match '<'");
				return -32;
			}
			// XML结点标签不带参数
			if(*__ptr=='>') {
				char* __ptr_tmp=__ptr+1;
				while(__ptr_tmp<__ptr_end&&*__ptr_tmp!='<')
					__ptr_tmp++;
				// 看来是最后一个结点了
				if(__ptr_tmp>=__ptr_end) {
					__ptr_node->__item_len=(int32_t)(__ptr+1-__ptr_node->__ptr_item);
					__ptr_node->__label_len=(int32_t)(__ptr-__ptr_node->__ptr_label);
					return 0;
				}
			}

			// 空格, 说明带有参数
			__ptr_node->__label_len=(int32_t)(__ptr-__ptr_node->__ptr_label);
			if(__ptr_node->__label_len<=0) {
				__set_error("__ptr_node->__label_len = (%d)", __ptr_node->__label_len);
				return -34;
			}

			// XML结点标签参数信息
			while(__ptr<__ptr_end&&*__ptr!='>')
				__ptr++;
			if(__ptr>=__ptr_end) {
				__set_error("lack of '>' to match '<'");
				return -35;
			}
			__ptr++;

			// 判断是否为闭标签<tag/>形式
			if(*(__ptr-2)=='/') {
				__ptr_node->__item_len=(int32_t)(__ptr-__ptr_node->__ptr_item);
				return 0;
			}

			// 跳过空白字符
			while(__ptr<__ptr_end&&__is_whitespace(*__ptr))
				__ptr++;
			// 哈哈, 看来这是最后一个标签了!
			if(__ptr>=__ptr_end) {
				__ptr_node->__item_len=(int32_t)(__ptr-__ptr_node->__ptr_item);
				return 0;
			}

			// 判断是否为CDATA数据
			if(strncmp(__ptr, SHINY_CDATA_BEGIN, SHINY_CDATA_BEGIN_LEN)!=0) {
				// 说明这并非是CDATA数据
				if(__ptr<__ptr_end&&*__ptr=='<') {
					__ptr_node->__item_len=(int32_t)(__ptr-__ptr_node->__ptr_item);
					return 0;
				}
			}

			__ptr_node->__ptr_text=__ptr;

			while(__ptr<__ptr_end) {
				// 寻找对应的结束标签
				if(*__ptr=='<'&&__ptr+2+__ptr_node->__label_len<__ptr_end \
						&&*(__ptr+1)=='/'&&strncmp(__ptr+2, __ptr_node->__ptr_label, __ptr_node->__label_len)==0) {
					__ptr_node->__text_len=(int32_t)(__ptr-__ptr_node->__ptr_text);
					// 检查是否为CDATA数据
					if(strncmp(__ptr_node->__ptr_text, SHINY_CDATA_BEGIN, SHINY_CDATA_BEGIN_LEN)==0) {
						char* __ptr_cdata_end=__ptr_node->__ptr_text+__ptr_node->__text_len;
						if(strncmp(__ptr_cdata_end-SHINY_CDATA_END_LEN, SHINY_CDATA_END, SHINY_CDATA_END_LEN)!=0) {
							__set_error("tag(%.*s) lack of CDATA end mark!", __ptr_node->__label_len, __ptr_node->__ptr_label);
							return -36;
						}
						__ptr_node->__ptr_text+=SHINY_CDATA_BEGIN_LEN;
						__ptr_node->__text_len-=(SHINY_CDATA_BEGIN_LEN+SHINY_CDATA_END_LEN);
					}
					// 跳过结束标签
					__ptr+=(2+__ptr_node->__label_len);
					break;
				}
				__ptr++;
			}

			// 没有找到结束标签
			if(__ptr>=__ptr_end) {
				__set_error("tag(%.*s) lack of end tag!", __ptr_node->__label_len, __ptr_node->__ptr_label);
				return -37;
			}

			// 检查结束标签尾部的>符号
			if(*__ptr!='>') {
				__set_error("tag(%.*s) lack of '>' to match '<'", __ptr_node->__label_len, __ptr_node->__ptr_label);
				return -38;
			}
			__ptr++;

			// 获取item的长度
			__ptr_node->__item_len=(int32_t)(__ptr-__ptr_node->__ptr_item);

			return 0;
		}

		bool __is_whitespace(const char ch)
		{return (ch==0x20||ch==0x09||ch==0x0d||ch==0x0a);}

		void __set_error(const char* format, ...)
		{
			va_list args;
			va_start(args, format);
			vsnprintf(__error_str, sizeof(__error_str), format, args);
			va_end(args);
		}

	protected:
		ShinyNode* __ptr_node_array;
		int32_t __now_node_size;
		int32_t __max_node_size;
		char __error_str[DEFAULT_ERROR_SIZE];
};

Q_END_NAMESPACE

#endif // __QSHINYXML_H_
