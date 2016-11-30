
#include "MainWindow.h"
#include "stll.h"
#include "EasyMath.h"
#include "STLLRasterizer.h"
#include "async\async++.h"

#include <new>

__int64 GetCPUClock()
{
    //__asm _emit 0x0F;
    //__asm _emit 0x31;
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t.QuadPart;
}

static LARGE_INTEGER _freq;
static ULONGLONG _ticks;

void GetCPUFreqency() {
    QueryPerformanceFrequency(&_freq);
    _ticks = _freq.QuadPart / 1000;
}

struct Ray
{
    _Vec3 origin;
    _Vec3 direct;
    Ray(const _Vec3& ori, const _Vec3& dir) : origin(ori), direct(dir) {}
    __forceinline float IntersectPlane(float z, _Vec3& point)
    {
        //o.z + n*d.z = z -> n = (z - o.z)/d.z
        float n = (z - origin.Pos.z) / direct.Pos.z;
        point.Pos.x = origin.Pos.x + (n * direct.Pos.x);
        point.Pos.y = origin.Pos.y + (n * direct.Pos.y);
        point.Pos.z = z;
        return n;
    }
};

struct Sphere
{
    float radius;
    float sqRad;
    //_Vec3 pos;

    Sphere(float rad) : radius(rad)
    {
        sqRad = rad*rad;
    }

    __forceinline bool Intersect(const Ray ray, float& dist, _Vec3& point) const
    {
        __m128 tmp[3];
        //tmp0 -> ray.origin
        _Vec4* op = new((void*)&tmp[0]) _Vec4();
        op->f[0] = -ray.origin.f[0];
        op->f[1] = -ray.origin.f[1];
        op->f[2] = -ray.origin.f[2];
        op->f[3] = 1.0f;

        //tmp1 -> ray.direction
        _Vec4* dir = new((void*)&tmp[1]) _Vec4();
        dir->f[0] = ray.direct.f[0];
        dir->f[1] = ray.direct.f[1];
        dir->f[2] = ray.direct.f[2];
        dir->f[3] = 1.0f;

        float eps = 1e-4;
        float b = Easy_vector_dot(tmp[0], tmp[1]); //op * dir
        float det = b*b - Easy_vector_dot(tmp[0], tmp[0]) + sqRad;
        if (det < 0) { return false; }
        else { det = sqrtf(det); }
        float t = b - det;
        if (t < eps)
        {
            t = b + eps;
            if (t < eps)
            {
                return false;
            }
        }
        dist = t;
        _Vec4* nr = new((void*)&tmp[2]) _Vec4();
        Easy_vector_scalar_mul(&tmp[2], tmp[1], t);
        point.f[0] = ray.origin.f[0] + nr->f[0];
        point.f[1] = ray.origin.f[1] + nr->f[1];
        point.f[2] = ray.origin.f[2] + nr->f[2];
        return true;
    }
};

struct Graphixel
{
    _Vec4 pos_dis; // XYZ:position W:distance
    _Vec4 normal;
};

struct GBuffer
{
    Graphixel* pixels;
    int width;
    int height;
    GBuffer(int width, int height)
    {
        this->width = width;
        this->height = height;
        pixels = new Graphixel[width*height];
    }
    ~GBuffer()
    {
        delete[] pixels;
    }
    __forceinline Graphixel& Get(int x, int y)
    {
        return pixels[y*height+x];
    }

    __forceinline void Set(int x, int y, float dist, _Vec3 pos, _Vec4 nor)
    {
        Graphixel* pt = &pixels[y*height + x];
        pt->pos_dis.P.P = pos;
        pt->pos_dis.P.w = dist;
        pt->normal = nor;
    }
};

__forceinline DWORD MakeColor(int r, int g, int b, float e)
{
    int _r = (int)(((float)r) * e);
    int _g = (int)(((float)g) * e);
    int _b = (int)(((float)b) * e);
    return RGB(_b, _g, _r);
}

MainApp::MainApp(HWND hwnd)
{
    m_hWnd = hwnd;
    m_hDC = GetDC(hwnd);
    m_nWidth = 0;
    m_nHeight = 0;
    m_pCallback = NULL;
    m_pData = NULL;
    GetCPUFreqency();
}

