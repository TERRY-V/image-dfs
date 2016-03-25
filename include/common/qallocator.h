/********************************************************************************************
**
** Copyright (C) 2010-2014 Terry Niu (Beijing, China)
** Filename:	qallocator.h
** Author:	TERRY-V
** Email:	cnbj8607@163.com
** Support:	http://blog.sina.com.cn/terrynotes
** Date:	2014/05/23
**
*********************************************************************************************/

#ifndef __QALLOCATOR_H_
#define __QALLOCATOR_H_

#include "qglobal.h"
#include "qvector.h"

Q_BEGIN_NAMESPACE

// 内存分配器类
template <typename T_TYPE>
class QAllocator {
	public:
		// @函数名: 内存分配器类构造函数
		// @参数01: 最大内存块数
		// @参数02: 每个内存块的最大长度
		inline QAllocator(int64_t maxBlockNum=2000, int64_t maxBlockLen=1<<20) : 
			mMaxBlockNum(maxBlockNum), 
			mMaxBlockLen(maxBlockLen)
		{
			mNowBlockNum=-1;
			mNowBlockLen=mMaxBlockLen;
			pBlockLibrary=NULL;
			mTotalLen=0;
		}

		// @函数名: 析构函数
		virtual ~QAllocator()
		{
			if(pBlockLibrary!=NULL) {
				for(int64_t i=0; i<mMaxBlockNum; ++i)
					q_delete_array<T_TYPE>(pBlockLibrary[i]);
				q_delete_array<T_TYPE*>(pBlockLibrary);
			}
		}

		// @函数名: 内存分配函数
		// @参数01: 分配长度
		// @返回值: 成功返回分配内存的头指针，失败返回NULL值
		T_TYPE* alloc(int64_t allocLen=1)
		{
			// 创建内存块索引
			if(pBlockLibrary==NULL) {
				pBlockLibrary=q_new_array<T_TYPE*>(mMaxBlockNum);
				if(pBlockLibrary==NULL)
					return NULL;
				memset(pBlockLibrary, 0, mMaxBlockNum*sizeof(void*));
			}

			// 需要开辟新的内存块
			if(allocLen>mMaxBlockLen-mNowBlockLen) {
				// 需要开辟的内存太长
				if(allocLen>mMaxBlockLen)
					return NULL;
				// 分配的内存块达到最大值
				if(++mNowBlockNum>=mMaxBlockNum) {
					--mNowBlockNum;
					return NULL;
				}
				// 开辟新的块内存
				if(pBlockLibrary[mNowBlockNum]==NULL) {
					pBlockLibrary[mNowBlockNum]=q_new_array<T_TYPE>(mMaxBlockLen);
					if(pBlockLibrary[mNowBlockNum]==NULL) {
						mNowBlockNum--;
						return NULL;
					}
				}
				// 新内存块的长度为0
				mNowBlockLen=0;
			}

			// 原内存块剩余内存可继续使用
			T_TYPE* pTemp=&pBlockLibrary[mNowBlockNum][mNowBlockLen];
			mNowBlockLen+=allocLen;
			mTotalLen+=allocLen;

			return pTemp;
		}

		// @函数名: 获取当前已分配的长度
		int64_t getAllocLength() const
		{return mTotalLen;}

		// @函数名: 获取当前实际已使用buffer长度
		int64_t getBufferLength() const
		{return mMaxBlockLen*mNowBlockNum+mNowBlockLen;}

		// @函数名: 重置内存分配器, 可重复使用
		void resetAllocator()
		{
			mNowBlockNum=-1;
			mNowBlockLen=mMaxBlockLen;
			mTotalLen=0;
		}

		// @函数名: 释放内存分配器
		int64_t freeAllocator()
		{
			this->~QAllocator();
			mNowBlockNum=-1;
			mNowBlockLen=mMaxBlockLen;
			pBlockLibrary=NULL;
			mTotalLen=0;
			return 0;
		}

	protected:
		// 最大可开辟内存块数
		int64_t mMaxBlockNum;
		// 内存块最大长度
		int64_t mMaxBlockLen;
		// 当前块数
		int64_t mNowBlockNum;
		// 当前块已使用长度
		int64_t mNowBlockLen;
		// 已分配内存长度
		int64_t mTotalLen;
		// 内存块指针
		T_TYPE** pBlockLibrary;
};

