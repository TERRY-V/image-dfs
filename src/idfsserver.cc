#include "idfsserver.h"

int32_t IDFSServer::initialize()
{
	int32_t ret=0;

	ret=config_->getFieldString("img-path", img_path_);
	if(ret<0)
		return TCP_ERR;

	ret=config_->getFieldString("img-dir", img_dir_);
	if(ret<0)
		return TCP_ERR;

	ret=config_->getFieldInt32("img-subdir-num", img_subdir_num_);
	if(ret<0)
		return TCP_ERR;

	ret=config_->getFieldString("mongo-uri", mongo_uri_);
	if(ret<0)
		return TCP_ERR;

	ret=config_->getFieldString("mongo-img-collection", mongo_img_collection_);
	if(ret<0)
		return TCP_ERR;

	/* directory */
	if(!QDir::mkdir(img_path_)) {
		logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
				"mkdir img_path_ (%s) error!", \
				img_path_);
		return TCP_ERR;
	}

	char directory[1<<10]={0};
	for(int32_t i=0; i<img_subdir_num_; i++)
	{
		if(snprintf(directory, sizeof(directory), "%s/%s/%03d", img_path_, img_dir_, i)<0)
			return TCP_ERR;

		if(!QDir::mkdir(directory))
			return TCP_ERR;
	}

	/* mongo */
	mongo_client_=new(std::nothrow) QMongoClient(mongo_uri_);
	if(!mongo_client_) {
		logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
				"mongo_client_ alloc uri (%s) error!", \
				mongo_uri_);
		return TCP_ERR;
	}

	mongo_client_->setCollection(mongo_img_collection_);

	return TCP_OK;
}

int32_t IDFSServer::server_init(void*& handle)
{
	return TCP_OK;
}

int32_t IDFSServer::server_header(const char* header_buffer, int32_t header_len, const void* handle)
{
	Q_CHECK_PTR(header_buffer);
	Q_ASSERT(header_len==sizeof(baseHeader), "header_len must be equal to sizeof(baseHeader)!");

	baseHeader* base_header=reinterpret_cast<baseHeader*>((char*)header_buffer);
	Q_CHECK_PTR(base_header);

	if(base_header->version!=TCP_HEADER_VERSION) {
		logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
				"TCP socket version error, version = (%.*s)!", \
				sizeof(uint64_t), \
				(char*)&base_header->version);
		return TCP_ERR_SOCKET_VERSION;
	}

	return base_header->length;
}

int32_t IDFSServer::server_process(const char* request_buffer, int32_t request_len, char* reply_buffer, int32_t reply_size, const void* handle)
{
	Q_CHECK_PTR(request_buffer);
	Q_CHECK_PTR(reply_buffer);
	Q_ASSERT(request_len>0, "request_len must be larger than 0!");
	Q_ASSERT(reply_size>0, "reply_size must be larger than 0!");

	const char* ptr_temp=request_buffer;
	const char* ptr_end=request_buffer+request_len;

	uint16_t operate_type=0;
	const char* ptr_data=NULL;
	int32_t data_len=0;

	int32_t reply_len=0;
	int32_t ret=0;

	requestParam* request_param=reinterpret_cast<requestParam*>((char*)request_buffer);
	ptr_temp+=sizeof(requestParam);

	if(request_param->protocol_type!=TCP_DEFAULT_PROTOCOL_TYPE) {
		logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
				"Protocol type error, protocol_type = (%d)!", \
				request_param->protocol_type);
		return TCP_ERR_PROTOCOL_TYPE;
	}

	if(request_param->source_type!=TCP_DEFAULT_COMMAND_TYPE) {
		logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
				"Source type error, source_type = (%d)!", \
				request_param->source_type);
		return TCP_ERR_SOURCE_TYPE;
	}

	if(request_param->command_type==TCP_DEFAULT_OPERATE_TYPE)
	{
		operate_type=*(uint16_t*)ptr_temp;
		ptr_temp+=sizeof(uint16_t);

		data_len=*(int32_t*)ptr_temp;
		if(data_len>0 && ptr_temp+sizeof(int32_t)+data_len==ptr_end) {
			ptr_data=ptr_temp+sizeof(int32_t);
		} else {
			logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
					"Data length error, data_len = (%d)!", \
					data_len);
			return TCP_ERR_DATA_LENGTH;
		}

		char* ptr_reply_temp=reply_buffer;
		char* ptr_reply_end=ptr_reply_temp+reply_size;

		replyParam* reply_param=reinterpret_cast<replyParam*>(ptr_reply_temp);
		reply_param->status=0;
		reply_param->command_type=request_param->command_type;

		ptr_reply_temp+=sizeof(replyParam)+sizeof(int32_t);

		ret=server_main(operate_type, ptr_data, data_len, ptr_reply_temp, (ptrdiff_t)(ptr_reply_end-ptr_reply_temp), handle);
		if(ret<0) {
			logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
					"process error, ret = (%d)!", \
					ret);
			return ret;
		} else {
			logger_->log(LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
					"process over, result = \n%.*s", \
					ret, \
					ptr_reply_temp);
			ptr_reply_temp+=ret;
		}

		*(int32_t*)(reply_buffer+sizeof(replyParam))=ret;
		reply_len=(ptrdiff_t)(ptr_reply_temp-reply_buffer);
	} else {
		logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
				"Command type error, command_type = (%d)!", \
				request_param->command_type);
		return TCP_ERR_COMMAND_TYPE;
	}

	return reply_len;
}