MainApp::~MainApp()
{}

void MainApp::Create(int nWidth, int nHeight)
{
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_hMemDC = CreateCompatibleDC(m_hDC);

    BITMAPINFOHEADER bmih;
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = nWidth;
    bmih.biHeight = nHeight;
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = 0;
    bmih.biXPelsPerMeter = 0;
    bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed = 0;
    bmih.biClrImportant = 0;
    m_hDIB = CreateDIBSection(m_hMemDC,(BITMAPINFO*)&bmih,0,&m_pData,NULL,0);

    SelectObject(m_hMemDC,(HGDIOBJ)m_hDIB);
    if ( m_pCallback )
    {
        m_pCallback->OnCreate(nWidth,nHeight);
    }
}

void MainApp::Frame()
{
    if ( m_pCallback )
    {
        m_pCallback->OnFrame((DWORD*)m_pData);
    }
    BitBlt(m_hDC,0,0,m_nWidth,m_nHeight,m_hMemDC,0,0,SRCCOPY);
    if (m_pCallback)
    {
        RECT rect;
        GetClientRect(m_hWnd, &rect);
        m_pCallback->OnPost(m_hDC,rect);
    }
}

void MainApp::Destroy()
{
    ReleaseDC(m_hWnd,m_hDC);
    if ( m_pCallback )
    {
        m_pCallback->OnDestroy();
    }
}

void MainApp::SetCallback(MainCallback *pCallback)
{
    m_pCallback = pCallback;
}

static const int _P_NUM = 256;
static const float _BALL_SIZE = 7.7f;
static const float _LIGHT_RANGE = 1.3f;
static const float _LIGHT_SIZE = 8.0f;

class STLLRun : public MainCallback
{
public:
    STLLRun()
    {
        m_nWidth = 0;
        m_nHeight = 0;
        m_pBmp = NULL;
        Easy_Init_Angles();
        showHelp = false;
        showTile = false;
        showFPS = true;
        showInfo = false;
        frames = 0;
        FPS = 0;
        start = 0;
    }

    ~STLLRun() {}

    void OnCreate(int nWidth, int nHeight)
    {
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        buffer = new GBuffer(nWidth, nHeight);
        m_nTileSize = 16;
        m_nTileX = nWidth / m_nTileSize;
        m_nTileY = nHeight / m_nTileSize;

        render = new STLLRender(m_nTileX, m_nTileY, _P_NUM);
        render->SetCamera(90.0f, (float)nWidth / (float)nHeight, 1.00f, 200.0f);

        float halfW = nWidth / 2.0f;
        float halfH = nHeight / 2.0f;

        Sphere sp(_BALL_SIZE);

        float z = -15.0f;
        float sW = 10.0f;
        float sH = 7.5f;

        eye.Pos.x = 0.0f;
        eye.Pos.y = 0.0f;
        eye.Pos.z = 20.0f;

        _Vec3 sPt;
        sPt.Pos.x = 0.0f;
        sPt.Pos.y = 0.0f;
        sPt.Pos.z = -10.0f;

        float dist;
        _Vec3 sect;

        __m128 tmp;
        _Vec4* nor = new (&tmp) _Vec4();

        _Vec4 norPlane;
        norPlane.f[0] = 0.0f;
        norPlane.f[1] = 0.0f;
        norPlane.f[2] = 1.0f;
        norPlane.f[3] = 0.0f;

        for (int y = 0; y < nHeight; y++)
        {
            for (int x = 0; x < nWidth; x++)
            {
                nor->f[0] = (float)(x - halfW) / halfW * sW - eye.Pos.x;
                nor->f[1] = (float)(halfH - y) / halfH * sH - eye.Pos.y;
                nor->f[2] = -10.0f;
                nor->f[3] = 1.0f;
                Easy_vector_normalize(&tmp, tmp);
                sPt.Pos.x = nor->f[0];
                sPt.Pos.y = nor->f[1];
                sPt.Pos.z = nor->f[2];
                Ray ray(eye,sPt);
                if (!sp.Intersect(ray, dist, sect))
                {
                    dist = ray.IntersectPlane(z, sect);
                    buffer->Set(x, y, dist, sect, norPlane);
                }
                else
                {
                    nor->P.P = sect;
                    nor->P.w = 1.0f;
                    Easy_vector_normalize(&tmp, tmp);
                    buffer->Set(x, y, dist, sect, *nor);
                }
            }
        }

        //build sparks
        //float step = PI*2.0f / (float)(_P_NUM);
        float step = PI*2.0f / (float)(_P_NUM);
        _Vec3 pos;
        _Mat4 rot;
        for (int i = 0; i < _P_NUM; i++)
        {
            float startPos = rand() % 360;
            startPos *= (PI/180.0f);
            float track = step * i;
            vecAngle[i].f[0] = track;

            vecAngle[i].f[1] = startPos;
            Angle sp = RADIAN(startPos);
            pos.Pos.x = Easy_Cos(sp)*_LIGHT_SIZE;
            pos.Pos.z = Easy_Sin(sp)*_LIGHT_SIZE;
            pos.Pos.y = 0;
            Easy_matrix_identity(rot.f4X4);
            Easy_matrix_rotate(rot.f4X4, track, 0.0f, 0.0f, 1.0f);
            _Vec3 _tmp = pos;
            Easy_vector_transform(pos.f, rot.f4X4, _tmp.f);
            pos.Pos.z -= 20.0f;
            sparks[i].pos[0] = pos.f[0];
            sparks[i].pos[1] = pos.f[1];
            sparks[i].pos[2] = pos.f[2];
            sparks[i].range = _LIGHT_RANGE;
            sparks[i].color[0] = (float)(rand() % 256) / 256.0f;
            sparks[i].color[1] = (float)(rand() % 256) / 256.0f;
            sparks[i].color[2] = (float)(rand() % 256) / 256.0f;
        }

        start = GetTickCount64();
        preFrm = start;
    }

