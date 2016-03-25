/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qxml.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2014/11/29
**
*********************************************************************************************/

#ifndef __QXML_H_
#define __QXML_H_

#include "qglobal.h"

Q_BEGIN_NAMESPACE

Q_FORWARD_CLASS(XMLDocument)
Q_FORWARD_CLASS(XMLElement)
Q_FORWARD_CLASS(XMLAttribute)
Q_FORWARD_CLASS(XMLComment)
Q_FORWARD_CLASS(XMLText)
Q_FORWARD_CLASS(XMLDeclaration)
Q_FORWARD_CLASS(XMLUnknown)

// XML错误信息
enum XMLError {
	XML_NO_ERROR=0,
	XML_SUCCESS=0,
	XML_ERROR_EMPTY_STR,
	XML_ERROR_ELEMENT_MISMATCH,
	XML_ERROR_PARSING_ELEMENT,
	XML_ERROR_PARSING_ATTRIBUTE,
	XML_ERROR_PARSING_TEXT,
	XML_ERROR_PARSING_CDATA,
	XML_ERROR_PARSING_COMMENT,
	XML_ERROR_PARSING_DECLARATION,
	XML_ERROR_PARSING_UNKNOWN,
	XML_ERROR_EMPTY_DOCUMENT,
	XML_ERROR_MISMATCHED_ELEMENT,
	XML_ERROR_PARSING,
};

class XMLUtil {
	public:
		static inline char* SkipWhiteSpace(char* p)
		{
			while(isspace(*reinterpret_cast<unsigned char*>(p)))
				++p;
			return p;
		}

		static inline bool isWhiteSpace(char* p)
		{return isspace(static_cast<unsigned char>(*p));}

		static inline bool IsNameStartChar(unsigned char ch)
		{return ((ch<128)?isalpha(ch):1)||ch==':'||ch=='_';}

		static inline bool IsNameChar(unsigned char ch)
		{return IsNameStartChar(ch)||isdigit(ch)||ch=='.'||ch=='-';}

		static inline bool StringEqual(const char* p, const char* q, int32_t nChar=MAX_INT32)
		{
			if(p==q) return true;
			int32_t n=0;
			while(*p&&*q&&*p==*q&&n<nChar) {
				++p;
				++q;
				++n;
			}
			if(n==nChar||(*p==0&&*q==0))
				return true;
			return false;
		}
};

// XML字符串
class XMLStr {
	public:
		inline XMLStr() :
			_interned(true),
			_start(0),
			_end(0)
		{}

		virtual ~XMLStr()
		{Reset();}

		void SetStr(const char* str, int32_t len=-1, bool interned=false);

		const char* GetStr() const
		{return _start;}

		char* ParseName(char* p);

		char* ParseText(char*p, const char* endTag);

		bool Empty() const
		{return _start==_end;}

	private:
		Q_DISABLE_COPY(XMLStr);

		void Reset();

	protected:
		bool _interned;
		char* _start;
		char* _end;
};

// XML结点信息
class XMLNode {
	friend class XMLDocument;
	friend class XMLElement;

	public:
		XMLDocument* GetDocument()
		{return _document;}

		virtual XMLElement* ToElement()
		{return 0;}

		virtual XMLText* ToText()
		{return 0;}

		virtual XMLComment* ToComment()
		{return 0;}

		virtual XMLDocument* ToDocument()
		{return 0;}

		virtual XMLDeclaration* ToDeclaration()
		{return 0;}

		virtual XMLUnknown* ToUnknown()
		{return 0;}

		const char* Value() const
		{return _value.GetStr();}

		void SetValue(const char* val)
		{_value.SetStr(val);}

		XMLNode* Parent()
		{return _parent;}

		XMLNode* FirstChild()
		{return _firstChild;}

		XMLNode* LastChild()
		{return _lastChild;}

		XMLNode* PreviousSibling()
		{return _prev;}

		XMLNode* NextSibling()
		{return _next;}

		XMLNode* LinkEndChild(XMLNode* addThis)
		{return InsertEndChild(addThis);}

		XMLNode* InsertEndChild(XMLNode* addThis)
		{
			if(addThis->_document!=_document)
				return 0;

			if(addThis->_parent)
				addThis->_parent->Unlink(addThis);
			if(_lastChild) {
				Q_ASSERT(_firstChild, "_firstChild is null!");
				Q_ASSERT(_lastChild->_next==0, "_lastChild->_next is not null!");
				_lastChild->_next=addThis;
				addThis->_prev=_lastChild;
				_lastChild=addThis;
				addThis->_next=0;
			} else {
				Q_ASSERT(_firstChild==0, "_firstChild is not null!");
				_firstChild=_lastChild=addThis;
				addThis->_prev=0;
				addThis->_next=0;
			}
			addThis->_parent=this;
			return addThis;
		}

		XMLElement* FirstChildElement(const char* val=0)
		{
			for(XMLNode* node=_firstChild; node; node=node->_next) {
				if(node->ToElement()&&(!val||XMLUtil::StringEqual(node->Value(), val)))
					return node->ToElement();
			}
			return 0;
		}