int32_t IDFSServer::server_main(uint16_t type, const char* ptr_data, int32_t data_len, char* ptr_out, int32_t out_size, const void* handle)
{
	Q_CHECK_PTR(ptr_data);
	Q_CHECK_PTR(ptr_out);
	Q_ASSERT(data_len>0, "data_len must be larger than 0!");
	Q_ASSERT(out_size>0, "out_size must be larger than 0!");

	char* ptr_temp=ptr_out;
	char* ptr_end=ptr_temp+out_size;
	int32_t ret=0;

	QMD5 qmd5;
	uint64_t iid=0;

	std::string imgid("");
	std::string local_path("");
	std::string file_path("");
	std::string img_size("");

	int32_t width=0;
	int32_t height=0;

	if(type>=0 && type<5)
	{
		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
		if(ptr_temp+ret>=ptr_end)
			return -51;
		ptr_temp+=ret;

		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "<doc>\n");
		if(ptr_temp+ret>=ptr_end)
			return -52;
		ptr_temp+=ret;

		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "<base>\n");
		if(ptr_temp+ret>=ptr_end)
			return -53;
		ptr_temp+=ret;

		iid=qmd5.MD5Bits64((unsigned char*)ptr_data, data_len);
		imgid=q_to_string(iid);

		mongo_mutex_.lock();
		if(mongo_client_->exists("imgid", imgid.c_str()))
		{
			if(mongo_client_->select("imgid", imgid.c_str(), "imgpath", local_path, "imgsize", img_size)==MONGO_ERR) {
				mongo_mutex_.unlock();
				return -54;
			}
		} else {
			file_path=q_format("%s/%03d/%lx.%s", img_dir_, static_cast<int32_t>(iid%1000), iid, get_image_type_name(type));
			local_path=img_path_+'/'+file_path;

			ret=save_image(local_path.c_str(), ptr_data, data_len);
			if(ret<0) {
				mongo_mutex_.unlock();
				return -55;
			}

			ret=getImageSize(local_path.c_str(), &width, &height);
			if(ret<0) {
				mongo_mutex_.unlock();
				return -56;
			}

			img_size=q_format("%d*%d", width, height);

			if(mongo_client_->insert("imgid", imgid.c_str(), "imgpath", local_path.c_str(), "imgsize", img_size.c_str())==MONGO_ERR) {
				mongo_mutex_.unlock();
				return -57;
			}
		}

		mongo_mutex_.unlock();

		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "<imgid><![CDATA[%lu]]></imgid>\n", iid);
		if(ptr_temp+ret>=ptr_end)
			return -58;
		ptr_temp+=ret;

		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "<imgpath><![CDATA[%s]]></imgpath>\n", file_path.c_str());
		if(ptr_temp+ret>=ptr_end)
			return -59;
		ptr_temp+=ret;

		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "<imgsize><![CDATA[%s]]></imgsize>\n", img_size.c_str());
		if(ptr_temp+ret>=ptr_end)
			return -60;
		ptr_temp+=ret;

		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "</base>\n");
		if(ptr_temp+ret>=ptr_end)
			return -61;
		ptr_temp+=ret;

		ret=snprintf(ptr_temp, ptr_end-ptr_temp, "</doc>");
		if(ptr_temp+ret>=ptr_end)
			return -62;
		ptr_temp+=ret;
		/************************ FINISH *****************************/
	} else {
		logger_->log(LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, log_screen_, \
				"Operate type error, operate_type = (%d)!", \
				type);
		return TCP_ERR_OPERATE_TYPE;
	}

	return (ptrdiff_t)(ptr_temp-ptr_out);
}

int32_t IDFSServer::server_free(const void* handle)
{
	return TCP_OK;
}

int32_t IDFSServer::release()
{
	q_free(img_path_);
	q_free(mongo_uri_);
	q_free(mongo_img_collection_);
	q_delete<QMongoClient>(mongo_client_);
	return TCP_OK;
}

int32_t IDFSServer::save_image(const char* path, const char* data, int32_t len)
{
	if(path==NULL||data==NULL||len<=0)
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
	} else {
		return 1;
	}

	return 0;
}

const char* IDFSServer::get_image_type_name(int32_t type)
{
	switch(type)
	{
	case 0:
		return "jpg";
	case 1:
		return "png";
	case 2:
		return "bmp";
	case 3:
		return "gif";
	case 4:
		return "tif";
	default:
		return "jpg";
	}
}