    void Clear()
    {
        memset(m_pBmp, 0xFFFFFFFF, m_nWidth*m_nHeight*sizeof(DWORD));
    }

    void DrawBox(int x, int y, DWORD color)
    {
        if (x < 0 || y < 0 || x >= m_nTileX || y >= m_nTileY)
            return;
        //top bottom
        int top = y * m_nTileSize;
        int bot = top + m_nTileSize - 1;
        int left = x * m_nTileSize;
        int right = left + m_nTileSize - 1;

        int line1 = top*m_nWidth + left;
        int line2 = bot*m_nWidth + left;
        for (int i = 0; i < m_nTileSize; i++)
        {
            m_pBmp[line1 + i] = color;
            m_pBmp[line2 + i] = color;
        }

        for (int j = 0; j < m_nTileSize; j++)
        {
            int line = (top + j)*m_nWidth;
            m_pBmp[line + left] = color;
            m_pBmp[line + right] = color;
        }
    }

    void DrawPoint(int x, int y, DWORD color)
    {
        if (x < 0 || y < 0 || x >= m_nWidth || y >= m_nHeight)
            return;
        m_pBmp[y*m_nWidth + x] = color;
    }

    void DrawCircle(float x, float y, float w, DWORD color)
    {
        int a = (int)x;
        int b = (int)y;
        int r = (int)w;

        int tstart = a - r;
        int tend = a + r;
        for (int tx = tstart; tx < tend; tx++)
        {
            int k = tx - a;
            int l = r*r - k*k;
            int m = (int)sqrt((float)l);
            DrawPoint(tx, b - m, color);
            DrawPoint(tx, b + m, color);
        }
    }

    __forceinline float Clamp(float val, float minVal, float maxVal)
    {
#if 0
        _mm_store_ss(&val, _mm_min_ss(_mm_max_ss(_mm_set_ss(val), _mm_set_ss(maxVal)), _mm_set_ss(minVal)));
#else
        if (val < minVal) { return minVal; }
        else if (val > maxVal) { return maxVal; }
#endif
        return val;
    }

