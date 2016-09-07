#include "../include/common/qfile.h"
#include "../include/common/qtcpsocket.h"

Q_USING_NAMESPACE

int main(int argc, char** argv)
{
	QTcpClient client;
	networkReply reply;
	int ret=0;

	client.setHost("192.168.1.91", 8090);
	client.setTimeout(10000);

	client.setProtocolType(1);
	client.setSourceType(1);
	client.setCommandType(1);
	client.setOperateType(0);

	int64_t size=0;
	char* data=QFile::readAll(argv[1], &size);
	assert(data!=NULL);

	ret=client.sendRequest(data, size);
	if(ret<0) {
		printf("ret = (%d)\n", ret);
		return -1;
	} 

	ret=client.getReply(&reply);
	if(ret<0) {
		printf("ret = (%d)\n", ret);
		return -2;
	}

	printf("reply = (%.*s)\n", reply.length, reply.data);

	return 0;
}
