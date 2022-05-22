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


  void sysEvent() {
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


  int max2( const int& x, const int& y ) {
    return max( x, y );
  }


  int max3( const int& x, const int& y, const int& z ) {
    return max2( max2( x, y ), z );
  }


  bool ReadOption( const string& section, const string& value ) {
    bool result = true;
    Union.GetSysPackOption().Read( result, section, value, result );
    return result;
  }


  char* ResizeTexture( int* srcsize, int dstx, int dsty, void* src, void* dest, unsigned long fmt ) {
#if ENGINE == Engine_G1
    XCALL( 0x00720B50 );
#elif ENGINE == Engine_G1A
    XCALL( 0x0075D4E0 );
#elif ENGINE == Engine_G2
    XCALL( 0x0076CB40 );
#elif ENGINE == Engine_G2A
    XCALL( 0x00659670 );
#endif
  }


  inline double __SafeDiv( const double& a, const double& b ) {
    return SafeDiv( a, b );
  }


  inline int __SafeDiv( const int& a, const int& b ) {
    return SafeDiv( a, b );
  }


  inline uint lindex( const uint& x, const uint& y, const uint& width ) {
    return y * width + x;
  }


  RECT FitRect( const RECT& screenRect, const RECT& imageRect ) {
    RECT newRect = imageRect;
    if( newRect.left   < 0 )                 newRect.left = 0;
    if( newRect.top    < 0 )                 newRect.top  = 0;
    if( newRect.right  > screenRect.right )  newRect.right = screenRect.right;
    if( newRect.bottom > screenRect.bottom ) newRect.bottom = screenRect.bottom;
    return newRect;
  }


  RECT AlignToCenter( const RECT& screenRect, const RECT& imageRect ) {
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


  RECT PlaceImageToScreen( const RECT& screenRect, const RECT& imageRect ) {
    float wScreen = screenRect.right  - screenRect.left;
    float hScreen = screenRect.bottom - screenRect.top;
    float wImage  = imageRect.right  - imageRect.left;
    float hImage  = imageRect.bottom - imageRect.top;
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
}