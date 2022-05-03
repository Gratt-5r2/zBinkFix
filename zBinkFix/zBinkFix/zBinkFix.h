// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
#define BINKSURFACE32A 5
#define BINKCOPYALL    0x80000000L
#define BINKNOSKIP     0x00080000L

	typedef long( __stdcall* BinkDoFrameFunc )(void* bink);
	typedef long( __stdcall* BinkCopyToBufferFunc )(void* bink, void* dest, long destpitch, ulong destheight, ulong destx, ulong desty, ulong flags);
	typedef void( __stdcall* BinkBufferBlitFunc )(void* buf, void* rects, ulong numrects);

	extern BinkDoFrameFunc BinkDoFrame;
	extern BinkCopyToBufferFunc BinkCopyToBuffer;
	extern BinkBufferBlitFunc BinkBufferBlit;

	extern void sysEvent();
	extern int max2( const int& x, const int& y );
	extern int max3( const int& x, const int& y, const int& z );
	extern bool ReadOption( const string& section, const string& value );
}