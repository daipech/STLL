#include "stll.h"
#include <windows.h>
#include "EasyMath.h"
#include "STLLRasterizer.h"

STLLRender *render;

void InitSTLL(int w,int h,int r)
{
    render = new STLLRender(w, h, r);
}

void UpdataSTLL(
	_STLLLight* pList, 
	int count, 
	_STLLLight* pData,
	float fov,
	float aspectRatio,
	float n,
	float f,
	int* &lightIdbuf ,
	unsigned char * &lightDataBuf,
	int & lightDataCount)
{
    render->Clear();
    render->SetCamera(fov, aspectRatio, n, f);
    render->Render(pList, count);
    lightIdbuf = (int *)render->GetMap()->GetIndicsTexture();
    lightDataBuf = (unsigned char *)render->GetMap()->GetLightsTexture();
    lightDataCount = render->GetMap()->GetLightCount();
}

void DeinitSTLL()
{
    delete render;
}
