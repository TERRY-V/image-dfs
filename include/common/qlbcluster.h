/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qlbcluster.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2015/01/15
**
*********************************************************************************************/

#ifndef __QLBCLUSTER_H_
#define __QLBCLUSTER_H_

#include "qglobal.h"
#include "qconfigreader.h"

Q_BEGIN_NAMESPACE

#pragma pack(1)

// 集群服务器结点信息结构体
typedef struct __server_info
{
	char		ip[16];		// 集群服务器IP地址
	int32_t 	port;		// 集群服务器PORT端口号
	int32_t 	timeout;	// 集群服务器超时时间

	int32_t		load_port;	// 集群服务器负载信息收集端口号
	int32_t 	weight;		// 集群服务器的综合权重(0该服务器已挂掉)

	__server_info() :
		port(-1),		// 集群服务器的默认端口号为-1
		timeout(10000),		// 集群服务器的默认超时时间为10秒
		load_port(5345),	// 集群服务器负载信息收集端口号, 默认为5345
		weight(1)		// 集群服务器的初始权值为1
	{
		ip[0]='\0';
	}

	__server_info(const __server_info& __si)
	{
		strcpy(ip, __si.ip);
		port=__si.port;
		timeout=__si.timeout;
		load_port=__si.load_port;
		weight=__si.weight;
	}
} SERVER_INFO;

#pragma pack()

// 通信连接调度算法
enum QScheduling
{
	RoundRobin,			// 轮询调度算法
	WeightedRoundRobin,		// 加权轮询调度算法

	LeastConnection,		// 最小连接调度算法
	WeightedLeastConnection,	// 加权最小连接调度算法

	LocalityBasedLeastConnection,	// 基于局部性的最少链接调度算法

	DestinationHashing,		// 目标地址散列调度算法
	SourceHashing			// 源地址散列调度算法
};

// 动态反馈负载均衡集群(调度模块)
// 说明: 
// 本集群系统能自动监视集群中各服务器负载状态, 并且将外部网请求转发到内部网的实际服务器执行.
// 当客户向均衡器发送一个请求报文时, 均衡器对此请求报文的目标地址进行替换, 将目标地址替换为
// 内部网实际服务器负载最轻的机器的IP地址, 然后将此报文转发出去. 当内部网中的实际服务器处理
// 完请求, 会将请求回应发向均衡器, 均衡器再次将目标地址替换为发出请求的外部网地址, 最后将此
// 报文转发给客户.

class QLBCluster: public noncopyable {
	public:
#pragma pack(1)
		// 集群服务器CPU报文信息结构体
		typedef struct __server_cpu_info
		{
			uint64_t	magic_mark;			// CPU报文信息唯一标识符
			int32_t		length;				// 报文后续数据总长度

			float64_t	server_connection_weight;	// 服务器TCP连接指标权重
			float64_t	server_load_weight;		// 服务器负载均值指标权重
			float64_t	server_disk_weight;		// 服务器磁盘信息指标权重
			float64_t	server_memory_weight;		// 服务器内存信息指标权重
			float64_t	server_process_weight;		// 服务器进程信息指标权重
		} SERVER_CPU_INFO;

		// 动态反馈负载均衡集群负载参数指标系数
		typedef struct __cluster_cpu_ratio
		{
			float64_t	cluster_connection_ratio;	// 集群TCP连接指标权重系数
			float64_t	cluster_load_ratio;		// 集群负载均值指标权重系数
			float64_t	cluster_disk_ratio;		// 集群磁盘信息指标权重系数
			float64_t	cluster_memory_ratio;		// 集群内存信息指标权重系数
			float64_t	cluster_process_ratio;		// 集群进程信息指标权重系数
			float64_t	cluster_expect_ratio;		// 集群服务器期待利用率
		} CLUSTER_CPU_RATIO;
#pragma pack()

	public:
		// 动态反馈负载均衡集群(调度模块)构造函数
		QLBCluster() :
			I_PROC_CPU_MARK(*(uint64_t*)"@CPU^-^@"),
			server_info_(0),
			server_max_(0),
			server_now_(0),
			server_iter_(0),
			cpu_ratio_(0),
			success_flag_(0)
		{}

