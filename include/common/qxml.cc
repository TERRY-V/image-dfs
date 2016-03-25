#include "qxml.h"

Q_BEGIN_NAMESPACE

void XMLStr::SetStr(const char* str, int32_t len, bool interned)
{
	Reset();
	_interned=interned;
	len=(len==-1)?strlen(str):len;
	if(!_interned) {
		_start=q_new_array<char>(len+1);
		Q_CHECK_PTR(_start);
		memcpy(_start, str, len);
		*(_start+len)=0;	// very import!!!!!
		_end=_start+len;
	} else {
		_start=const_cast<char*>(str);
		_end=_start+len;
	}
}

char* XMLStr::ParseName(char* p)
{
	char* start=p;
	if(!start||!(*start))
		return 0;
	while(*p&&(p==start?XMLUtil::IsNameStartChar(*p):XMLUtil::IsNameChar(*p)))
		++p;
	if(p>start) {
		SetStr(start, p-start);
		return p;
	}
	return 0;
}

char* XMLStr::ParseText(char*p, const char* endTag)
{
	Q_CHECK_PTR(endTag);
	Q_ASSERT(*endTag, "endTag is empty!");

	char* start=p;
	char endChar=*endTag;
	int32_t length=strlen(endTag);

	while(*p) {
		if((*p==endChar)&&!strncmp(p, endTag, length)) {
			SetStr(start, p-start);
			return p+length;
		}
		++p;
	}
	return 0;
}

void XMLStr::Reset()
{
	if(!_interned) q_delete_array<char>(_start);
	_interned=0;
	_start=0;
	_end=0;
}

// XMLode相关函数
char* XMLNode::ParseDeep(char* p, XMLStr* parentEnd)
{
#if defined (XML_DEBUG)
	Q_INFO("XMLNode Parsing:\t{%.*s...}", 1<<6, p);
#endif
	while(p&&*p) {
#if defined (XML_DEBUG)
		Q_INFO("XMLNode Identify:\t(%.*s...)", 1<<6, p);
#endif

		XMLNode* node=0;
		p=_document->Identify(p, &node);
		if(p==0||node==0)
			break;

		XMLStr endTag;
		p=node->ParseDeep(p, &endTag);
		if(!p) {
			node->~XMLNode();
			node=0;
			if(!_document->Error())
				_document->SetError(XML_ERROR_PARSING, "XML_ERROR_PARSING: xml parsing error!");
			break;
		}

		if(node->ToElement()&&node->ToElement()->ClosingType()==XMLElement::CLOSING) {
			if(parentEnd)
				parentEnd->SetStr(static_cast<XMLElement*>(node)->_value.GetStr());
#if defined (XML_DEBUG)
			Q_INFO("XMLNode parentEnd:\t(%s)", parentEnd->GetStr());
#endif
			node->~XMLNode();
			return p;
		}

#if defined (XML_DEBUG)
		Q_INFO("XMLNode endTag:\t(%s)", endTag.GetStr());
#endif
		XMLElement* ele=node->ToElement();
		if(ele) {
			if(endTag.Empty()&&ele->ClosingType()==XMLElement::OPEN) {
				_document->SetError(XML_ERROR_MISMATCHED_ELEMENT, "XML_ERROR_MISMATCHED_ELEMENT: endTag is empty!");
				p=0;
			} else if(!endTag.Empty()&&ele->ClosingType()!=XMLElement::OPEN) {
				_document->SetError(XML_ERROR_MISMATCHED_ELEMENT, "XML_ERROR_MISMATCHED_ELEMENT: endTag = (%s), ClosingType != OPEN!", \
						endTag.GetStr());
				p=0;
			} else if(!endTag.Empty()) {
				if(!XMLUtil::StringEqual(endTag.GetStr(), node->Value())) {
					_document->SetError(XML_ERROR_MISMATCHED_ELEMENT, "XML_ERROR_MISMATCHED_ELEMENT: endTag = (%s), node->Value() = (%s)!", \
							endTag.GetStr(), node->Value());
					p=0;
				}
			}
		}
		if(p==0) {
			node->~XMLNode();
			node=0;
		}
		if(node) {
			this->InsertEndChild(node);
		}
	}
	return 0;
}

// XML Attribute
char* XMLAttribute::ParseDeep(char* p)
{
	p=_name.ParseName(p);
	if(!p||!*p)
		return 0;

	p=XMLUtil::SkipWhiteSpace(p);
	if(!p||*p!='=')
		return 0;

	++p;
	p=XMLUtil::SkipWhiteSpace(p);
	if(*p!='\"'&&*p!='\'')
		return 0;

	char endTag[2]={*p, 0};
	++p;

	p=_value.ParseText(p, endTag);
	return p;
}