// 内存分配器类, 分配内存后可循环利用
class QAllocatorRecycle {
	public:
		// @函数名: 内存分配器类构造函数
		inline QAllocatorRecycle() :
			mNowChunkNum(0),
			mMaxChunkNum(0),
			mNowBlockLen(0),
			mMaxBlockLen(0),
			mChunkSize(0),
			mNowBlockNum(0),
			mMaxBlockNum(0),
			mRecycleNum(0),
			mAllocNum(0),
			pBlockLibrary(0),
			pRecycleLibrary(0)
		{}

		// @函数名: 内存分配器类析构函数
		virtual ~QAllocatorRecycle()
		{
			if(pBlockLibrary!=NULL) {
				for(int64_t i=0; i<mMaxBlockNum; ++i)
					q_delete_array<char>(pBlockLibrary[i]);
				q_delete_array<char*>(pBlockLibrary);
			}
			pRecycleLibrary=NULL;
		}

		// @函数名: 初始化函数
		// @参数01: 每个chunk的长度
		// @参数02: 最大block数
		// @参数03: 每次分配block数
		// @返回值: 成功返回0, 失败返回<0的错误码
		int32_t init(int64_t lChunkSize=1*1024*1024, int64_t lMaxBlockNum=1000000, int64_t lPerAllocChunkNum=1000)
		{
			if(lChunkSize<=0||lMaxBlockNum<=0||lPerAllocChunkNum<=0)
				return -1;

			mChunkSize=lChunkSize+sizeof(void*);

			mMaxBlockNum=lMaxBlockNum;
			mNowBlockNum=-1;

			mNowChunkNum=lPerAllocChunkNum;
			mMaxChunkNum=lPerAllocChunkNum;

			mNowBlockLen=0;
			mMaxBlockLen=lPerAllocChunkNum*mChunkSize;

			pBlockLibrary=NULL;
			pRecycleLibrary=NULL;

			mRecycleNum=0;
			mAllocNum=0;
			
			return 0;
		}

		// @函数名: 内存分配函数
		// @返回值: 成功返回分配内存的首指针, 失败返回NULL
		char* alloc()
		{
			mMutex.lock();
			// 从回收区查看是否有内存可复用
			if(pRecycleLibrary!=NULL) {
				char* p=pRecycleLibrary+sizeof(void*);
				pRecycleLibrary=*(char**)pRecycleLibrary;
				mRecycleNum--;
				mMutex.unlock();
				return p;
			}
			// 创建内存块索引
			if(pBlockLibrary==NULL) {
				pBlockLibrary=q_new_array<char*>(mMaxBlockNum);
				if(pBlockLibrary==NULL) {
					mMutex.unlock();
					return NULL;
				}
				memset(pBlockLibrary, 0, mMaxBlockNum*sizeof(void*));
			}
			// 是否需要新分配内存
			if(mNowChunkNum==mMaxChunkNum) {
				// 当前block数已经达到峰值
				if(++mNowBlockNum>=mMaxBlockNum) {
					mNowBlockNum--;
					mMutex.unlock();
					return NULL;
				}
				// 开辟新内存buffer
				if(pBlockLibrary[mNowBlockNum]==NULL) {
					pBlockLibrary[mNowBlockNum]=q_new_array<char>(mMaxBlockLen);
					if(pBlockLibrary[mNowBlockNum]==NULL) {
						mNowBlockNum--;
						mMutex.unlock();
						return NULL;
					}
				}
				mNowBlockLen=0;
				mNowChunkNum=0;
			}

			// 当前内存足够了
			char* p=&pBlockLibrary[mNowBlockNum][mNowBlockLen]+sizeof(void*);
			mNowBlockLen+=mChunkSize;

			mNowChunkNum++;
			mAllocNum++;

			mMutex.unlock();
			return p;
		}

		// @函数名: 释放buffer所指向的内存块, 未真实释放
		void free(char* buffer)
		{
			mMutex.lock();
			buffer-=sizeof(void*);
			*(char**)buffer=pRecycleLibrary;
			pRecycleLibrary=buffer;
			mRecycleNum++;
			mMutex.unlock();
		}

		// @函数名: 重置内存分配器
		void resetAllocator()
		{
			mMutex.lock();
			pRecycleLibrary=NULL;
			mNowChunkNum=mMaxChunkNum;
			mNowBlockLen=0;
			mNowBlockNum=-1;
			mRecycleNum=0;
			mAllocNum=0;
			mMutex.unlock();
		}

		// @函数名: 释放内存分配器
		void freeAllocator()
		{
			mMutex.lock();
			if(pBlockLibrary!=NULL) {
				for(int64_t i=0; i<mMaxBlockNum; ++i)
					q_delete_array<char>(pBlockLibrary[i]);
				q_delete_array<char*>(pBlockLibrary);
			}
			pRecycleLibrary=NULL;
			mNowChunkNum=mMaxChunkNum;
			mNowBlockLen=0;
			mNowBlockNum=-1;
			mRecycleNum=0;
			mAllocNum=0;
			mMutex.unlock();
		}

