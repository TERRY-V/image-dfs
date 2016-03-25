#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cassert>

#include "qsaver.h"

class QSaverService : public QService {
	public:
		QSaverService()
		{}

		virtual ~QSaverService()
		{}

		virtual int32_t init()
		{
			int32_t ret=qsc.init("../conf/init.cnf");
			if(ret<0){
				Q_INFO("QSaver init failed, ret = (%d)", ret);
				return -1;
			}
			return 0;
		}

		virtual int32_t run(int32_t argc, char** argv)
		{
			Q_INFO("QSaver init success ,now to start...");
			qsc.start();
			Q_INFO("QSaver quit");
			return 0;
		}

		virtual int32_t destroy()
		{
			return 0;
		}
	
	private:
		QSaver qsc;
};

int32_t main(int32_t argc, char **argv)
{
	QSaverService qss;
	return qss.main(argc, argv);
}