		XMLElement* NextSiblingElement(const char* val=0)
		{
			for(XMLNode* node=this->_next; node; node=node->_next) {
				if(node->ToElement()&&(!val||XMLUtil::StringEqual(node->Value(), val)))
					return node->ToElement();
			}
			return 0;
		}

		void DeleteChildren()
		{
			while(_firstChild) {
				XMLNode* node=_firstChild;
				Unlink(node);
				node->~XMLNode();
			}
			_firstChild=_lastChild=0;
		}

		void Unlink(XMLNode* child)
		{
			if(child==_firstChild) _firstChild=_firstChild->_next;
			if(child==_lastChild) _lastChild=_lastChild->_prev;
			if(child->_prev) child->_prev->_next=child->_next;
			if(child->_next) child->_next->_prev=child->_prev;
			child->_parent=0;
		}

		virtual char* ParseDeep(char* p, XMLStr* parentEnd);

	protected:
		XMLNode(XMLDocument* doc) :
			_document(doc),
			_parent(0),
			_firstChild(0),
			_lastChild(0),
			_prev(0),
			_next(0)
		{}

		virtual ~XMLNode()
		{
			DeleteChildren();
			if(_parent) _parent->Unlink(this);
		}

		XMLDocument* _document;
		XMLNode* _parent;
		XMLStr _value;

		XMLNode* _firstChild;
		XMLNode* _lastChild;

		XMLNode* _prev;
		XMLNode* _next;
};

// XML属性信息
class XMLAttribute {
	friend class XMLElement;

	public:
		const char* Name() const
		{return _name.GetStr();}

		const char* Value() const
		{return _value.GetStr();}

		XMLAttribute* Next() const
		{return _next;}

		void SetName(const char* name)
		{_name.SetStr(name);}

		void SetAttribute(const char* value)
		{_value.SetStr(value);}

	protected:
		XMLAttribute() :
			_next(0)
		{}

		virtual ~XMLAttribute()
		{}

		char* ParseDeep(char* p);

		XMLStr _name;
		XMLStr _value;
		XMLAttribute* _next;
};

// XML文本信息
class XMLText: public XMLNode {
	friend class XMLDocument;

	public:
		virtual XMLText* ToText()
		{return this;}

		void setCData(bool isCData)
		{_isCData=isCData;}

		bool CData() const
		{return _isCData;}

		char* ParseDeep(char* p, XMLStr* endTag);

	protected:
		XMLText(XMLDocument* doc) :
			XMLNode(doc),
			_isCData(false)
		{}

		virtual ~XMLText()
		{}

	private:
		bool _isCData;
};

// XML注释信息
class XMLComment: public XMLNode {
	friend class XMLDocument;

	public:
		virtual XMLComment* ToComment()
		{return this;}

		char* ParseDeep(char* p, XMLStr* endTag);

	protected:
		XMLComment(XMLDocument* doc) :
			XMLNode(doc)
		{}

		virtual ~XMLComment()
		{}
};

// XML声明信息
class XMLDeclaration: public XMLNode {
	friend class XMLDocument;

	public:
		virtual  XMLDeclaration* ToDeclaration()
		{return this;}

		char* ParseDeep(char* p, XMLStr* endTag);

	protected:
		XMLDeclaration(XMLDocument* doc) :
			XMLNode(doc)
		{}

		virtual ~XMLDeclaration()
		{}
};

// XML未知信息 --> 专门解析dtd数据
class XMLUnknown: public XMLNode {
	friend class XMLDocument;

	public:
		virtual XMLUnknown* ToUnknown()
		{return this;}

		char* ParseDeep(char* p, XMLStr* endTag);

	protected:
		XMLUnknown(XMLDocument* doc) :
			XMLNode(doc)
		{}

		virtual ~XMLUnknown()
		{}
};

// XML元素信息
class XMLElement: public XMLNode {
	friend class XMLDocument;

	public:
		const char* Name() const
		{return Value();}

		void SetName(const char* str)
		{SetValue(str);}

		virtual XMLElement* ToElement()
		{return this;}

		const char* Attribute(const char* name, const char* value=0)
		{
			XMLAttribute* a=FindAttribute(name);
			if(!a)
				return 0;
			if(!value||XMLUtil::StringEqual(a->Value(), value))
				return a->Value();
			return 0;
		}

		void SetAttribute(const char* name, const char* value)
		{
			XMLAttribute* a=FindOrCreateAttribute(name);
			a->SetAttribute(value);
		}

		void DeleteAttribute(const char* name)
		{
			XMLAttribute* prev=0;
			for(XMLAttribute* a=_rootAttribute; a; a=a->_next) {
				if(!XMLUtil::StringEqual(name, a->Name())) {
					if(prev) {
						prev->_next=a->_next;
					} else {
						_rootAttribute=a->_next;
					}
					a->~XMLAttribute();
					break;
				}
				prev=a;
			}
		}

