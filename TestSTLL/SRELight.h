
#ifndef _SRELIGHT_H_
#define _SRELIGHT_H_

#include "SRERasterizer.h"
#include "EasyMath.h"
#include "TAList.h"

struct PtLight
{
    _Vec3 pos;
    float range;
    _Vec4 color;
};

struct ListIndexUnit
{
    int offset;
    int count;
};

struct LinkListUnit
{
    int light;
    int next;
};

class RenderLights : public RCCallback
{
public:
    RenderLights( int width, int height );
    ~RenderLights();
    void OnRaster( int x, int y );
    void SetMatrix( _Mat4* proj );
    void Render( PtLight* pList, int count, BYTE* pData, int size );
    void Clear( bool index, bool ltList, bool lkList );
    void SetMaxLightPerPixel( int size );
public:
    int m_nWidth;
    int m_nHeight;
    ListIndexUnit* m_pIdxList;
    BYTE* m_pLightList;
    int m_nLtListNum;
    int m_nLightDataNum;
    CTAList<LinkListUnit> m_LinkList;
    int m_nLkListNum;
protected:
    Vec4Pool vecPool;
    Mat4Pool matPool;
    _Mat4* matProj;
    _Mat4* matVP;
    int tmpLt;
    int tmpIdx;
    int maxLt;
    RasterCircle m_Raster;
};

#endif