    __forceinline void DrawPixel(int x, int y, _STLLLight* lights, int num)
    {
        __m128 tmp[2];
        _Vec4* light = new (&tmp[0])_Vec4();
        light->f[0] = -0.33f;
        light->f[1] = -0.33f;
        light->f[2] = 0.33f;
        light->f[3] = 1.0f;
        _Vec4* nor = new (&tmp[1])_Vec4();
        //lambertain reflection model
        Graphixel& px = buffer->Get(x, y);
        *nor = px.normal;
        float e = Easy_vector_dot(tmp[0], tmp[1]);
        if (e < 0.0f)
        {
            e = 0.0f;
        }
        else if (e > 1.0f)
        {
            e = 1.0f;
        }
        float color[3];
        color[0] = e;
        color[1] = e;
        color[2] = e;
        for (int i = 0; i < num; i++)
        {
            _STLLLight& lt = lights[i];

            float tmp[3];
            tmp[0] = lt.pos[0];
            tmp[1] = lt.pos[1];
            tmp[2] = lt.pos[2] + 20.0f;
            float dis = Easy_vector_Getlenth_2(tmp, px.pos_dis.f);
            if (dis < lt.range*lt.range)
            {
                float k = sqrtf(dis) / lt.range;
                float att = 1.0f-k*k;
                color[0] += lt.color[0] * att;
                color[1] += lt.color[1] * att;
                color[2] += lt.color[2] * att;
                color[0] = Clamp(color[0], 0.0f, 1.0f);
                color[1] = Clamp(color[1], 0.0f, 1.0f);
                color[2] = Clamp(color[2], 0.0f, 1.0f);
            }
        }
        color[0] *= 255.0f;
        color[1] *= 255.0f;
        color[2] *= 255.0f;
        DrawPoint(x, y, RGB(color[0], color[1], color[2]));
    }

    __forceinline void DrawTile(int x, int y, _STLLLight* lights, int num)
    {
        int startX = x * m_nTileSize;
        int startY = y * m_nTileSize;
        for (int i = 0; i < m_nTileSize; i++)
        {
            for (int j = 0; j < m_nTileSize; j++)
            {
                DrawPixel(startX + j, startY + i, lights, num);
            }
        }
        if (num > 0 && showTile) {
            DrawBox(x, y, 0x00FF0000);
        }
    }

    void OnFrame(DWORD* pBmp)
    {
        ULONGLONG tickStart = GetTickCount64();
        float interval = (float)(tickStart - preFrm)*0.002f;
        m_pBmp = pBmp;
        Clear();

        _Vec3 pos;
        _Mat4 rot;
        //_Mat4* proj = render->GetRasterizer()->GetProjection();
        for (int i = 0; i < _P_NUM; i++)
        {
            float track = vecAngle[i].f[0];
            float startPos = vecAngle[i].f[1] + interval;
            if (startPos > 360.0f)
            {
                startPos -= 360.0f;
            }
            Angle sp = RADIAN(startPos);
            pos.Pos.x = Easy_Cos(sp)*_LIGHT_SIZE;
            pos.Pos.z = Easy_Sin(sp)*_LIGHT_SIZE;
            pos.Pos.y = 0;
            Easy_matrix_identity(rot.f4X4);
            Easy_matrix_rotate(rot.f4X4, track, 0.0f, 0.0f, 1.0f);
            _Vec3 _tmp = pos;
            Easy_vector_transform(pos.f, rot.f4X4, _tmp.f);
            pos.Pos.z -= 20.0f;
            sparks[i].pos[0] = pos.f[0];
            sparks[i].pos[1] = pos.f[1];
            sparks[i].pos[2] = pos.f[2];
            vecAngle[i].f[1] = startPos;
            //float tpos[3];
            //Easy_vector_transform(tpos, proj->f4X4, pos.f);
            //DrawCircle(tpos[0], tpos[1], sparks[i].range, 0x00FF0000);
        }

        ULONGLONG rstStart = GetCPUClock();
        render->Clear();
        render->Render(sparks, _P_NUM);
        STLLLightListMap* map = render->GetMap();
        ULONGLONG rstEnd = GetCPUClock();

#if 0
        for (int y = 0; y < m_nTileY; y++)
        {
            for (int x = 0; x < m_nTileX; x++)
            {
                _STLLIndex idx = map->GetIndicsTexture()[y*m_nTileX+x];
                DrawTile(x, y, &map->GetLightsTexture()[idx.offset], idx.count);
            }
        }
#else
        int count = m_nTileX*m_nTileY;
        async::parallel_for(async::irange(0, count), [map,this](int i){
            int y = i / m_nTileX;
            int x = i % m_nTileX;
            _STLLIndex idx = map->GetIndicsTexture()[y*m_nTileX + x];
            DrawTile(x, y, &map->GetLightsTexture()[idx.offset], idx.count);
        });
#endif

        ULONGLONG tickEnd = GetTickCount64();

        if (tickEnd - start > 1000)
        {
            FPS = frames;
            frames = 0;
            start = tickEnd;
        }
        else
        {
            frames++;
        }
        frmTime = tickEnd - tickStart;
        rstTime = (rstEnd - rstStart) / _ticks;
        preFrm = tickStart;
    }