// XML Text
char* XMLText::ParseDeep(char* p, XMLStr* endTag)
{
	if(this->CData()) {
		p=_value.ParseText(p, "]]>");
		if(!p)
			_document->SetError(XML_ERROR_PARSING_CDATA, "XML_ERROR_PARSING_CDATA: xml parsing CDATA error!");
#if defined (XML_DEBUG)
		Q_INFO("XMLText ParseText:\t(CDATA text = %s)", _value.GetStr());
#endif
		return p;
	} else {
		p=_value.ParseText(p, "<");
		if(!p)
			_document->SetError(XML_ERROR_PARSING_TEXT, "XML_ERROR_PARSING_TEXT: xml parsing text error!");
#if defined (XML_DEBUG)
		Q_INFO("XMLText ParseText:\t(text = %s)", _value.GetStr());
#endif
		if(p&&*p)
			return p-1;
	}
	return 0;
}

// XMLComment
char* XMLComment::ParseDeep(char* p, XMLStr* endTag)
{
	p=_value.ParseText(p, "-->");
	if(!p) {
		_document->SetError(XML_ERROR_PARSING_COMMENT, "XML_ERROR_PARSING_COMMENT: xml parsing comment error!");
	}
	return p;
}

// XMLDeclaration
char* XMLDeclaration::ParseDeep(char* p, XMLStr* endTag)
{
	p=_value.ParseText(p, "?>");
	if(!p)
		_document->SetError(XML_ERROR_PARSING_DECLARATION, "XML_ERROR_PARSING_DECLARATION: xml parsing declaration error!");
	return p;
}

// XMLUnknown
char* XMLUnknown::ParseDeep(char* p, XMLStr* endTag)
{
	p=_value.ParseText(p, ">");
	if(!p) {
		_document->SetError(XML_ERROR_PARSING_UNKNOWN, "XML_ERROR_PARSING_UNKNOWN: xml parsing unknown error!");
	}
	return p;
}

// XMLElement
char* XMLElement::ParseDeep(char* p, XMLStr* endTag)
{
	p=XMLUtil::SkipWhiteSpace(p);
	if(!p)
		return 0;
	if(*p=='/') {
#if defined (XML_DEBUG)
		Q_INFO("XMLElement ParseDeep:\tCLOSING");
#endif
		_closingType=CLOSING;
		++p;
	}

	p=_value.ParseName(p);
	if(_value.Empty())
		return 0;
#if defined (XML_DEBUG)
	Q_INFO("XMLElement ParseDeep:\t(name = %s)", _value.GetStr());
#endif

	p=ParseAttributes(p);
	if(!p||!*p||_closingType)
		return p;

	p=XMLNode::ParseDeep(p, endTag);
	return p;
}

// XMLAttribute
char* XMLElement::ParseAttributes(char* p)
{
	XMLAttribute* prevAttribute=0;

	while(p) {
		p=XMLUtil::SkipWhiteSpace(p);
		if(!p||!(*p)) {
			_document->SetError(XML_ERROR_PARSING_ELEMENT, "XML_ERROR_PARSING_ELEMENT: xml parsing element error!");
			return 0;
		}

		if(XMLUtil::IsNameStartChar(*p)) {
			XMLAttribute* attrib=new(std::nothrow) XMLAttribute();
			Q_CHECK_PTR(attrib);
			p=attrib->ParseDeep(p);
			if(!p||Attribute(attrib->Name())) {
				attrib->~XMLAttribute();
				_document->SetError(XML_ERROR_PARSING_ATTRIBUTE, "XML_ERROR_PARSING_ATTRIBUTE: xml parsing attribute error!");
				return 0;
			}
			if(prevAttribute) {
				prevAttribute->_next=attrib;
			} else {
				_rootAttribute=attrib;
			}
			prevAttribute=attrib;
		} else if(*p=='/'&&*(p+1)=='>') {
			_closingType=CLOSED;
			return p+2;
		} else if(*p=='>') {
			++p;
			break;
		} else {
			_document->SetError(XML_ERROR_PARSING_ELEMENT, "XML_ERROR_PARSING_ELEMENT: xml parsing element error!");
			return 0;
		}
	}
	return p;
}

int32_t XMLDocument::Parse(const char* xml, int32_t nBytes)
{
	if(!xml||!*xml) {
		_document->SetError(XML_ERROR_EMPTY_STR, "XML_ERROR_EMPTY_STR: xml str is empty!");
		return _errorID;
	}

	nBytes=(nBytes==-1)?strlen(xml):nBytes;

	_charBuffer=q_new_array<char>(nBytes+1);
	Q_CHECK_PTR(_charBuffer);
	memcpy(_charBuffer, xml, nBytes);
	_charBuffer[nBytes]=0;

	const char* start=xml;
	xml=XMLUtil::SkipWhiteSpace(const_cast<char*>(xml));
	if(!xml||!*xml) {
		_document->SetError(XML_ERROR_EMPTY_STR, "XML_ERROR_EMPTY_STR: xml str is empty!");
		return _errorID;
	}

	ptrdiff_t delta=xml-start;
#if defined (XML_DEBUG)
	Q_INFO("Document Parsing:\t(%.*s...)", 1<<6, _charBuffer+delta);
#endif
	ParseDeep(_charBuffer+delta, NULL);
	return _errorID;
}