		// 动态反馈负载均衡集群(调度模块)析构函数
		virtual ~QLBCluster()
		{
			if(server_info_)
				q_delete_array<SERVER_INFO>(server_info_);

			if(cpu_ratio_)
				q_delete<CLUSTER_CPU_RATIO>(cpu_ratio_);
		}

		// @函数名: 动态反馈负载均衡集群初始化函数
		// @参数01: 集群服务器配置文件路径
		// @参数02: 集群最大支持服务器数目
		// @参数03: 动态反馈间隔时间(默认为毫秒)
		// @参数04: 新权值与当前权值的差值阀值
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(const char* cfg_file, int32_t server_max=1000, uint32_t cpu_interval=20*1000, int32_t weight_gap_threshold=10)
		{
			server_max_=server_max;
			cpu_interval_=cpu_interval;
			weight_gap_threshold_=weight_gap_threshold;

			success_flag_=0;

			server_info_=q_new_array<SERVER_INFO>(server_max_);
			if(server_info_==NULL) {
				Q_DEBUG("QLBCluster: alloc error, server_max_ = (%d)", server_max_);
				return -1;
			}

			cpu_ratio_=q_new<CLUSTER_CPU_RATIO>();
			if(cpu_ratio_==NULL) {
				Q_DEBUG("QLBCluster: alloc error, cpu_ratio_ is null!");
				return -2;
			}

			int32_t ret=read_cluster_config(cfg_file);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read cluster config error, ret = (%d)!", ret);
				return -3;
			}

			if(q_init_socket()) {
				Q_DEBUG("QLBCluster: socket init error!");
				return -4;
			}

			if(q_create_thread(thread_collect, this)) {
				Q_DEBUG("QLBCluster: create collect thread error!");
				return -5;
			}

			while(success_flag_==0)
				q_sleep(1);

			if(success_flag_==-1) {
				Q_DEBUG("QLBCluster: collect thread start error!");
				return -6;
			}