    void PrintText(HDC hdc, int x, int y, const CHAR* format, ...)
    {
        CHAR buf[1024];
        va_list vas;
        va_start(vas, format);
        int cnt = vsnprintf_s(buf, 1024, format, vas);
        va_end(vas);
        TextOut(hdc, x, y, buf, cnt);
    }

    void OnPost(HDC hdc, const RECT& rect)
    {
        if (showHelp)
        {
            PrintText(hdc, rect.left + 1, rect.top + 0, "Press F to show/hide FPS");
            PrintText(hdc, rect.left + 1, rect.top + 15, "Press T to show/hide Tiles");
            PrintText(hdc, rect.left + 1, rect.top + 30, "Press I to show/hide Info");
            PrintText(hdc, rect.left + 1, rect.top + 45, "Press H to hide Help");
        }
        else
        {
            PrintText(hdc, rect.left+1, rect.top+1, "Press H to show help");
        }

        if (showFPS)
        {
            PrintText(hdc, rect.right - 70, rect.top + 0, "FPS:%d", FPS);
        }
        if (showInfo)
        {
            PrintText(hdc, rect.right - 100, rect.top + 15, "LIGHTS:%d", _P_NUM);
            PrintText(hdc, rect.right - 100, rect.top + 30, "FRAME:%d", frmTime);
            PrintText(hdc, rect.right - 100, rect.top + 45, "RASTER:%d", rstTime);
            PrintText(hdc, rect.right - 100, rect.top + 60, "LIST:%d", render->GetMap()->GetLightCount());
        }
    }

    void OnDestroy()
    {
        delete render;
        delete buffer;
    }

public:
    bool showHelp;
    bool showTile;
    bool showFPS;
    bool showInfo;

protected:
    int m_nWidth;
    int m_nHeight;
    DWORD* m_pBmp;
    GBuffer* buffer;
    _Vec3 eye;
    int m_nTileX;
    int m_nTileY;
    int m_nTileSize;
    STLLRender* render;
    _STLLLight sparks[_P_NUM];
    _Vec3 vecAngle[_P_NUM];
    ULONGLONG start;
    int frames;
    int FPS;
    int frmTime;
    int rstTime;
    ULONGLONG preFrm;
};
// Global variable 
 
HINSTANCE hinst; 
HDC hdc;
PAINTSTRUCT ps;
HWND hWnd;

MainApp* pApp = NULL;
STLLRun* pRun = NULL;

static const int _WIDTH = 1280;
static const int _HEIGHT = 960;

// Function prototypes. 

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int); 
BOOL InitApplication(HINSTANCE); 
BOOL InitInstance(HINSTANCE, int); 
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM); 
 
// Application entry point. 
 
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, int nCmdShow) 
{ 
    MSG msg;
 
    if (!InitApplication(hinstance)) 
        return FALSE; 
 
    if (!InitInstance(hinstance, nCmdShow)) 
        return FALSE; 
 
    //HDC tmp = GetDC(hWnd);
    BOOL fGotMessage;
    while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 && fGotMessage != -1) 
    { 
        TranslateMessage(&msg);
        DispatchMessage(&msg); 

        //SetPixel(tmp,100,100,0x000000FF);
        
        if ( pApp )
        {
            pApp->Frame();
            Sleep(0);
        }
    } 
    //ReleaseDC(hWnd,tmp);

    if ( pApp )
    {
        pApp->Destroy();
        delete pApp;
        delete pRun;
    }

    return msg.wParam; 
        UNREFERENCED_PARAMETER(lpCmdLine); 
} 
 
