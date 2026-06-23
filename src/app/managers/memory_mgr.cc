/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-06-23 14:38:29
 * @LastEditTime: 2026-06-23 15:18:42
 * @FilePath: /kk_frame/src/app/managers/memory_mgr.cc
 * @Description: 内存池管理器
 * @BugList: 
 * 
 * Copyright (c) 2026 by Ricken, All Rights Reserved. 
 * 
**/

#include "memory_mgr.h"

#include <cstdlib>

#if MEMORY_MGR_THREAD_SAFE
#define MEMORY_MGR_LOCK() std::lock_guard<std::mutex> lock(mMutex)
#else
#define MEMORY_MGR_LOCK() ((void)0)
#endif

struct MemoryMgr::Chunk {
    size_t allocMem;
    Chunk* prev;
    Chunk* next;
    bool   isFree;
};

struct MemoryMgr::Memory {
    char* start;
    uint32_t id;
    size_t poolSize;
    size_t allocMem;
    size_t progMem;
    Chunk* freeList;
    Chunk* allocList;
    Memory* next;
};

struct MemoryMgr::Pool {
    PoolId id;
    uint32_t lastMemoryId;
    bool autoExtend;
    size_t poolSize;
    size_t maxPoolSize;
    size_t allocatedPoolSize;
    Memory* memoryList;
};

MemoryMgr::MemoryMgr() {
    mNextPoolId = INVALID_POOL_ID + 1;
}

MemoryMgr::~MemoryMgr() {
    destroy();
}

void MemoryMgr::reset() {
    clear();
}

void MemoryMgr::clear() {
    MEMORY_MGR_LOCK();

    for (auto it = mPools.begin(); it != mPools.end(); ++it) {
        Pool* pool = it->second;
        Memory* memory = pool->memoryList;
        while (memory) {
            initMemory(memory, memory->poolSize);
            memory = memory->next;
        }
    }
}

void MemoryMgr::destroy() {
    MEMORY_MGR_LOCK();

    for (auto it = mPools.begin(); it != mPools.end(); ++it) {
        destroyPoolLocked(it->second);
    }
    mPools.clear();
    mNextPoolId = INVALID_POOL_ID + 1;
}

MemoryMgr::PoolId MemoryMgr::createPool(size_t maxPoolSize, size_t poolSize) {
    MEMORY_MGR_LOCK();

    Pool* pool = createPoolLocked(maxPoolSize, poolSize);
    return pool ? pool->id : INVALID_POOL_ID;
}

bool MemoryMgr::destroyPool(PoolId poolId) {
    MEMORY_MGR_LOCK();

    auto it = mPools.find(poolId);
    if (it == mPools.end()) return false;

    destroyPoolLocked(it->second);
    mPools.erase(it);
    return true;
}

bool MemoryMgr::clearPool(PoolId poolId) {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    if (!pool) return false;

    Memory* memory = pool->memoryList;
    while (memory) {
        initMemory(memory, memory->poolSize);
        memory = memory->next;
    }

    return true;
}

bool MemoryMgr::hasPool(PoolId poolId) const {
    MEMORY_MGR_LOCK();
    return mPools.find(poolId) != mPools.end();
}

void* MemoryMgr::alloc(PoolId poolId, size_t size) {
    if (size == 0) return nullptr;
    if (size > static_cast<size_t>(-1) - chunkHeaderSize() - chunkEndSize()) return nullptr;

    const size_t totalNeededSize = alignSize(size + chunkHeaderSize() + chunkEndSize());

    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    if (!pool || totalNeededSize > pool->poolSize) return nullptr;

FIND_FREE_CHUNK:
    Memory* memory = pool->memoryList;
    while (memory) {
        if (memory->poolSize - memory->allocMem < totalNeededSize) {
            memory = memory->next;
            continue;
        }

        Chunk* freeChunk = memory->freeList;
        while (freeChunk) {
            if (freeChunk->allocMem >= totalNeededSize) {
                Chunk* allocChunk = freeChunk;

                if (freeChunk->allocMem - totalNeededSize > chunkHeaderSize() + chunkEndSize()) {
                    Chunk* newFreeChunk = reinterpret_cast<Chunk*>(
                        reinterpret_cast<char*>(allocChunk) + totalNeededSize);

                    *newFreeChunk = *allocChunk;
                    newFreeChunk->allocMem -= totalNeededSize;
                    *reinterpret_cast<Chunk**>(
                        reinterpret_cast<char*>(newFreeChunk) + newFreeChunk->allocMem - chunkEndSize()) = newFreeChunk;

                    if (!newFreeChunk->prev) {
                        memory->freeList = newFreeChunk;
                    } else {
                        newFreeChunk->prev->next = newFreeChunk;
                    }
                    if (newFreeChunk->next) newFreeChunk->next->prev = newFreeChunk;

                    allocChunk->isFree = false;
                    allocChunk->allocMem = totalNeededSize;
                    *reinterpret_cast<Chunk**>(
                        reinterpret_cast<char*>(allocChunk) + totalNeededSize - chunkEndSize()) = allocChunk;
                } else {
                    removeChunk(memory->freeList, allocChunk);
                    allocChunk->isFree = false;
                }

                insertChunk(memory->allocList, allocChunk);

                memory->allocMem += allocChunk->allocMem;
                memory->progMem += allocChunk->allocMem - chunkHeaderSize() - chunkEndSize();
                return reinterpret_cast<char*>(allocChunk) + chunkHeaderSize();
            }

            freeChunk = freeChunk->next;
        }

        memory = memory->next;
    }

    if (pool->autoExtend) {
        const size_t remainSize = pool->maxPoolSize - pool->allocatedPoolSize;
        if (remainSize < totalNeededSize) return nullptr;

        const size_t newPoolSize = remainSize >= pool->poolSize ? pool->poolSize : remainSize;
        Memory* newMemory = createMemory(pool, newPoolSize);
        if (!newMemory) return nullptr;

        newMemory->next = pool->memoryList;
        pool->memoryList = newMemory;
        pool->allocatedPoolSize += newPoolSize;
        goto FIND_FREE_CHUNK;
    }

    return nullptr;
}