			return 0;
		}

		// @函数名: 集群均衡器最大支持服务器数目
		int32_t max_size() const
		{return server_max_;}

		// @函数名: 集群均衡器当前服务器数目
		int32_t size() const
		{return server_now_;}

		// @函数名: 新增一台服务器
		int32_t push_back(const SERVER_INFO& item)
		{
			if(server_now_==server_max_)
				return -1;
			server_info_[server_now_++]=item;
			return 0;
		}

		// @函数名: 移除指定位置服务器
		void erase(int32_t pos)
		{
			Q_ASSERT(pos>=0&&pos<=server_max_-1, "QVector: out of range, pos = (%d)", pos);
			for(int32_t j=pos; j<server_max_-1; j++)
				server_info_[j]=server_info_[j+1];
			server_max_--;
		}

		// @函数名: 获取集群服务器列表
		SERVER_INFO* get_cluster() const
		{return server_info_;}

		// @函数名: 获取集群中指定服务器
		SERVER_INFO& get_server(int32_t pos)
		{
			Q_ASSERT(pos>=0&&pos<=server_max_-1, "QVector: out of range, pos = (%d)", pos);
			return server_info_[pos];
		}

		// @函数名: 通信连接调度算法
		// @参数01: 调度算法
		// @参数02: 散列用key(通常可使用MD5算法)
		// @返回值: 成功返回调度服务器的编号, 失败返回<0的错误码
		int32_t get_server(QScheduling sched=WeightedRoundRobin, uint64_t hash_key=0)
		{
			int32_t server_index=-1;

			switch(sched)
			{
				case RoundRobin:
					server_index=get_server_RR();
					break;
				case WeightedRoundRobin:
					server_index=get_server_WRR();
					break;
#if 0
				case LeastConnections:
					server_index=get_server_LC();
					break;
				case WeightedLeastConnections:
					server_index=get_server_WLC();
					break;
				case LocalityBasedLeastConnection:
					server_index=get_server_LBLC();
					break;
#endif
				case DestinationHashing:
					server_index=get_server_DH(hash_key);
					break;
				case SourceHashing:
					server_index=get_server_SH(hash_key);
					break;
				default:
					break;
			}

			return server_index;
		}

	private:
		// 读取集群配置文件相关信息
		int32_t read_cluster_config(const char* cfg_file)
		{
			QConfigReader cfg;

			char tag_name[1<<10];
			int32_t server_num=0;
			int32_t ret=0;

			ret=cfg.init(cfg_file);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read error, cfg_file = (%s)!", cfg_file);
				return -1;
			}

			// 读取集群服务器各指标权重系数
			ret=cfg.getFieldDouble("CLUSTER_CONNECTION_RATIO", cpu_ratio_->cluster_connection_ratio);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read cluster connection ratio error!");
				return -2;
			}

			ret=cfg.getFieldDouble("CLUSTER_LOAD_RATIO", cpu_ratio_->cluster_load_ratio);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read cluster load ratio error!");
				return -3;
			}

			ret=cfg.getFieldDouble("CLUSTER_DISK_RATIO", cpu_ratio_->cluster_disk_ratio);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read cluster disk ratio error!");
				return -4;
			}

			ret=cfg.getFieldDouble("CLUSTER_MEMORY_RATIO", cpu_ratio_->cluster_memory_ratio);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read cluster memory ratio error!");
				return -5;
			}

			ret=cfg.getFieldDouble("CLUSTER_PROCESS_RATIO", cpu_ratio_->cluster_process_ratio);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read cluster process ratio error!");
				return -6;
			}

			ret=cfg.getFieldDouble("CLUSTER_EXPECT_RATIO", cpu_ratio_->cluster_expect_ratio);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read cluster expect ratio error!");
				return -7;
			}

			// 读取集群服务器相关信息
			ret=cfg.getFieldInt32("CLUSTER_SERVER_NUM", server_num);
			if(ret<0) {
				Q_DEBUG("QLBCluster: read server num error, ret = (%d)", ret);
				return -8;
			}

			if(server_num>server_max_) {
				Q_DEBUG("QLBCluster: server num (%d) must be smaller than server_max_ (%d)", server_num, server_max_);
				return -9;
			}

			for(int32_t i=0; i!=server_num; ++i)
			{
				q_snprintf(tag_name, sizeof(tag_name)-1, "CLUSTER_IP_%03d", i+1);
				ret=cfg.getFieldString(tag_name, server_info_[i].ip, sizeof(server_info_[i].ip));
				if(ret<0) {
					Q_DEBUG("QLBCluster: read server (%d) ip error!", i);
					return -10;
				}

				q_snprintf(tag_name, sizeof(tag_name)-1, "CLUSTER_PORT_%03d", i+1);
				ret=cfg.getFieldInt32(tag_name, server_info_[i].port);
				if(ret<0) {
					Q_DEBUG("QLBCluster: read server (%d) port error!", ret);
					return -11;
				}

				q_snprintf(tag_name, sizeof(tag_name)-1, "CLUSTER_LOAD_PORT_%03d", i+1);
				ret=cfg.getFieldInt32(tag_name, server_info_[i].load_port);
				if(ret<0) {
					Q_DEBUG("QLBCluster: read server (%d) load port error!", ret);
					return -12;
				}

				q_snprintf(tag_name, sizeof(tag_name)-1, "CLUSTER_TIMEOUT_%03d", i+1);
				ret=cfg.getFieldInt32(tag_name, server_info_[i].timeout);
				if(ret<0) {
					Q_DEBUG("QLBCluster: read server (%d) timeout failed, ret = (%d)", ret);
					return -13;
				}

				++server_now_;
			}

			return 0;
		}

		// 该线程会定时从实际服务器收集CPU信息
		static Q_THREAD_T thread_collect(void* ptr_info)
		{
			QLBCluster* cc=reinterpret_cast<QLBCluster*>(ptr_info);
			Q_CHECK_PTR(cc);

			Q_SOCKET_T sock_client;

			SERVER_CPU_INFO cpu_info;
			int32_t server_weight=0;

			int32_t ret=0;

			cc->success_flag_=1;

			Q_FOREVER {
				for(int32_t i=0; i<cc->server_now_; ++i) {
					try {
						// 连接SOCKET服务端
						ret=q_connect_socket(sock_client, cc->server_info_[i].ip, cc->server_info_[i].load_port);
						if(ret!=0) {
							Q_DEBUG("QLBCluster: TCP socket [%s:%d] connection......", cc->server_info_[i].ip, cc->server_info_[i].load_port);
							throw -1;
						}

						// 设置超时时间
						ret=q_set_overtime(sock_client, cc->server_info_[i].timeout);
						if(ret!=0) {
							Q_DEBUG("QLBCluster: TCP socket [%s:%d] set overtime......", cc->server_info_[i].ip, cc->server_info_[i].load_port);
							throw -2;
						}

						// 发送收集CPU信息的请求报文
						ret=q_sendbuf(sock_client, (char*)(&cc->I_PROC_CPU_MARK), sizeof(uint64_t));
						if(ret!=0) {
							Q_DEBUG("QLBCluster: TCP socket [%s:%d] send......", cc->server_info_[i].ip, cc->server_info_[i].load_port);
							throw -3;
						}

						// 接收收集CPU信息的请求报文
						ret=q_recvbuf(sock_client, (char*)(&cpu_info), sizeof(SERVER_CPU_INFO));
						if(ret!=0) {
							Q_DEBUG("QLBCluster: TCP socket [%s:%d] recv......", cc->server_info_[i].ip, cc->server_info_[i].load_port);
							throw -4;
						}

						// 检查魔数标识符
						if(cpu_info.magic_mark!=cc->I_PROC_CPU_MARK) {
							Q_DEBUG("QLBCluster: TCP socket [%s:%d] magic_mark = (%lu)", cc->server_info_[i].ip, cc->server_info_[i].load_port, \
									cpu_info.magic_mark);
							throw -5;
						}

						// 计算综合权重
						if(cc->calculate_server_weight(cc->cpu_ratio_, &cpu_info, &server_weight)) {
							Q_DEBUG("QLBCluster: calculate server weight error!");
							throw -6;
						}

						// 为减少开销, 仅当权值差值不少于阀值时, 才重新设置服务器的综合权值
						if(::abs(cc->server_info_[i].weight-server_weight)>=cc->weight_gap_threshold_)
							cc->server_info_[i].weight=server_weight;

						// 关闭本次连接
						q_close_socket(sock_client);

						Q_DEBUG("QLBCluster: [%s:%05d] connection weight(%lf), load weight(%lf), disk weight(%lf), memory weight(%lf), process weight(%lf)", \
								cc->server_info_[i].ip, cc->server_info_[i].load_port, \
								cpu_info.server_connection_weight, \
								cpu_info.server_load_weight, \
								cpu_info.server_disk_weight, \
								cpu_info.server_memory_weight, \
								cpu_info.server_process_weight);

						Q_DEBUG("QLBCluster: current weight(%d), server weight(%d)", server_weight, cc->server_info_[i].weight);
					} catch(const int32_t err) {
						// 关闭本次连接
						q_close_socket(sock_client);

						// 此时, 该服务器可能挂掉了......
						// 设置该服务器的权值为0, 该服务器将不再接受新的请求, 等待重新加入.
						cc->server_info_[i].weight=0;

						Q_DEBUG("QLBCluster: [%s:%05d] may encounter an error!", cc->server_info_[i].ip, cc->server_info_[i].load_port);
					}
				}

				// 每隔指定时间查询各台服务器的CPU负载, 进行权值计算和调整
				q_sleep(cc->cpu_interval_);
			}

			cc->success_flag_=-1;
			return NULL;
		}

		// 计算实际服务器的综合负载
		// 实际服务器的负载越大, 权值越小, 实际服务器的负载越小, 权值越大
		int32_t calculate_server_weight(CLUSTER_CPU_RATIO* ccr, SERVER_CPU_INFO* sci, int32_t* server_weight)
		{
			if(ccr==NULL||sci==NULL||server_weight==NULL)
				return -1;

			float64_t aggregate_load=0;
			float64_t weight=0;

			aggregate_load+=ccr->cluster_connection_ratio*sci->server_connection_weight;
			aggregate_load+=ccr->cluster_load_ratio*sci->server_load_weight;
			aggregate_load+=ccr->cluster_disk_ratio*sci->server_disk_weight;
			aggregate_load+=ccr->cluster_memory_ratio*sci->server_memory_weight;
			aggregate_load+=ccr->cluster_process_ratio*sci->server_process_weight;

			// 负反馈公式
			if(aggregate_load==ccr->cluster_expect_ratio)
				weight=5;
			else if(aggregate_load<ccr->cluster_expect_ratio)
				weight=5+5*pow(ccr->cluster_expect_ratio-aggregate_load, 1.0/3.0);
			else
				weight=5-5*pow(aggregate_load-ccr->cluster_expect_ratio, 1.0/3.0);

			*server_weight=static_cast<int32_t>(weight*10);

			return 0;
		}

		// 轮询调度算法(容易导致服务器间负载不平衡)
		// 当服务器权值为0时, 表示该服务器不可用而不被调度(如屏蔽服务器故障和系统维护)
		int32_t get_server_RR()
		{
			mutex_lock_.lock();

			int32_t j=server_iter_;
			do {
				j=(j+1)%server_max_;
				if(server_info_[j].weight>0) {
					server_iter_=j;
					mutex_lock_.unlock();
					return j;
				}
			} while(j!=server_iter_);

			mutex_lock_.unlock();
			return -1;
		}

		// 加权轮询调度算法
		int32_t get_server_WRR()
		{
			mutex_lock_.lock();

			int32_t j=server_iter_;
			while(true) {
				j=(j+1)%server_now_;
				if(j==0) {
					current_weight_-=gcd();
					if(current_weight_<=0) {
						current_weight_=max_weight();
						if(current_weight_==0) {
							mutex_lock_.unlock();
							return -1;
						}
					}
				}

				if(server_info_[j].weight>=current_weight_) {
					server_iter_=j;
					mutex_lock_.unlock();
					return j;
				}
			}

			mutex_lock_.unlock();
			return -1;
		}

		// 目标地址散列调度算法
		int32_t get_server_DH(uint64_t hash_key)
		{
			int32_t i=hash_key%server_now_;
			int32_t j=i;

			do {
				if(server_info_[j].weight>0)
					return j;
				else
					++j;
			} while(j!=i);

			return -1;
		}

		// 源地址散列调度算法
		int32_t get_server_SH(uint64_t hash_key)
		{
			int32_t i=hash_key%server_now_;
			int32_t j=i;

			do {
				if(server_info_[j].weight>0)
					return j;
				else
					++j;
			} while(j!=i);

			return -1;
		}

	private:
		// 获取实际服务器中最大的权值
		int32_t max_weight()
		{
			int32_t max_weight=0;
			for(int32_t i=0; i<server_now_; ++i) {
				if(max_weight<server_info_[i].weight)
					max_weight=server_info_[i].weight;
			}
			return max_weight;
		}

		// 获取实际服务器权值的最大公约数
		int32_t gcd()
		{
			if(server_now_<=0)
				return -1;

			int32_t min=server_info_[0].weight;
			for(int32_t i=0; i<server_now_; ++i) {
				if(server_info_[i].weight<min)
					min=server_info_[i].weight;
			}

			while(min>=1)
			{
				bool is_common=true;
				for(int32_t i=0; i<server_now_; ++i) {
					if(server_info_[i].weight%min!=0) {
						is_common=false;
						break;
					}
				}
				if(is_common)
					break;
				min--;
			}

			return min;
		}

