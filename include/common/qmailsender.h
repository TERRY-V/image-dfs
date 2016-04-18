/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qmailsender.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2014/05/28
**
*********************************************************************************************/

#ifndef __QMAILSENDER_H_
#define __QMAILSENDER_H_

#include <curl/curl.h>
#include "qglobal.h"

Q_BEGIN_NAMESPACE

#define MULTI_PERFORM_HANG_TIMEOUT 60*1000

// C++邮件发送类，不支持QQ邮箱
class QMailSender {
	public:
		// 邮件发送类构造函数, SMTP默认端口号为25
		explicit QMailSender(const std::string& user="", const std::string& passwd="", const std::string& smtpServer="", const std::string& mailFrom="")
		{
			this->mUser=user;
			this->mPasswd=passwd;
			this->mSmtpServer=smtpServer;
			this->mRecipientList.clear();
			this->mMailFrom=mailFrom;
			this->mMailContent.clear();
			this->mMailContentPos=0;
		}

		// 邮件发送类析构函数
		virtual ~QMailSender()
		{}

		// 邮箱用户名
		void setUser(const std::string& user)
		{this->mUser=user;}

		std::string getUser() const
		{return this->mUser;}

		// 邮箱密码
		void setPasswd(const std::string& passwd)
		{this->mPasswd=passwd;}

		std::string getPasswd() const
		{return this->mPasswd;}

		// 设置邮箱SMTP服务器
		void setSmtpServer(const std::string& smtpServer)
		{this->mSmtpServer=smtpServer;}

		std::string getSmtpServer() const
		{return this->mSmtpServer;}
		
		// 设置邮箱发件人
		void setMailFrom(const std::string& mailFrom)
		{this->mMailFrom=mailFrom;}

		std::string getMailFrom() const
		{return mMailFrom;}
		
		// 添加收件人
		void addRecipient(const std::string& mailTo)
		{mRecipientList.push_back(mailTo);}

		void addRecipient(std::list<std::string> recipientList)
		{copy(recipientList.begin(), recipientList.end(), mRecipientList.begin());}

		// 邮件发送函数
		bool sendMail(const std::string& strSubject, const std::string& strMailBody) 
		{
			mMailContent.clear();
			mMailContentPos=0;

			ConstructHead(strSubject, strMailBody);

			CURL* curl=curl_easy_init();

			struct curl_slist* rcpt_list=NULL;
			for(std::list<std::string>::iterator it=mRecipientList.begin(); it!=mRecipientList.end(); it++)
				rcpt_list=curl_slist_append(rcpt_list, it->c_str());
			
			curl_easy_setopt(curl, CURLOPT_URL, mSmtpServer.c_str());
			curl_easy_setopt(curl, CURLOPT_USERNAME, mUser.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, mPasswd.c_str());

			curl_easy_setopt(curl, CURLOPT_MAIL_FROM, mMailFrom.c_str());
			curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpt_list);

			curl_easy_setopt(curl, CURLOPT_READFUNCTION, &read_callback);
			curl_easy_setopt(curl, CURLOPT_READDATA, this);
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

			CURLcode res=curl_easy_perform(curl);
			if(res!=CURLE_OK) {
				Q_INFO("QMailSender: curl_easy_perform failed, %s", curl_easy_strerror(res));
				return false;
			}
			
			curl_slist_free_all(rcpt_list);
			curl_easy_cleanup(curl);

			return true;
		}

	private:
		static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* userp)
		{
			QMailSender* sender=(QMailSender*)userp;
			if(size*nmemb<1)
				return 0;

			if((size_t)sender->mMailContentPos<sender->mMailContent.size()) {
				size_t len=sender->mMailContent[sender->mMailContentPos].length();
				memcpy(ptr, sender->mMailContent[sender->mMailContentPos].c_str(), sender->mMailContent[sender->mMailContentPos].length());
				sender->mMailContentPos++;
				return len;
			}
			return 0;
		}
		
		bool ConstructHead(const std::string& strSubject, const std::string& strContent)
		{
			mMailContent.push_back("MIME-Versioin: 1.0\r\n");

			std::string strTemp="To: ";
			for(std::list<std::string>::iterator it=mRecipientList.begin(); it!=mRecipientList.end(); ++it) {
				strTemp+=*it;
				if(it!=--mRecipientList.end()) strTemp+=",";
		    	}
		    	strTemp+="\r\n";
		    	mMailContent.push_back(strTemp);

			if(!mMailFrom.empty()) {
				strTemp="From: ";
				strTemp+=mMailFrom;
				strTemp+="\r\n";
				mMailContent.push_back(strTemp);
			}

			if(!strSubject.empty()) {
				strTemp="Subject: ";
				strTemp+=strSubject;
				strTemp+="\r\n";
				mMailContent.push_back(strTemp);
			}

			mMailContent.push_back("Content-Transfer-Encoding: 8bit\r\n");
			mMailContent.push_back("Content-Type: text/html; Charset=\"UTF-8\"\r\n\r\n");

			if(!strContent.empty()) {
				strTemp=strContent;
				strTemp+="\r\n\r\n";
				mMailContent.push_back(strTemp);
			}

			return true;
		}

	protected:
		// 邮箱用户名
		std::string mUser;
		// 邮箱密码
		std::string mPasswd;
		// 邮箱SMTP服务器
		std::string mSmtpServer;
		// 接收者邮件
		std::list<std::string> mRecipientList;
		// 发送者邮箱
		std::string mMailFrom;
		// 发送的内容队列，包括头和内容项
		std::vector<std::string> mMailContent;
		// 用于发送数据时记录发送到第几个content
		int32_t mMailContentPos;
};

Q_END_NAMESPACE

#endif // __QMAILSENDER_H_