bool MemoryMgr::free(PoolId poolId, void* ptr) {
    if (!ptr) return false;

    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    if (!pool) return false;

    Memory* memory = findMemoryLocked(pool, ptr);
    if (!memory) return false;

    Chunk* chunk = reinterpret_cast<Chunk*>(reinterpret_cast<char*>(ptr) - chunkHeaderSize());
    bool found = false;
    for (Chunk* item = memory->allocList; item; item = item->next) {
        if (item == chunk) {
            found = true;
            break;
        }
    }
    if (!found) return false;

    removeChunk(memory->allocList, chunk);
    insertChunk(memory->freeList, chunk);
    chunk->isFree = true;

    memory->allocMem -= chunk->allocMem;
    memory->progMem -= chunk->allocMem - chunkHeaderSize() - chunkEndSize();

    return mergeFreeChunk(memory, chunk);
}

bool MemoryMgr::contains(PoolId poolId, void* ptr) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    return pool ? containsLocked(pool, ptr) : false;
}

size_t MemoryMgr::maxMemory(PoolId poolId) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    return pool ? pool->maxPoolSize : 0;
}

size_t MemoryMgr::totalMemory(PoolId poolId) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    return pool ? pool->allocatedPoolSize : 0;
}

size_t MemoryMgr::usedMemory(PoolId poolId) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    return pool ? usedMemoryLocked(pool) : 0;
}

size_t MemoryMgr::progMemory(PoolId poolId) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    return pool ? progMemoryLocked(pool) : 0;
}

float MemoryMgr::usage(PoolId poolId) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    if (!pool || pool->allocatedPoolSize == 0) return 0.0f;
    return static_cast<float>(usedMemoryLocked(pool)) / pool->allocatedPoolSize;
}

float MemoryMgr::progUsage(PoolId poolId) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    if (!pool || pool->allocatedPoolSize == 0) return 0.0f;
    return static_cast<float>(progMemoryLocked(pool)) / pool->allocatedPoolSize;
}

size_t MemoryMgr::memoryCount(PoolId poolId) const {
    MEMORY_MGR_LOCK();

    Pool* pool = findPoolLocked(poolId);
    return pool ? memoryCountLocked(pool) : 0;
}

size_t MemoryMgr::poolCount() const {
    MEMORY_MGR_LOCK();
    return mPools.size();
}

size_t MemoryMgr::chunkHeaderSize() {
    return sizeof(Chunk);
}

size_t MemoryMgr::chunkEndSize() {
    return sizeof(Chunk*);
}

size_t MemoryMgr::alignSize(size_t size) {
    const size_t align = sizeof(long);
    return (size + align - 1) & ~(align - 1);
}

void MemoryMgr::initMemory(Memory* memory, size_t poolSize) {
    memory->poolSize = poolSize;
    memory->allocMem = 0;
    memory->progMem = 0;
    memory->freeList = reinterpret_cast<Chunk*>(memory->start);
    memory->freeList->allocMem = poolSize;
    memory->freeList->prev = nullptr;
    memory->freeList->next = nullptr;
    memory->freeList->isFree = true;
    memory->allocList = nullptr;
}

void MemoryMgr::insertChunk(Chunk*& head, Chunk* chunk) {
    chunk->prev = nullptr;
    chunk->next = head;
    if (head) head->prev = chunk;
    head = chunk;
}

void MemoryMgr::removeChunk(Chunk*& head, Chunk* chunk) {
    if (!chunk->prev) {
        head = chunk->next;
        if (chunk->next) chunk->next->prev = nullptr;
    } else {
        chunk->prev->next = chunk->next;
        if (chunk->next) chunk->next->prev = chunk->prev;
    }
}

