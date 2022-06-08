// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
#define BINKSURFACE32A  5
#define BINKSURFACE32RA 6
#define BINKCOPYALL     0x80000000L
#define BINKNOSKIP      0x00080000L

	struct zImageSize {
		int X;
		int Y;
		int Pitch4;

		zImageSize() : X(0), Y(0), Pitch4(0) {

		}

		zImageSize( const int& x, const int& y, const int& pitch ) {
			X = x;
			Y = y;
			Pitch4 = pitch;
		}

		bool operator == ( const zImageSize& other ) const {
			return X == other.X && Y == other.Y;
		}

		bool operator < ( const zImageSize& other ) const {
			return X < other.X && Y < other.Y;
		}

		bool operator > ( const zImageSize& other ) const {
			return X > other.X && Y > other.Y;
		}
	};


	class Chronograph {
		double Frequency;
		int64 StartTime;
	public:

		Chronograph() {
			Frequency = 0.0;
			StartTime = 0;
		}

		void StartCounter() {
			LARGE_INTEGER li;
			QueryPerformanceFrequency( &li );
			Frequency = double( li.QuadPart ) / 1000.0;
			QueryPerformanceCounter( &li );
			StartTime = li.QuadPart;
		}

		int64 GetCounter() {
			LARGE_INTEGER li;
			QueryPerformanceCounter( &li );
			return int64( double( li.QuadPart - StartTime ) / Frequency );
		}
	};


	typedef long( __stdcall* BinkDoFrameFunc )(void* bink);
	typedef long( __stdcall* BinkCopyToBufferFunc )(void* bink, void* dest, long destpitch, ulong destheight, ulong destx, ulong desty, ulong flags);
	typedef void( __stdcall* BinkBufferBlitFunc )(void* buf, void* rects, ulong numrects);
	typedef void( __stdcall* BinkGotoFunc )(void* bink, ulong frame, long flags);
	typedef void( __stdcall* BinkSetVolumeFunc )(void* bnk, uint trackid, long volume );

	extern BinkDoFrameFunc BinkDoFrame;
	extern BinkCopyToBufferFunc BinkCopyToBuffer;
	extern BinkBufferBlitFunc BinkBufferBlit;
	extern BinkGotoFunc BinkGoto;

	extern void sysEvent();
	extern int max2( const int& x, const int& y );
	extern int max3( const int& x, const int& y, const int& z );
	extern bool ReadOption( const string& section, const string& value );
	extern char* ResizeTexture( int* srcsize, int dstx, int dsty, void* src, void* dest, unsigned long fmt );
	extern void ResizeImage( void* dstImage, const zImageSize& dstSize, const void* srcImage, const zImageSize& srcSize, const uint& format );
}