		// @函数名: 内存分配器跟踪
		void trace(const char* name)
		{
			Q_INFO("QAllocatorRecyle(%s): mAllocNum=(%ld), mRecyleNum=(%ld), mMaxBlockNum=(%ld), mNowBlockNum=(%ld)", \
					name, mAllocNum, mRecycleNum, mMaxBlockNum, mNowBlockNum+1);
		}

		// @函数名: 获取chunk的大小
		int64_t getChunkLen() const
		{return mChunkSize;}

	private:
		// 当前block已使用chunk数
		int64_t mNowChunkNum;
		// 当前block最大chunk数
		int64_t mMaxChunkNum;

		// 当前block已使用长度
		int64_t mNowBlockLen;
		// 当前block的长度
		int64_t mMaxBlockLen;

		// 每个chunk大小
		int64_t mChunkSize;

		// 已使用block数目
		int64_t mNowBlockNum;
		// 最大block数目
		int64_t mMaxBlockNum;

		// 总回收chunk数目
		int64_t mRecycleNum;
		// 总使用chunk数目
		int64_t mAllocNum;

		// 分配内存区
		char** pBlockLibrary;
		// 回收内存指针
		char* pRecycleLibrary;

		// 线程安全互斥锁
		QMutexLock mMutex;
};

// 内存池(适合小块内存分配)
template<int32_t CHUNK_SIZE>
class QAllocPool {
	public:
		// @函数名: 内存池构造函数
		inline QAllocPool() : 
			pRoot(0), 
			mNowAllocs(0), 
			nAllocs(0), 
			mMaxAllocs(0)
		{}

		// @函数名: 内存池析构函数
		virtual ~QAllocPool()
		{
			for(int32_t i=0; i<mBlockPtrs.size(); ++i) {
				if(mBlockPtrs[i]!=NULL) {
					delete mBlockPtrs[i];
					mBlockPtrs[i]=NULL;
				}
			}
		}

		// @函数名: 获取每个chunk的大小
		virtual int32_t chunk_size() const
		{return CHUNK_SIZE;}

		// @函数名: 获取当前已分配chunk数
		int32_t now_allocs() const
		{return mNowAllocs;}

		// @函数名: 分配内存
		virtual char* alloc()
		{
			if(!pRoot) {
				Block* block=new(std::nothrow) Block();
				if(block==NULL)
					return NULL;
				mBlockPtrs.push_back(block);
				for(int32_t i=0; i<CHUNKS_PER_BLOCK-1; ++i)
					block->chunk[i].next=&block->chunk[i+1];
				block->chunk[CHUNKS_PER_BLOCK-1].next=0;
				pRoot=block->chunk;
			}
			char *pTemp=pRoot;
			pRoot=pRoot->next;

			++mNowAllocs;
			if(mNowAllocs>mMaxAllocs)
				mMaxAllocs=mNowAllocs;
			nAllocs++;
			return pTemp;
		}

		// @函数名: 释放相关内存
		virtual void free(char* ptr)
		{
			if(!ptr)
				return;

			--mNowAllocs;
			Chunk* chunk=(Chunk*)ptr;
#ifdef DEBUG
			memset(chunk, 0xfe, sizeof(Chunk));
#endif
			// 并未释放物理内存, 可循环利用
			chunk->next=pRoot;
			pRoot=chunk;
		}

		// @函数名: 内存池分配情况跟踪
		void trace(const char* name)
		{
			Q_INFO("QAllocPool(%s): watermark=(%d) [%dk], current=(%d), nalloc=(%d), blocks=(%d)", \
					name, mMaxAllocs, mMaxAllocs*CHUNK_SIZE/1024, mNowAllocs, \
					nAllocs, mBlockPtrs.size());
		}

		// 每个Block平均的chunk数目
		enum {CHUNKS_PER_BLOCK=(10*CHUNK_SIZE)/CHUNK_SIZE};

	protected:
		union Chunk {
			Chunk* next;
			char mem[CHUNK_SIZE];
		};
		struct Block {
			Chunk chunk[CHUNKS_PER_BLOCK];
		};

		QVector<Block*> mBlockPtrs;
		Chunk* pRoot;

		int32_t mNowAllocs;
		int32_t nAllocs;
		int32_t mMaxAllocs;
};

Q_END_NAMESPACE

#endif // __QALLOCATOR_H_
