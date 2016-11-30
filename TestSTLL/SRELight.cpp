
#include "EasyMath.h"
#include "SRELight.h"


RenderLights::RenderLights(int width, int height)
: vecPool(16), matPool(16), m_Raster(0,0,width,height)
{
    matProj = matPool.Get();
    matVP = matPool.Get();
    Easy_matrix_identity(*matProj);
    Easy_matrix_identity(*matVP);
    matVP->Q4[0].f[0] = (float)width*0.5f;
    matVP->Q4[1].f[1] = -(float)height*0.5f;
    matVP->Q4[3].f[0] = (float)width*0.5f;
    matVP->Q4[3].f[1] = (float)height*0.5f;
    tmpLt = 0;
    tmpIdx = 0;
    m_nWidth = width;
    m_nHeight = height;
    m_pIdxList = new ListIndexUnit[width*height];
    m_pLightList = NULL;
    //m_pLinkList = NULL;
    m_LinkList.ChangeMaxCount(width*height);
    m_nLtListNum = 0;
    m_nLkListNum = 0;
    m_nLightDataNum = 0;
    m_Raster.SetCallback(this);
    maxLt = 100;
}

RenderLights::~RenderLights()
{
    delete [] m_pIdxList;
}

void RenderLights::OnRaster(int x, int y)
{
    int idx = y*m_nWidth+x;
    //m_pLinkList[tmpIdx].light = tmpLt;
    //m_pLinkList[tmpIdx].next = m_pIdxList[idx].offset;
    if ( m_pIdxList[idx].count >= maxLt )
        return;
    static LinkListUnit tmpUnit;
    tmpUnit.light = tmpLt;
    tmpUnit.next = m_pIdxList[idx].offset;
    m_LinkList.Add(tmpUnit);
    m_pIdxList[idx].offset = tmpIdx;
    m_pIdxList[idx].count++;
    tmpIdx++;
}

void RenderLights::SetMatrix(_Mat4 *proj)
{
    memcpy(matProj,proj,sizeof(_Mat4));
}

void RenderLights::Render(PtLight *pList, int count, BYTE* pData, int size)
{
    Clear(true,false,true);
    //int tmpNum = count*m_nWidth*m_nHeight;
    //if ( tmpNum > m_nLkListNum || m_pLinkList == NULL )
    //{
    //    Clear(false,false,true);
    //    m_nLkListNum = tmpNum;
    //    m_pLinkList = new LinkListUnit[m_nLkListNum];
    //}
    int tmpNum = 0;

    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    _Vec4* tmpSrc = vecPool.Get();
    _Vec4* tmpPrjed = vecPool.Get();
    _Vec4* tmpDst = vecPool.Get();
    for ( int i = 0; i < count; i++ )
    {
        tmpLt = i;
        tmpSrc->P.P = pList[i].pos;
        tmpSrc->P.w = 1.0f;
        Easy_matrix_mult_vector4X4((__m128*)tmpPrjed,(__m128*)matProj,*((__m128*)tmpSrc));
        r = 1.0f / tmpPrjed->P.w;
        Easy_vector_scalar_mul((__m128*)tmpPrjed,*((__m128*)tmpPrjed),r);
        Easy_matrix_mult_vector4X4((__m128*)tmpDst,(__m128*)matVP,*((__m128*)tmpPrjed));

        x = tmpDst->Q.x;
        y = tmpDst->Q.y;
        r *= pList[i].range;

        // z refuse
        if ( tmpDst->Q.z + r < 0.0f || tmpDst->Q.z - r > 1.0f )
        {
            continue;
        }

        m_Raster.Raster(x,y,r*m_nHeight);
    }

    m_nLightDataNum = tmpIdx;
    if ( tmpIdx > m_nLtListNum || m_pLightList == NULL )
    {
        tmpNum = tmpIdx;
        Clear(false,true,false);
        m_nLtListNum = tmpNum;
        m_pLightList = (BYTE*)malloc(m_nLtListNum*size);
    }
    int oldOffset = 0;
    tmpIdx = 0;
    for ( int i = 0; i < m_nWidth*m_nHeight; i++ )
    {
        oldOffset = m_pIdxList[i].offset;
        if ( oldOffset < 0 )
            continue;
        m_pIdxList[i].offset = tmpIdx;
        for ( int j = 0; j < m_pIdxList[i].count; j++ )
        {
            memcpy(m_pLightList+tmpIdx*size,pData+m_LinkList[oldOffset].light*size,size);
            oldOffset = m_LinkList[oldOffset].next;
            tmpIdx++;
        }
    }

    vecPool.Put(tmpSrc);
    vecPool.Put(tmpPrjed);
    vecPool.Put(tmpDst);
}

void RenderLights::Clear(bool index, bool ltList, bool lkList )
{
    tmpIdx = 0;
    tmpLt = 0;
    if ( index )
    {
        for ( int i = 0; i < m_nWidth*m_nHeight; i++ )
        {
            m_pIdxList[i].offset = -1;
            m_pIdxList[i].count = 0;
        }
    }


    if ( ltList && m_pLightList )
    {
        free(m_pLightList);
        m_pLightList = NULL;
        m_nLtListNum = 0;
    }
    //if ( lkList && m_pLinkList )
    //{
    //    delete [] m_pLinkList;
    //    m_pLinkList = NULL;
    //    m_nLkListNum = 0;
    //}
    if ( lkList )
    {
        m_LinkList.Reverse(m_nWidth*m_nHeight);
    }
}

void RenderLights::SetMaxLightPerPixel( int size )
{
    maxLt = size;
}