#if 0
		// 素数乘法Hash函数
		uint32_t hash_key(uint32_t dest_ip)
		{
			// 2654435761UL是2到2^32 (4294967296)间接近于黄金分割的素数.
			// (sqrt(5) - 1) / 2 =  0.618033989
			// 2654435761 / 4294967296 = 0.618033987
			return (dest_ip* 2654435761UL) & HASH_TAB_MASK;
		}
#endif

	protected:
		uint64_t		I_PROC_CPU_MARK;	// CPU请求信息标识头信息

		SERVER_INFO*		server_info_;		// 集群服务器列表信息
		int32_t			server_max_;		// 集群最大支持服务器结点数目
		int32_t			server_now_;		// 集群当前实际服务器结点数目

		int32_t			cpu_interval_;		// 集群服务器CPU查询间隔时间(5-20s为宜)
		int32_t			weight_gap_threshold_;	// 新权值与当前权值的差值阀值

		int32_t			server_iter_;		// 服务器指示变量
		int32_t			current_weight_;	// 当前调度的权值
		QMutexLock		mutex_lock_;		// 调度算法用互斥锁

		CLUSTER_CPU_RATIO*	cpu_ratio_;		// 集群服务器负载参数系数

		int32_t			success_flag_;		// 动态反馈收集线程成功启动标识
};

Q_END_NAMESPACE

#endif // __QLBCLUSTER_H_
