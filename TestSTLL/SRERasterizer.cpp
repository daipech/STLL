
#include "EasyMath.h"
#include "SRERasterizer.h"

struct RCList
{
    float* lines;
};

RCList* g_rcList = NULL;
int g_rcMaxRad = 0;

void RasterCircle::Init(int maxRadius)
{
    g_rcList = new RCList[maxRadius+1];
    g_rcMaxRad = maxRadius;
    for ( int i = 0; i <= maxRadius; i++ )
    {
        int startY = i;
        g_rcList[i].lines = new float[i*2+1];
        for ( int j = 0; j <= i*2; j++ )
        {
            if (i)
            {
                Angle ts = Easy_ASin(((float)i - j)*0.8f / (float)i);
                g_rcList[i].lines[j] = Easy_Cos(ts)*i;
            }
            else
            {
                g_rcList[i].lines[j] = 0.0f;
            }
        }
    }
}

RasterCircle::RasterCircle(int x, int y, int width, int height)
{
    m_nX = x;
    m_nY = y;
    m_nWidth = width;
    m_nHeight = height;
    m_pCallback = NULL;
}

RasterCircle::~RasterCircle()
{
}

void RasterCircle::SetCallback(RCCallback *callback)
{
    m_pCallback = callback;
}


void RasterCircle::Raster(float x, float y, float radius)
{
    if ( m_pCallback == NULL )
    {
        return;
    }

#if 0
    float r = ceilf(radius);

    int clip = (int)(EasyRound(y) - r);
    int startY = max(clip,m_nY);
    int endY = min((int)(EasyRound(y) + r), m_nHeight - 1);
#else
    float r = ceilf(radius);
    int clip = EasyRound(y - r);
    int startY = max(EasyRound(y - radius), m_nY);
    int endY   = min(EasyRound(y + radius), m_nHeight -1);
#endif

    // y refuse and x refuse
    if ( startY > m_nHeight || endY < m_nY ||
        x + radius < m_nX || x - radius > m_nWidth )
    {
        return;
    }
    //clip = startY - clip;
    RCLine line;
    for ( int k = startY; k <= endY; k++ )
    {
        line = GetLine(x,y,((float)k),r,clip);
        line.nStart = max(line.nStart,m_nX);
        line.nEnd = min(line.nEnd,m_nWidth-1);
        for ( int j = line.nStart; j <= line.nEnd; j++ )
        {
            m_pCallback->OnRaster(j,k);
        }
    }
}

RCLine RasterCircle::GetLine(float x, float y, float k, float r, int clip)
{
    float tmp = 0.0f;
    RCLine line;
    if ( ((int)r) > g_rcMaxRad )
    {
        Angle ts = Easy_ASin((y-k)*0.8f/(r));
        tmp = ceilf(Easy_Cos(ts) * (r));
    }
    else
    {
        tmp = ceilf(g_rcList[(int)(r)].lines[(int)(k-clip)]);
    }
    line.nStart = (int)(EasyRound(x) - tmp);
    line.nEnd = (int)(EasyRound(x) + tmp);
    return line;
}


RasterCircle2::RasterCircle2(int x, int y, int width, int height)
{
    m_nX = x;
    m_nY = y;
    m_nWidth = width;
    m_nHeight = height;
    m_pCallback = NULL;
}

RasterCircle2::~RasterCircle2()
{
}

void RasterCircle2::SetCallback(RCCallback *callback)
{
    m_pCallback = callback;
}


void RasterCircle2::Raster(float x, float y, float radius)
{
    if (m_pCallback == NULL)
    {
        return;
    }

    float r = radius + 0.5f;
    float start = y - r;
    float end   = y + r;
    float tR    = r * r;

    // y refuse and x refuse
    if (start > m_nHeight || end < m_nY ||
        x + radius < m_nX || x - radius > m_nWidth)
    {
        return;
    }

    for (float i = start; i < end; i += 0.9999999f)
    {
        int h = 0;
        if (i < y)
        {
            h = max((int)EasyRound(i), m_nY);
        }
        else
        {
            h = min((int)EasyRound(i), m_nHeight - 1);
        }

        float k  = h - y;
        float l  = tR - k*k;
        float m = sqrt(l);
        int from = max((int)floorf(x - m), m_nX);
        int to   = min((int)ceilf(x + m), m_nWidth - 1);
        for (int j = from; j <= to; j++)
        {
            m_pCallback->OnRaster(j, h);
        }
    }
}




