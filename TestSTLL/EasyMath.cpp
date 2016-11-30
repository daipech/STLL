
#include "EasyMath.h"

float g_SinTable[65536];
float g_CosTable[65536];

void Easy_Init_Angles()
{
    for ( int i = 0; i < 65536; i++ )
    {
        g_SinTable[i] = sinf(TORADIAN(i));
        g_CosTable[i] = cosf(TORADIAN(i));
    }
}

float Easy_Sin( Angle ang )
{
    return g_SinTable[AngAbs(ang)];
}

float Easy_Cos( Angle ang )
{
    return g_CosTable[AngAbs(ang)];
}

float Easy_Tan( Angle ang )
{
    Angle tmp = AngAbs(ang);
    return g_SinTable[tmp]/g_CosTable[tmp];
}

float Easy_Cot( Angle ang )
{
    Angle tmp = AngAbs(ang);
    return g_CosTable[tmp]/g_SinTable[tmp];
}

Angle Easy_ASin( float val )
{
    return RADIAN(asinf(val));
}

Angle Easy_ACos( float val )
{
    return RADIAN(acosf(val));
}

BasicPool::BasicPool(int size, int count, int align)
{
    m_memory = (BYTE*)_aligned_malloc(size*count,align);
    m_elements = m_memory;
    m_memsize = size*count;
    m_elemsize = size;
    m_count = count;
    m_marks = new int[count];
    for ( int i = 0; i < count-1; i++ )
    {
        m_marks[i] = i+1;
    }
    m_marks[count-1] = 0;
    m_free = 0;
}

BasicPool::~BasicPool()
{
    delete [] m_marks;
    _aligned_free((void*)m_memory);
}

BYTE* BasicPool::Alloc()
{
    if ( m_free < 0 )
        return NULL;
    int tmp = m_free;
    m_free = m_marks[tmp];
    m_marks[tmp] = -1;
    return m_elements + tmp*m_elemsize;
}

void BasicPool::Free(BYTE *ptr)
{
    int tmp = (ptr - m_memory) / m_elemsize;
    if ( tmp < 0 || tmp >= m_count )
        return;
    m_marks[tmp] = m_free;
    m_free = tmp;
}

void memcmp_ta(void * dst,const void * src , unsigned int size)
{
	memcpy_s(dst,size,src,size);
}


