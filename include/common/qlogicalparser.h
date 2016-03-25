/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qlogicalparser.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2013/12/05
**
*********************************************************************************************/

#ifndef __QLOGICALPARSER_H_
#define __QLOGICALPARSER_H_

#include "qglobal.h"

Q_BEGIN_NAMESPACE

// QLogicalParser逻辑解析器
class QLogicalParser: public noncopyable {
	public:
		inline QLogicalParser()
		{}

		virtual ~QLogicalParser()
		{}

		// @函数名: 逻辑解析函数
		// @参数01: 待匹配语法
		// @参数02: 待匹配键
		// @参数03: 带匹配语义项
		// @返回值: 匹配成功返回true，失败返回false
		bool parse(const std::string& inGrammar, const std::string& inKey, const std::list<std::string>& inMeanings)
		{
			std::vector<std::string> v;
			InToPost(inGrammar, v);
			int32_t tag=CalRun(inKey, inMeanings, v);
			return (tag==1)?true:false;
		}

	private:
		// @函数名: 中缀转后缀表达式
		// @参数01: 待匹配语法
		// @参数02: 待匹配语义项
		// @返回值: 返回void
		void InToPost(const std::string& inGrammar, std::vector<std::string>& v)
		{
			std::string t;
			std::stack<char> stk;
			for(size_t pos(0); pos!=inGrammar.length(); ++pos) {
				char C=inGrammar.at(pos);
				switch(C) {
					case '&':
					case '|':
					case '!':
						if(!stk.empty()) {
							while(!stk.empty()&&Priority(C)<=Priority(stk.top())) {
								v.push_back(std::string("")+stk.top());
								stk.pop();
							}
						}
						stk.push(C);
						break;
					default:
						if(pos==inGrammar.length()-1||(pos<inGrammar.length()-1&&IsOperator(inGrammar.at(pos+1)))) {
							v.push_back(t+C);
							t.clear();
						} else {
							t+=C;
						}
						break;
				}
			}
			while(!stk.empty()) {
				v.push_back(std::string("")+stk.top());
				stk.pop();
			}
		}

		// @函数名: 优先级比较函数
		// @参数01: 优先级字符
		// @返回值: 成功返回优先级, 失败返回<0的错误码
		int32_t Priority(const char op)
		{
			switch(op) {
				case '|':
					return 1;
					break;
				case '&':
					return 2;
					break;
				case '!':
					return 3;
					break;
				default:
					return -1;
					break;
			}
		}

		// @函数名: 判断是否为操作符
		bool IsOperator(const char op)
		{return (op=='&'||op=='|'||op=='!')?true:false;}

		// @函数名: 逻辑解析匹配函数
		// @参数01: 待匹配词
		// @参数02: 待匹配语义项
		// @返回值: 匹配成功返回1, 失败返回0
		int32_t CalRun(const std::string& inKey, const std::list<std::string>& inMeanings, const std::vector<std::string>& v)
		{
			int32_t result(0);
			for(size_t i=0; i!=v.size(); ++i) {
				int32_t tag(0);
				if(v[i]=="&"||v[i]=="|"||v[i]=="!") {
					result=DoOperator(inKey, inMeanings, v[i]);
				} else {
					if(v[i]=="UK"||v[i]==inKey||find(inMeanings.begin(), inMeanings.end(), v[i])!=inMeanings.end())
						tag=1;
					s.push(tag);
				}
			}
			while(!s.empty()) {
				result=s.top();
				s.pop();
			}
			return result;
		}

		// @函数名: 操作符运算
		// @参数01: 待匹配词
		// @参数02: 待匹配语义项
		// @参数03: 操作符
		// @返回值: 判断为真返回1, 判断为假返回0
		int32_t DoOperator(const std::string& inKey, const std::list<std::string>& inMeanings, const std::string& op)
		{
			int32_t left(0), right(0), v(0);
			bool res(false);
			res=(op=="!")?Get1Operand(right):Get2Operands(left, right);
			if(res) {
				if(op=="!") {
					v=(right==1)?0:1;
				} else if(op=="&") {
					v=(left==1&&right==1)?1:0;
				} else if(op=="|") {
					v=(left==1||right==1)?1:0;
				} else {
					v=0;
				}
				s.push(v);
			} else {
				Clear();
			}
			return v;
		}

		// @函数名: 获取单操作符
		bool Get1Operand(int32_t& right)
		{
			if(s.empty()) return false;
			right=s.top();
			s.pop();
			return true;
		}

		// @函数名: 获取双操作符
		bool Get2Operands(int32_t& left, int32_t& right)
		{
			if(s.empty()) return false;
			right=s.top();
			s.pop();
			if(s.empty()) return false;
			left=s.top();
			s.pop();
			return true;
		}

		// @函数名: 清空栈
		void Clear()
		{while(!s.empty()) s.pop();}

	protected:
		// 逻辑表达式成员栈
		std::stack<int32_t> s;
};

Q_END_NAMESPACE

#endif // __QLOGICPARSER_H_
