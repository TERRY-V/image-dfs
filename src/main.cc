#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cassert>

#include "idfsserver.h"

class IDFSService : public QService {
	public:
		IDFSService()
		{}

		virtual ~IDFSService()
		{}

		virtual int32_t init()
		{
			int32_t ret=server.init("../conf/init.cnf");
			if(ret<0){
				Q_INFO("IDFSserver init failed, ret = (%d)", ret);
				return -1;
			}
			return 0;
		}

		virtual int32_t run(int32_t argc, char** argv)
		{
			Q_INFO("IDFSserver init success ,now to start...");
			server.start();
			Q_INFO("IDFSserver quit");
			return 0;
		}

		virtual int32_t destroy()
		{
			return 0;
		}
	
	private:
		IDFSServer server;
};

int32_t main(int32_t argc, char **argv)
{
	IDFSService idfs;
	return idfs.main(argc, argv);
}

