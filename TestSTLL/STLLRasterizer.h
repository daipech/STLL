#pragma once

#include <thread>
#include <atomic>

typedef std::atomic<int> AtomicInt;

class AsyncList
{
public:
    static int GetSize()
    {
        return m_size;
    }

    static void SetSize(int size) {
        m_size = size;
    }
public:
    AsyncList() {}
    AsyncList(int size);
    ~AsyncList();

    __forceinline int Add(int val)
    {
        int old = m_pos.fetch_add(1);
        if (old < m_size)
        {
            m_buffer[old] = val;
            return 1;
        }
        else
        {
            m_pos--;
            return 0;
        }
    }

    __forceinline int Get(int index) {
        assert(index < m_size);
        return m_buffer[index];
    }

    __forceinline int Position()
    {
        return m_pos.load();
    }

    __forceinline void Clear()
    {
        m_pos = 0;
    }
private:
    static int m_size;
private:
    AtomicInt m_pos;
    int* m_buffer;
};

class STLLLightListMap
{
public:
    STLLLightListMap(int width, int height, int size);
    ~STLLLightListMap();

    __forceinline void AddToTile(int x, int y, int val)
    {
        assert(x<m_width && y<m_height);
        int rt = m_tiles[y*m_width + x].Add(val);
        m_count += rt;
    }

    __forceinline void AddToTile(int idx, int val)
    {
        assert(idx < (m_width*m_height));
        m_count += m_tiles[idx].Add(val);
    }

    __forceinline void AddLine(int y, int xStart, int xEnd, int val)
    {
        assert(y < m_height);
        xStart = max(xStart, 0);
        xEnd = min(xEnd, m_width);
        int offset = y*m_width;
        for (int i = xStart; i < xEnd; i++)
        {
            m_count += m_tiles[offset + i].Add(val);
        }
    }

    __forceinline void Clear()
    {
        m_count = 0;
        for (int i = 0; i < m_length; i++)
        {
            m_tiles[i].Clear();
        }
    }

    void BuildTexture(_STLLLight* lightList, int listSize);

    inline _STLLIndex* GetIndicsTexture()
    {
        return m_indicsTex;
    }

    inline _STLLLight* GetLightsTexture()
    {
        return m_lightsTex;
    }

    __forceinline int GetLightCount()
    {
        return m_count;
    }

    __forceinline int GetWidth() { return m_width; }
    __forceinline int GetHeight() { return m_height; }
private:
    int m_width;
    int m_height;
    int m_length;
    int m_alloc;
    AtomicInt m_count;
    AsyncList* m_tiles;
    _STLLIndex* m_indicsTex;
    _STLLLight* m_lightsTex;
};

class STLLRasterizer
{
public:
    STLLRasterizer(STLLLightListMap* map);
    ~STLLRasterizer();

    void SetTarget(STLLLightListMap* map);
    STLLLightListMap* GetTarget();

    void SetProjection(_Mat4* proj);
    _Mat4* GetProjection();

    inline void Rasterize(float x, float y, float radius, int index);
    inline void DrawLight(_STLLLight* light, int index);
protected:
    STLLLightListMap* m_map;
    Mat4Pool matPool;
    _Mat4* matProj;
    _Mat4* matVP;
};


class STLLRender
{
public:
    STLLRender(int width, int height, int range);
    ~STLLRender();

    STLLLightListMap* GetMap() { return m_map; }
    STLLRasterizer* GetRasterizer() { return m_raster; }

    void Clear();
    void SetCamera( float fov, float aspectRatio, float _near, float _far);
    void Render(_STLLLight* pList, int count);
protected:
    STLLLightListMap* m_map;
    STLLRasterizer* m_raster;
};
