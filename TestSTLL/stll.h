#pragma once

/*
 * STLL point Light structure
 */
struct _STLLLight
{
    float pos[3];    //point light position
    float range;     //lighting range
    float color[4];  //lighting color
};

/*
 * STLL index texture 'pixel' format
 */
struct _STLLIndex
{
    int offset;  //light list texture offset
    int count;   //lights number of this tile
};

/*
 * initialize STLL
 * w: screen width (in tiles)
 * h: screen height (in tiles)
 * r: the max lighting range (in tiles)
 */
void InitSTLL(int w=32,int h=18,int r=20);

/*
 * update STLL state
 * 
 */
void UpdataSTLL(
	_STLLLight* pList,               //[in]  point light list
	int count,                       //[in]  light list size
	_STLLLight* pData,               //[in]  same as pList
	float fov,                       //[in]  scene camera FOV(Field of view)
	float aspectRatio,               //[in]  scene camera aspect ratio
	float near,                      //[in]  scene camera near clip
	float far,                       //[in]  scene camera far clip
	int* &lightIdbuf,                //[out] light index texture data pointer
	unsigned char * &lightDataBuf,   //[out] light list texture data pointer
	int & lightDataCount);           //[out] light list count

/*
 * finalize STLL
 */
void DeinitSTLL();