BOOL InitApplication(HINSTANCE hinstance) 
{ 
    WNDCLASSEX wcx; 
 
    // Fill in the window class structure with parameters 
    // that describe the main window. 
 
    wcx.cbSize = sizeof(wcx);          // size of structure 
    wcx.style = CS_HREDRAW | 
        CS_VREDRAW;                    // redraw if size changes 
    wcx.lpfnWndProc = MainWndProc;     // points to window procedure 
    wcx.cbClsExtra = 0;                // no extra class memory 
    wcx.cbWndExtra = 0;                // no extra window memory 
    wcx.hInstance = hinstance;         // handle to instance 
    wcx.hIcon = LoadIcon(NULL, 
        IDI_APPLICATION);              // predefined app. icon 
    wcx.hCursor = LoadCursor(NULL, 
        IDC_ARROW);                    // predefined arrow 
    wcx.hbrBackground = (HBRUSH)GetStockObject( 
        WHITE_BRUSH);                  // white background brush 
    wcx.lpszMenuName =  "MainMenu";    // name of menu resource 
    wcx.lpszClassName = "TestSTLL";  // name of window class 
    wcx.hIconSm = (HICON)LoadImage(hinstance, // small class icon 
        MAKEINTRESOURCE(5),
        IMAGE_ICON, 
        GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), 
        LR_DEFAULTCOLOR); 
 
    // Register the window class. 
 
    return RegisterClassEx(&wcx); 
} 
 
BOOL InitInstance(HINSTANCE hinstance, int nCmdShow) 
{ 
    HWND hwnd; 
 
    // Save the application-instance handle. 
 
    hinst = hinstance; 
 
    // Create the main window. 
 
    hwnd = CreateWindow( 
        "TestSTLL",         // name of window class 
        "TestSTLL",         // title-bar string 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, // top-level window 
        CW_USEDEFAULT,       // default horizontal position 
        CW_USEDEFAULT,       // default vertical position 
        _WIDTH,              // default width 
        _HEIGHT+40,          // default height 
        (HWND) NULL,         // no owner window 
        (HMENU) NULL,        // use class menu 
        hinstance,           // handle to application instance 
        (LPVOID) NULL);      // no window-creation data 
 
    if (!hwnd)
    {
        DWORD err = GetLastError();
        char buf[100];
        sprintf(buf, "wtf:%d", err);
        MessageBoxA(NULL, buf, "error", MB_OK);
        return FALSE; 
    }
    hWnd = hwnd;
    pApp = new MainApp(hwnd);
    pRun = new STLLRun();
    pApp->SetCallback(pRun);
    pApp->Create(_WIDTH,_HEIGHT);
 
    // Show the window and send a WM_PAINT message to the window 
    // procedure. 
 
    ShowWindow(hwnd, nCmdShow); 
    UpdateWindow(hwnd); 
    return TRUE; 
 
} 

LRESULT CALLBACK MainWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{ 
 
    switch (uMsg) 
    { 
        case WM_CREATE: 
            // Initialize the window. 
            return 0; 
 
        case WM_PAINT: 
            {
            //hdc = BeginPaint(hwnd, &ps);

            //SetPixel(hdc,3,3,0xFF000000);
            //SetPixel(hdc,3,4,0xFF000000);
            //SetPixel(hdc,4,3,0xFF000000);
            //SetPixel(hdc,4,4,0xFF000000);

            //EndPaint(hwnd, &ps); 
            }
            return 0; 
 
        case WM_SIZE: 
            // Set the size and position of the window. 
            return 0; 
 
        case WM_DESTROY: 
            // Clean up window-specific data objects. 
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            switch (wParam)
            {
                default:
                    break;
            }
            break;
        case WM_KEYUP:
            switch (wParam)
            {
            case 'H':
                pRun->showHelp = !pRun->showHelp;
                break;
            case 'T':
                pRun->showTile = !pRun->showTile;
                break;
            case 'F':
                pRun->showFPS = !pRun->showFPS;
                break;
            case 'I':
                pRun->showInfo = !pRun->showInfo;
                break;
            default:
                break;
            }
            break;
 
        // 
        // Process other messages. 
        // 
 
        default: 
            return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    } 
    return 0; 
} 


