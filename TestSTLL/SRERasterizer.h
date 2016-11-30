
#ifndef _SRERASTERIZER_H_
#define _SRERASTERIZER_H_

struct RCCallback
{
    virtual void OnRaster( int x, int y ) = 0;
};

struct RCLine
{
    int nStart;
    int nEnd;
};

class RasterCircle
{
public:
    static void Init( int maxRadius );
public:
    RasterCircle( int x, int y, int width, int height );
    ~RasterCircle();
    void SetCallback( RCCallback* callback );
    void Raster( float x, float y, float radius );
protected:
    RCLine GetLine( float x, float y, float k, float r, int clip );
protected:
    int m_nX;
    int m_nY;
    int m_nWidth;
    int m_nHeight;
    RCCallback* m_pCallback;
};

class RasterCircle2
{
public:
    RasterCircle2(int x, int y, int width, int height);
    ~RasterCircle2();
    void SetCallback(RCCallback* callback);
    void Raster(float x, float y, float radius);
protected:
    int m_nX;
    int m_nY;
    int m_nWidth;
    int m_nHeight;
    RCCallback* m_pCallback;
};

#endif
