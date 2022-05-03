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
}