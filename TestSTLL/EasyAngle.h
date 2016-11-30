
#ifndef _EASYANGLE_H
#define _EASYANGLE_H

#ifndef PI
#define PI      (3.14159265359f)
#define DBL_PI  (6.28318530718f)
#define HLF_PI  (1.57079632680f)
#endif

typedef int Angle;

#define MAXANGLE    65535
#define ANGLE(a)    ((int)((a/360.0f)*MAXANGLE))
#define RADIAN(a)   ((int)((a/DBL_PI)*MAXANGLE))
#define TOANGLE(a)  ((360.0f*(a))/(float)MAXANGLE)
#define TORADIAN(a) ((DBL_PI*(a))/(float)MAXANGLE)
#define CLIPANG(a)  ((a) & 0x0000FFFF)

inline int Abs( int x ) {
   int y = x >> 31;
   return ( ( x ^ y ) - y );
}

inline float FAbs( float f ) {
	int tmp = *reinterpret_cast<int *>( &f );
	tmp &= 0x7FFFFFFF;
	return *reinterpret_cast<float *>( &tmp );
}

inline int EasyRound(float f)
{
    unsigned long i;
    __asm fld		f
    __asm fistp		i		// use default rouding mode (round nearest)
    return i;
}

inline Angle FastAngAbs( Angle ang )
{
    int mask = ang >> 31;
    int clip = (ang ^ mask) & 0x0000FFFF;
    return clip + ((MAXANGLE - 2*clip) & mask);
}

#define AngAbs(a) FastAngAbs(a)

void Easy_Init_Angles();

float Easy_Sin( Angle ang );

float Easy_Cos( Angle ang );

float Easy_Tan( Angle ang );

float Easy_Cot( Angle ang );

Angle Easy_ASin( float val );

Angle Easy_ACos( float val );

#endif