const char* XMLDocument::ToXML()
{
	QStringBuffer _xml;
	if(_document->Declaration()) {
		XMLDeclaration* dec=_document->Declaration();
		_xml.append("<?");
		_xml.append(dec->Value());
		_xml.append('>');
		_xml.append('\n');
	}
	XMLElement* elem=_document->RootElement();
	ToXML(elem, _xml);
	return _xml.getBuffer();
}

char* XMLDocument::Identify(char* p, XMLNode** node)
{
	XMLNode* returnNode=0;
	char* start=p;
	p=XMLUtil::SkipWhiteSpace(p);
	if(!p||!*p)
		return p;

	static const char* xmlHeader		= {"<?"};
	static const char* commentHeader	= {"<!--"};
	static const char* dtdHeader		= {"<!"};
	static const char* cdataHeader		= {"<![CDATA["};
	static const char* elementHeader	= {"<"};

	static const int32_t xmlHeaderLen	= 2;
	static const int32_t commentHeaderLen	= 4;
	static const int32_t dtdHeaderLen	= 2;
	static const int32_t cdataHeaderLen	= 9;
	static const int32_t elementHeaderLen	= 1;

	if(XMLUtil::StringEqual(p, xmlHeader, xmlHeaderLen)) {
		returnNode=new(std::nothrow) XMLDeclaration(this);
		Q_CHECK_PTR(returnNode);
		p+=xmlHeaderLen;
#if defined (XML_DEBUG)
		Q_INFO("XMLNode's _Type_:\t<%s>", "Declaration");
#endif
	} else if(XMLUtil::StringEqual(p, commentHeader, commentHeaderLen)) {
		returnNode=new(std::nothrow) XMLComment(this);
		Q_CHECK_PTR(returnNode);
		p+=commentHeaderLen;
#if defined (XML_DEBUG)
		Q_INFO("XMLNode's _Type_:\t<%s>", "Comment");
#endif
	} else if(XMLUtil::StringEqual(p, cdataHeader, cdataHeaderLen)) {
		XMLText* text=new(std::nothrow) XMLText(this);
		Q_CHECK_PTR(text);
		returnNode=text;
		p+=cdataHeaderLen;
		text->setCData(true);
#if defined (XML_DEBUG)
		Q_INFO("XMLNode's _Type_:\t<%s>", "CData");
#endif
	} else if(XMLUtil::StringEqual(p, dtdHeader, dtdHeaderLen)) {
		returnNode=new(std::nothrow) XMLUnknown(this);
		Q_CHECK_PTR(returnNode);
		p+=dtdHeaderLen;
#if defined (XML_DEBUG)
		Q_INFO("XMLNode's _Type_:\t<%s>", "DTD");
#endif
	} else if(XMLUtil::StringEqual(p, elementHeader, elementHeaderLen)) {
		returnNode=new(std::nothrow) XMLElement(this);
		Q_CHECK_PTR(returnNode);
		p+=elementHeaderLen;
#if defined (XML_DEBUG)
		Q_INFO("XMLNode's _Type_:\t<%s>", "Element");
#endif
	} else {
		returnNode=new(std::nothrow) XMLText(this);
		Q_CHECK_PTR(returnNode);
		p=start;
#if defined (XML_DEBUG)
		Q_INFO("XMLNode's _Type_:\t<%s>", "Text");
#endif
	}

	*node=returnNode;
	return p;
}

void XMLDocument::ToXML(XMLElement* element, QStringBuffer& _xml)
{
	if(element) {
		_xml.append('<');
		_xml.append(element->Name());
		if(element->FirstAttribute()) {
			for(XMLAttribute* attr=element->FirstAttribute(); attr; attr=attr->Next()) {
				_xml.append(' ');
				_xml.append(attr->Name());
				_xml.append('=');
				_xml.append('\"');
				_xml.append(attr->Value());
				_xml.append('\"');
			}
		}
		_xml.append('>');
		if(element->FirstChildElement()) {
			_xml.append('\n');
			for(XMLElement* child=element->FirstChildElement(); child; child=child->NextSiblingElement())
				ToXML(child, _xml);
		} else {
			if(element->FirstChild()&&element->FirstChild()->ToText()) {
				XMLText* text=element->FirstChild()->ToText();
				if(text->CData())
					_xml.append("<![CDATA[");
				_xml.append(text->Value());
				if(text->CData())
					_xml.append("]]>");
			}
		}
		_xml.append("</");
		_xml.append(element->Name());
		_xml.append('>');
		_xml.append('\n');
	}
}

Q_END_NAMESPACE
