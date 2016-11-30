
#include "stll.h"
#include "EasyMath.h"
#include "STLLRasterizer.h"
#include "async\async++.h"

int AsyncList::m_size = 20;

AsyncList::AsyncList(int size) : m_pos(0)
{
    m_buffer = (int*)_aligned_malloc(size*sizeof(int), 4);
}

AsyncList::~AsyncList()
{
    _aligned_free(m_buffer);
}

STLLLightListMap::STLLLightListMap(int width, int height, int size) :
m_width(width), m_height(height), m_alloc(0)
{
    int length = width*height;
    m_length = length;
    AsyncList::SetSize(size);
    m_tiles = new AsyncList[length];
    for (int i = 0; i < length; i++)
    {
        new(&m_tiles[i]) AsyncList(size);
    }

    m_indicsTex = new _STLLIndex[length];
    m_lightsTex = NULL;
}

STLLLightListMap::~STLLLightListMap()
{
    int length = m_width*m_height;
    //for (int i = 0; i < length; i++)
    //{
    //    m_tiles[i].~AsyncList();
    //}

    delete[] m_tiles;
    delete[] m_indicsTex;
    if (m_lightsTex)
    {
        delete[] m_lightsTex;
    }
}

void STLLLightListMap::BuildTexture(_STLLLight* lightList, int listSize)
{
    if (!m_lightsTex || m_count >= m_alloc)
    {
        if (m_lightsTex)
        {
            delete[] m_lightsTex;
        }
        m_lightsTex = new _STLLLight[m_count];
        m_alloc = m_count;
    }

    int pos = 0;
    for (int i = 0; i < m_length; i++)
    {
        int count = m_tiles[i].Position();
        m_indicsTex[i].offset = pos;
        m_indicsTex[i].count = count;
        for (int j = 0; j < count; j++)
        {
            int lightId = m_tiles[i].Get(j);
            assert(lightId < listSize);
            m_lightsTex[pos + j] = lightList[lightId];
        }
        pos += count;
    }
}

STLLRasterizer::STLLRasterizer(STLLLightListMap* map) : matPool(4)
{
    m_map = map;
    matProj = matPool.Get();
    matVP = matPool.Get();
    Easy_matrix_identity(*matProj);
    Easy_matrix_identity(*matVP);

    int width = map->GetWidth();
    int height = map->GetHeight();
    matVP->Q4[0].f[0] = (float)width*0.5f;
    matVP->Q4[1].f[1] = -(float)height*0.5f;
    matVP->Q4[3].f[0] = (float)width*0.5f;
    matVP->Q4[3].f[1] = (float)height*0.5f;
}


STLLRasterizer::~STLLRasterizer()
{
}


void STLLRasterizer::SetTarget(STLLLightListMap* map)
{
    m_map = map;
}

STLLLightListMap* STLLRasterizer::GetTarget()
{
    return m_map;
}

void STLLRasterizer::SetProjection(_Mat4* proj)
{
    memcpy(matProj, proj, sizeof(_Mat4));
}

_Mat4* STLLRasterizer::GetProjection()
{
    return matProj;
}


inline void STLLRasterizer::Rasterize(float x, float y, float radius, int index)
{
    float minY = max(y-radius-0.5f,0.0f);
    float maxY = min(y+radius+0.5f,m_map->GetWidth()-1.0f);
    float tmpR = radius*radius;

    int startY = (int)floorf(minY);
    int endY   = (int)ceilf(maxY);

    for (int i = startY; i < endY; i++)
    {
        float tmpY  = fabsf(i - y) - 1.0f;
        float delta = sqrtf(tmpR - tmpY*tmpY);
        int startX  = (int)floorf(x-delta-0.5f);
        int endX    = (int)ceilf(x+delta+0.5f);

        m_map->AddLine(i, startX, endX, index);
    }
}


inline void STLLRasterizer::DrawLight(_STLLLight* light, int index)
{
    float height = (float)m_map->GetHeight();
    StackPool<128> vecPool;

    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    _Vec4* tmpSrc = vecPool.Get<_Vec4>(16);
    _Vec4* tmpPrjed = vecPool.Get<_Vec4>(16);
    _Vec4* tmpDst = vecPool.Get<_Vec4>(16);

    tmpSrc->f[0] = light->pos[0];
    tmpSrc->f[1] = light->pos[1];
    tmpSrc->f[2] = light->pos[2];
    tmpSrc->f[3] = 1.0f;

    Easy_matrix_mult_vector4X4((__m128*)tmpPrjed, (__m128*)matProj, *((__m128*)tmpSrc));
    r = 1.0f / tmpPrjed->P.w;
    Easy_vector_scalar_mul((__m128*)tmpPrjed, *((__m128*)tmpPrjed), r);
    Easy_matrix_mult_vector4X4((__m128*)tmpDst, (__m128*)matVP, *((__m128*)tmpPrjed));

    x = tmpDst->Q.x;
    y = tmpDst->Q.y;
    r *= light->range;

    // z refuse
    if (tmpDst->Q.z + r < 0.0f || tmpDst->Q.z - r > 1.0f)
    {
        return;
    }

    Rasterize(x, y, (r*height), index);
}


STLLRender::STLLRender(int width, int height, int range)
{
    m_map = new STLLLightListMap(width, height, range);
    m_raster = new STLLRasterizer(m_map);
}

STLLRender::~STLLRender()
{
    delete m_map;
    delete m_raster;
}

void STLLRender::Clear()
{
    m_map->Clear();
}

void STLLRender::SetCamera(float fov, float aspectRatio, float _near, float _far)
{
    _Mat4 projMat;
    EastPerspective(fov, aspectRatio, _near, _far, projMat.f16);
    m_raster->SetProjection(&projMat);
}

void STLLRender::Render(_STLLLight* pList, int count)
{
    STLLRasterizer* raster = m_raster;
    async::parallel_for(async::irange(0, count), [raster,pList](int i){
        raster->DrawLight(&pList[i], i);
    });
    m_map->BuildTexture(pList, count);
}
