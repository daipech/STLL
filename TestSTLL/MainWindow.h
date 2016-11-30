#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>

struct MainCallback
{
    virtual void OnCreate( int nWidth, int nHeight ) = 0;
    virtual void OnFrame( DWORD* pBmp ) = 0;
    virtual void OnPost(HDC hdc, const RECT& rect) = 0;
    virtual void OnDestroy() = 0;
};

class MainApp
{
public :
    MainApp( HWND hwnd );
    ~MainApp();

    void Create( int nWidth, int nHeight );
    void Frame();
    void Destroy();

    void SetCallback( MainCallback* pCallback );
public:
    HWND m_hWnd;
    HDC m_hDC;
    HDC m_hMemDC;
    HBITMAP m_hDIB;
    void* m_pData;
    int m_nWidth;
    int m_nHeight;
    MainCallback* m_pCallback;
};



