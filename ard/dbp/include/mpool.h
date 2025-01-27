#pragma once

/**
AllocPool - we would preallocate memory in one chunk of buffer
and reuse it's pointers for object. The object will
have overloaded new/delete operators

----------- 64 KB (current) -------------
64K = 65536
65536 - 16 = 65520
*/
#define MPOOL_ALLOC_CHUNK    65520

namespace ard 
{
    class AllocPool
    {
    public:
        void* mget(std::size_t size) 
        {
            if (m_allocated.empty()) {
                //qDebug() << "alloc" << size;
                reserveMPool(size);
            }

            void* m = m_allocated.back();
            m_allocated.pop_back();
            return m;
        };

        void reserveMPool(std::size_t obj_size) 
        {           
            std::size_t chunk_size = MPOOL_ALLOC_CHUNK;
            void* p_alloc = ::operator new(MPOOL_ALLOC_CHUNK);
            char* p = (char*)p_alloc;
            while (chunk_size > obj_size) {
                m_allocated.push_back(p);
                chunk_size -= obj_size;
                p += obj_size;
            }
            //qDebug() << "alloc-reserved" << obj_size << m_allocated.size();
        }

        void mfree(void* m) {
            m_allocated.push_back(m);
        };

    protected:
        std::vector<void*> m_allocated;
    };
};

#define DECLARE_IN_ALLOC_POOL(T)        \
public:                                 \
void* operator new(std::size_t size)    \
{                                       \
    if (size == sizeof(T)) {            \
        return m_alloc_pool.mget(size); \
    }                                   \
    return ::operator new(size);        \
};                                      \
void operator delete(void *p, std::size_t size)\
{                                       \
    if (size == sizeof(T)) {            \
        m_alloc_pool.mfree(p);          \
        return;                         \
    }                                   \
    ::operator delete(p);               \
};                                      \
private:                                \
static ard::AllocPool m_alloc_pool;     \


#define IMPLEMENT_ALLOC_POOL(T) ard::AllocPool ard::T::m_alloc_pool;

