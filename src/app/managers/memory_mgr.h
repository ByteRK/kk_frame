/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-23 14:38:29
 * @LastEditTime: 2026-06-23 14:38:34
 * @FilePath: /kk_frame/src/app/managers/memory_mgr.h
 * @Description: 内存池管理器
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#ifndef __MEMORY_MGR_H__
#define __MEMORY_MGR_H__

#include "template/singleton.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#define g_memory MemoryMgr::instance()

class MemoryMgr : public Singleton<MemoryMgr> {
    friend Singleton<MemoryMgr>;

public:
    typedef uint32_t PoolId;

    static const PoolId INVALID_POOL_ID = 0;
    static const size_t DEFAULT_POOL_SIZE = 1024 * 1024;
    static const size_t DEFAULT_MAX_POOL_SIZE = 16 * 1024 * 1024;

protected:
    MemoryMgr();

public:
    ~MemoryMgr();

    /* 清空所有内存池中已分配对象，但保留已经扩展出来的内存池 */
    void reset();
    void clear();

    /* 销毁所有内存池并重置 ID 计数 */
    void destroy();

    /* 创建/管理独立内存池 */
    PoolId createPool(size_t maxPoolSize = DEFAULT_MAX_POOL_SIZE,
        size_t poolSize = DEFAULT_POOL_SIZE);
    bool   destroyPool(PoolId poolId);
    bool   clearPool(PoolId poolId);
    bool   hasPool(PoolId poolId) const;

    /* 使用指定内存池分配/释放内存 */
    void* alloc(PoolId poolId, size_t size);
    bool  free(PoolId poolId, void* ptr);

    template <typename T>
    T* allocObject(PoolId poolId);

    template <typename T>
    bool freeObject(PoolId poolId, T* ptr);

    /* 指针是否属于当前正在使用的内存池分配 */
    bool contains(PoolId poolId, void* ptr) const;

    /* 指定内存池统计信息 */
    size_t maxMemory(PoolId poolId) const;
    size_t totalMemory(PoolId poolId) const;
    size_t usedMemory(PoolId poolId) const;
    size_t progMemory(PoolId poolId) const;
    float  usage(PoolId poolId) const;
    float  progUsage(PoolId poolId) const;
    size_t memoryCount(PoolId poolId) const;

    /* 已创建的独立内存池数量 */
    size_t poolCount() const;

private:
    struct Chunk;
    struct Memory;
    struct Pool;

private:
    static size_t chunkHeaderSize();
    static size_t chunkEndSize();
    static size_t alignSize(size_t size);
    static void   initMemory(Memory* memory, size_t poolSize);
    static void   insertChunk(Chunk*& head, Chunk* chunk);
    static void   removeChunk(Chunk*& head, Chunk* chunk);

    Pool*   createPoolLocked(size_t maxPoolSize, size_t poolSize);
    void    destroyPoolLocked(Pool* pool);
    Memory* createMemory(Pool* pool, size_t poolSize);
    Pool*   findPoolLocked(PoolId poolId) const;
    Memory* findMemoryLocked(Pool* pool, void* ptr) const;
    bool    containsLocked(Pool* pool, void* ptr) const;
    bool    mergeFreeChunk(Memory* memory, Chunk* chunk);
    size_t  usedMemoryLocked(Pool* pool) const;
    size_t  progMemoryLocked(Pool* pool) const;
    size_t  memoryCountLocked(Pool* pool) const;

private:
    mutable std::mutex mMutex;
    PoolId mNextPoolId;
    std::unordered_map<PoolId, Pool*> mPools;
};

template <typename T>
T* MemoryMgr::allocObject(PoolId poolId) {
    return static_cast<T*>(alloc(poolId, sizeof(T)));
}

template <typename T>
bool MemoryMgr::freeObject(PoolId poolId, T* ptr) {
    return free(poolId, static_cast<void*>(ptr));
}

#endif // __MEMORY_MGR_H__