		XMLAttribute* FirstAttribute()
		{return _rootAttribute;}

		const char* GetText()
		{
			if(FirstChild()&&FirstChild()->ToText())
				return FirstChild()->ToText()->Value();
			return 0;
		}

		enum {
			OPEN,		// <foo>
			CLOSED,		// <foo/>
			CLOSING		// </foo>
		};

		int32_t ClosingType() const
		{return _closingType;}

		char* ParseDeep(char* p, XMLStr* endTag);

	private:
		XMLElement(XMLDocument* doc) :
			XMLNode(doc),
			_closingType(0),
			_rootAttribute(0)
		{}

		virtual ~XMLElement()
		{
			while(_rootAttribute) {
				XMLAttribute* next=_rootAttribute->_next;
				_rootAttribute->~XMLAttribute();
				_rootAttribute=next;
			}
		}

		XMLAttribute* FindAttribute(const char* name)
		{
			XMLAttribute* a=0;
			for(a=_rootAttribute; a; a=a->_next) {
				if(XMLUtil::StringEqual(a->Name(), name))
					return a;
			}
			return 0;
		}

		XMLAttribute* FindOrCreateAttribute(const char* name)
		{
			XMLAttribute* last=0;
			XMLAttribute* attrib=0;
			for(attrib=_rootAttribute; attrib; last=attrib, attrib=attrib->_next) {
				if(XMLUtil::StringEqual(attrib->Name(), name))
					break;
			}
			if(!attrib) {
				attrib=new(std::nothrow) XMLAttribute();
				Q_CHECK_PTR(attrib);
				if(last) {
					last->_next=attrib;
				} else {
					_rootAttribute=attrib;
				}
				attrib->SetName(name);
			}
			return attrib;
		}

		char* ParseAttributes(char* p);

		int32_t _closingType;
		XMLAttribute* _rootAttribute;
};

// XML文档信息
class XMLDocument: public XMLNode {
	friend class XMLElement;

	public:
		XMLDocument() :
			XMLNode(0),
			_errorID(XML_NO_ERROR),
			_errorStr(0),
			_charBuffer(0)
		{_document=this;}

		virtual ~XMLDocument()
		{
			DeleteChildren();
			q_delete_array<char>(_charBuffer);
			q_delete_array<char>(_errorStr);
		}

		virtual XMLDocument* ToDocument()
		{return this;}

		int32_t Parse(const char* xml, int32_t nBytes=-1);

		const char* ToXML();

		XMLDeclaration* Declaration()
		{
			if(_document->FirstChild()&&(_document->FirstChild()->ToDeclaration()))
				return _document->FirstChild()->ToDeclaration();
			return 0;
		}

		XMLElement* RootElement()
		{return FirstChildElement();}

		void clear()
		{
			DeleteChildren();
			_errorID=XML_NO_ERROR;
			q_delete_array<char>(_errorStr);
			q_delete_array<char>(_charBuffer);
		}

		XMLDeclaration* NewDeclaration(const char* str=0)
		{
			XMLDeclaration* dec=new(std::nothrow) XMLDeclaration(this);
			Q_CHECK_PTR(dec);
			dec->SetValue(str?str:"xml version=\"1.0\" encoding=\"UTF-8\"");
			return dec;
		}

		XMLComment* NewComment(const char* str)
		{
			XMLComment* comment=new(std::nothrow) XMLComment(this);
			Q_CHECK_PTR(comment);
			comment->SetValue(str);
			return comment;
		}

		XMLElement* NewElement(const char* name)
		{
			XMLElement* ele=new(std::nothrow) XMLElement(this);
			Q_CHECK_PTR(ele);
			ele->SetName(name);
			return ele;
		}

		XMLText* NewText(const char* str)
		{
			XMLText* text=new(std::nothrow) XMLText(this);
			Q_CHECK_PTR(text);
			text->SetValue(str);
			return text;
		}

		XMLUnknown* NewUnknown(const char* str)
		{
			XMLUnknown* unknown=new(std::nothrow) XMLUnknown(this);
			Q_CHECK_PTR(unknown);
			unknown->SetValue(str);
			return unknown;
		}

		void SetError(XMLError error, const char* format, ...)
		{
			_errorID=error;
			if(!_errorStr) _errorStr=q_new_array<char>(1<<10);	// enough!
			Q_CHECK_PTR(_errorStr);
			va_list args;
			va_start(args, format);
			vsnprintf(_errorStr, 1<<10, format, args);
			va_end(args);
		}

		bool Error() const
		{return _errorID!=XML_NO_ERROR;}

		const char* GetError() const
		{return _errorStr;}

		char* Identify(char* p, XMLNode** node);

	private:
		void ToXML(XMLElement* element, QStringBuffer& _xml);

	protected:
		Q_DISABLE_COPY(XMLDocument);

		XMLError _errorID;
		char* _errorStr;
		char* _charBuffer;
};

Q_END_NAMESPACE

#endif // __QXML_H_
