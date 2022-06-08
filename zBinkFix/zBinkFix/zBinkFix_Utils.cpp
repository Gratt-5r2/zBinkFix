// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  void* BinkImport( const string& symbol ) {
    HMODULE module = GetModuleHandle( "binkw32.dll" );
    return GetProcAddress( module, symbol );
  }


  BinkDoFrameFunc BinkDoFrame           = (BinkDoFrameFunc)BinkImport( "_BinkDoFrame@4" );
  BinkCopyToBufferFunc BinkCopyToBuffer = (BinkCopyToBufferFunc)BinkImport( "_BinkCopyToBuffer@28" );
  BinkBufferBlitFunc BinkBufferBlit     = (BinkBufferBlitFunc)BinkImport( "_BinkBufferBlit@12" );
  BinkGotoFunc BinkGoto                 = (BinkGotoFunc)BinkImport( "_BinkGoto@12" );
#if ENGINE >= Engine_G2
  BinkSetVolumeFunc BinkSetVolume       = (BinkSetVolumeFunc)BinkImport( "_BinkSetVolume@12" );
#else
  BinkSetVolumeFunc BinkSetVolume       = (BinkSetVolumeFunc)BinkImport( "_BinkSetVolume@8" );
#endif


  static void sysEvent() {
#if ENGINE == Engine_G1
    XCALL( 0x004F6AC0 );
#elif ENGINE == Engine_G1A
    XCALL( 0x00509530 );
#elif ENGINE == Engine_G2
    XCALL( 0x005026F0 );
#elif ENGINE == Engine_G2A
    XCALL( 0x005053E0 );
#endif
  }


  inline SIZE ToSize( const int& x, const int& y ) {
    SIZE sz;
    sz.cx = x;
    sz.cy = y;
    return sz;
  }


  inline void GetScreenSize( int& x, int& y ) {
    x = zrenderer->vid_xdim;
    y = zrenderer->vid_ydim;
  }


  inline int min2( const int& x, const int& y ) {
    return min( x, y );
  }


  inline int min3( const int& x, const int& y, const int& z ) {
    return min2( min2( x, y ), z );
  }


  inline int max2( const int& x, const int& y ) {
    return max( x, y );
  }


  inline int max3( const int& x, const int& y, const int& z ) {
    return max2( max2( x, y ), z );
  }


  bool ReadOption( const string& section, const string& value, const bool& default = true ) {
    bool result = true;
    Union.GetSysPackOption().Read( result, section, value, default );
    return result;
  }


  inline float SafeDivide( const float& a, const float& b ) {
    return b == 0.0f ? 0.0f : a / b;
  }


  inline int xy2i( const int& x, const int& y, const int& width ) {
    return y * width + x;
  }


  static RECT FitRect( const RECT& screenRect, const RECT& imageRect ) {
    RECT newRect = imageRect;
    if( newRect.left   < 0 )                 newRect.left = 0;
    if( newRect.top    < 0 )                 newRect.top  = 0;
    if( newRect.right  > screenRect.right )  newRect.right = screenRect.right;
    if( newRect.bottom > screenRect.bottom ) newRect.bottom = screenRect.bottom;
    return newRect;
  }


  static RECT AlignToCenter( const RECT& screenRect, const RECT& imageRect ) {
    int wScreen = screenRect.right  - screenRect.left;
    int hScreen = screenRect.bottom - screenRect.top;
    int wImage  = imageRect.right   - imageRect.left;
    int hImage  = imageRect.bottom  - imageRect.top;
    int xOffset = (wScreen - wImage) / 2;
    int yOffset = (hScreen - hImage) / 2;

    RECT newRect;
    newRect.left   = imageRect.left   + xOffset;
    newRect.right  = imageRect.right  + xOffset;
    newRect.top    = imageRect.top    + yOffset;
    newRect.bottom = imageRect.bottom + yOffset;
    return newRect;
  }

#pragma warning(push)
#pragma warning(disable:4244)
  static RECT PlaceImageToScreen( const RECT& screenRect, const RECT& imageRect ) {
    float wScreen = screenRect.right  - screenRect.left;
    float hScreen = screenRect.bottom - screenRect.top;
    float wImage  = imageRect.right   - imageRect.left;
    float hImage  = imageRect.bottom  - imageRect.top;
    float wRatio  = wScreen / wImage;
    float hRatio  = hScreen / hImage;
    float scale   = min( wRatio, hRatio );

    RECT newRect;
    newRect.left   = 0;
    newRect.top    = 0;
    newRect.right  = wImage * scale;
    newRect.bottom = hImage * scale;
    return FitRect( screenRect, AlignToCenter( screenRect, newRect ) );
  }


  static int GetCpusCount() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    return sysinfo.dwNumberOfProcessors;
  }


  static int BinkGetInterpolationThreadsCount() {
    int cpus = GetCpusCount();
    int threadsCount = cpus <= 6 ? cpus : cpus * 0.80;

    Union.GetSysPackOption().Read( threadsCount, "DEBUG", "FixBink_InterpCpuCount", threadsCount );
    return min( threadsCount, INTERPOLATION_INFO_DIM );
  }
#pragma warning(pop)


  static int BinkGetInterpolationPixelSize() {
    int cpus = GetCpusCount();
    int pixelSize;
         if( cpus == 1  ) pixelSize = 0; // Shitbox
    else if( cpus >= 16 ) pixelSize = 1; // Who are you ??
    else                  pixelSize = 2; // Stanrard PC

    Union.GetSysPackOption().Read( pixelSize, "DEBUG", "FixBink_InterpPixelSize", pixelSize );
    return pixelSize;
  }


  static int BinkGetDecreaseQuelityWithLags() {
    bool enabled = true;
    Union.GetSysPackOption().Read( enabled, "DEBUG", "FixBink_ReduceQualityWithLags", enabled );
    return enabled;
  }


  static bool CkeckFixBinkEnabled() {
    if( !CHECK_THIS_ENGINE )
      return false;

    if( Union.Dx11IsEnabled() )
      return false;

    if( ReadOption( "DEBUG", "FixBink", true ) != 1 )
      return false;

    if( ReadOption( "DEBUG", "FixBinkOld", false ) == 1 ) {
      LoadLibraryAST( "zFixBinkOld.dll" );
      return false;
    }

    return true;
  }
}