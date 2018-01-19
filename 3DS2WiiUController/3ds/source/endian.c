#include <3ds.h>

u16 fixEndian16(u16 n)
{
	u8 c1 = n & 255;
	u8 c2 = (n >> 8) & 255;
	return ((u16)c1 << 8) + (u16)c2;
}

u32 fixEndian32(u32 n)
{
	u8 c1 = n & 255;
	u8 c2 = (n >> 8) & 255;
	u8 c3 = (n >> 16) & 255;
	u8 c4 = (n >> 24) & 255;
	return ((u32)c1 << 24) + ((u32)c2 << 16) + ((u32)c3 << 8) + (u32)c4;
}