MemoryMgr::Pool* MemoryMgr::createPoolLocked(size_t maxPoolSize, size_t poolSize) {
    if (poolSize == 0 || maxPoolSize == 0 || poolSize > maxPoolSize) return nullptr;
    if (poolSize <= chunkHeaderSize() + chunkEndSize()) return nullptr;

    Pool* pool = static_cast<Pool*>(::calloc(1, sizeof(Pool)));
    if (!pool) return nullptr;

    PoolId poolId = INVALID_POOL_ID;
    for (size_t i = 0; i < static_cast<size_t>(-1); i++) {
        if (mNextPoolId == INVALID_POOL_ID) mNextPoolId++;
        if (mPools.find(mNextPoolId) == mPools.end()) {
            poolId = mNextPoolId++;
            break;
        }
        mNextPoolId++;
    }
    if (poolId == INVALID_POOL_ID) {
        ::free(pool);
        return nullptr;
    }

    pool->id = poolId;
    pool->lastMemoryId = 0;
    pool->autoExtend = poolSize < maxPoolSize;
    pool->poolSize = poolSize;
    pool->maxPoolSize = maxPoolSize;
    pool->allocatedPoolSize = 0;
    pool->memoryList = nullptr;

    Memory* memory = createMemory(pool, pool->poolSize);
    if (!memory) {
        ::free(pool);
        return nullptr;
    }

    pool->memoryList = memory;
    pool->allocatedPoolSize = pool->poolSize;
    mPools.insert(std::make_pair(pool->id, pool));
    return pool;
}

void MemoryMgr::destroyPoolLocked(Pool* pool) {
    if (!pool) return;

    Memory* memory = pool->memoryList;
    while (memory) {
        Memory* next = memory->next;
        ::free(memory);
        memory = next;
    }

    ::free(pool);
}

MemoryMgr::Memory* MemoryMgr::createMemory(Pool* pool, size_t poolSize) {
    char* buffer = static_cast<char*>(::malloc(sizeof(Memory) + poolSize));
    if (!buffer) return nullptr;

    Memory* memory = reinterpret_cast<Memory*>(buffer);
    memory->start = buffer + sizeof(Memory);
    memory->id = pool->lastMemoryId++;
    memory->next = nullptr;
    initMemory(memory, poolSize);
    return memory;
}

MemoryMgr::Pool* MemoryMgr::findPoolLocked(PoolId poolId) const {
    auto it = mPools.find(poolId);
    return it == mPools.end() ? nullptr : it->second;
}

MemoryMgr::Memory* MemoryMgr::findMemoryLocked(Pool* pool, void* ptr) const {
    if (!pool || !ptr) return nullptr;

    char* addr = static_cast<char*>(ptr);
    Memory* memory = pool->memoryList;
    while (memory) {
        if (memory->start <= addr && memory->start + memory->poolSize > addr) {
            return memory;
        }
        memory = memory->next;
    }

    return nullptr;
}

bool MemoryMgr::containsLocked(Pool* pool, void* ptr) const {
    Memory* memory = findMemoryLocked(pool, ptr);
    if (!memory) return false;

    Chunk* chunk = reinterpret_cast<Chunk*>(static_cast<char*>(ptr) - chunkHeaderSize());
    for (Chunk* item = memory->allocList; item; item = item->next) {
        if (item == chunk) return true;
    }

    return false;
}

bool MemoryMgr::mergeFreeChunk(Memory* memory, Chunk* chunk) {
    Chunk* mergeHead = chunk;
    Chunk* current = chunk;

    while (current->isFree) {
        mergeHead = current;
        if (reinterpret_cast<char*>(current) - chunkEndSize() - chunkHeaderSize() <= memory->start) {
            break;
        }
        current = *reinterpret_cast<Chunk**>(
            reinterpret_cast<char*>(current) - chunkEndSize());
    }

    current = reinterpret_cast<Chunk*>(
        reinterpret_cast<char*>(mergeHead) + mergeHead->allocMem);
    while (reinterpret_cast<char*>(current) < memory->start + memory->poolSize && current->isFree) {
        removeChunk(memory->freeList, current);
        mergeHead->allocMem += current->allocMem;
        current = reinterpret_cast<Chunk*>(
            reinterpret_cast<char*>(current) + current->allocMem);
    }

    *reinterpret_cast<Chunk**>(
        reinterpret_cast<char*>(mergeHead) + mergeHead->allocMem - chunkEndSize()) = mergeHead;
    return true;
}

size_t MemoryMgr::usedMemoryLocked(Pool* pool) const {
    size_t total = 0;
    Memory* memory = pool->memoryList;
    while (memory) {
        total += memory->allocMem;
        memory = memory->next;
    }
    return total;
}

size_t MemoryMgr::progMemoryLocked(Pool* pool) const {
    size_t total = 0;
    Memory* memory = pool->memoryList;
    while (memory) {
        total += memory->progMem;
        memory = memory->next;
    }
    return total;
}

size_t MemoryMgr::memoryCountLocked(Pool* pool) const {
    size_t count = 0;
    Memory* memory = pool->memoryList;
    while (memory) {
        count++;
        memory = memory->next;
    }
    return count;
}

#undef MEMORY_MGR_LOCK
