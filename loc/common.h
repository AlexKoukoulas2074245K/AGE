#pragma once

using byte    = unsigned char;
using word    = unsigned short;
using dword   = unsigned int;
using timer_t = unsigned long;

inline int pow2c(dword n)
{
	--n;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	++n;
	return n;
}

inline int max(const dword a, const dword b) 
{
	return a > b ? a : b;
}