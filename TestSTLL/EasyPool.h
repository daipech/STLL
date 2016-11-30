
#ifndef _EASYPOOL_H
#define _EASYPOOL_H
/*
 * basic memory pool
 */
class BasicPool
{
public:
    /*
     * constructor
     * size: max object size
     * count: max object count
     * align: object alignment
     */
    BasicPool( int size, int count, int align );
    virtual ~BasicPool();

    BYTE* Alloc();
    void Free( BYTE* ptr );

protected:
    BYTE* m_memory;
    BYTE* m_elements;
    int* m_marks;
    int m_free;
    int m_memsize;
    int m_elemsize;
    int m_count;
};

template <typename T, int N>
class EasyPool : public BasicPool
{
public:
    EasyPool( int count ) : BasicPool(sizeof(T),count,N)
    { }

    virtual ~EasyPool() {}

    T* Get() { return (T*)Alloc(); }
    void Put( T* ptr ) { Free((BYTE*)ptr);  }
};

typedef EasyPool<_Vec4,16> Vec4Pool;
typedef EasyPool<_Quat,16> QuatPool;
typedef EasyPool<_Mat4,16> Mat4Pool;

template <int SIZE>
class StackPool
{
public:
    StackPool()
    {
        alloc = &buf[0];
    }
    ~StackPool()
    {}

    template <typename T>
    T* Get(int align)
    {
        size_t tmp = (size_t)(alloc);
        size_t delta = tmp % align;
        if (delta != 0)
        {
            alloc += (align - delta);
        }

        T* rt = reinterpret_cast<T*>(alloc);
        alloc += sizeof(T);
        assert(alloc < &buf[SIZE]);
        return rt;
    }

    void Clear() { alloc = &buf[0]; }
protected:
    BYTE buf[SIZE];
    BYTE* alloc;
};

#endif

