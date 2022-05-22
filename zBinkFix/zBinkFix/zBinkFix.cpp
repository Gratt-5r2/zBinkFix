// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {

  int* BinkImageBuffer = Null;

  inline void UpdateBinkBufferSize( int*& buffer, int imageSizeX, int imageSizeY ) {
    int totalLength = imageSizeX * imageSizeY * sizeof( int ) * 10;
    buffer = (int*)shi_realloc( buffer, totalLength );
  }


  void zCBinkPlayer::GetBinkSize( int& x, int& y ) {
    ulong* xy = (ulong*)mVideoHandle;
    x = xy[0];
    y = xy[1];
  }


  inline void GetScreenSize( int& x, int& y ) {
    x = zrenderer->vid_xdim;
    y = zrenderer->vid_ydim;
  }




  int zCBinkPlayer::BeginFrame() {
    if( !IsPlaying() )
      return False;

    zCVideoPlayer::PlayFrame();
    PlayHandleEvents();
    BinkDoFrame( mVideoHandle );
    return True;
  }


  SIZE ToSize( const int& x, const int& y ) {
    SIZE sz;
    sz.cx = x;
    sz.cy = y;
    return sz;
  }


  int zCBinkPlayer::OpenSurface( zTRndSurfaceDesc& surfaceInfo ) {
    if( !zrenderer->Vid_Lock( surfaceInfo ) )
      return False;

    return True;
  }


  int zCBinkPlayer::OpenSurface( RECT& rect, DDSURFACEDESC2& ddsd ) {
    zCRnd_D3D* rnd = dynamic_cast<zCRnd_D3D*>( zrenderer );
    if( !rnd )
      return False;

    ZeroMemory( &ddsd, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( DDSURFACEDESC2 );
    rnd->xd3d_pfrontBuffer->Lock( &rect, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WAIT | DDLOCK_WRITEONLY, Null );
    return True;
  }


  int zCBinkPlayer::CloseSurface() {
    zrenderer->Vid_Unlock();
    return True;
  }


  int zCBinkPlayer::CloseSurface( RECT& rect ) {
    zCRnd_D3D* rnd = dynamic_cast<zCRnd_D3D*>(zrenderer);
    if( !rnd )
      return False;

    rnd->xd3d_pfrontBuffer->Unlock( &rect );
    return True;
  }


#define BINKCPY_FLAGS BINKSURFACE32A | BINKCOPYALL | BINKNOSKIP

  RECT MakeRect( const int& width, const int& height ) {
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    return rect;
  }

  static int InterpolationPixelSize = BinkGetInterpolationPixelSize();

  int zCBinkPlayer::BlitFrame() {
    zImageSize screenSize, videoSize, surfaceSize;
    GetScreenSize( screenSize.X, screenSize.Y );
    GetBinkSize( videoSize.X, videoSize.Y );

    RECT screenRect     = MakeRect( screenSize.X, screenSize.Y );
    RECT imageRect      = MakeRect( videoSize.X,  videoSize.Y );
    RECT surfaceRect    = PlaceImageToScreen( screenRect, imageRect );
    surfaceSize.X       = surfaceRect.right - surfaceRect.left;
    surfaceSize.Y       = surfaceRect.bottom - surfaceRect.top;
    surfaceSize.Pitch32 = screenSize.Pitch32;

    DDSURFACEDESC2 ddsd;
    if( !OpenSurface( surfaceRect, ddsd ) )
      return False;

    surfaceSize.Pitch32 = ddsd.lPitch / sizeof( int );
    videoSize.Pitch32 = videoSize.X;
    UpdateBinkBufferSize( BinkImageBuffer, videoSize.X, videoSize.Y );

    BinkCopyToBuffer( mVideoHandle, BinkImageBuffer, videoSize.Pitch32 * sizeof( int ), videoSize.Y, 0, 0, BINKCPY_FLAGS);

    zBinkImage dstImage( (int*)ddsd.lpSurface, surfaceSize );
    zBinkImage srcImage( BinkImageBuffer, videoSize );
    InterpolateImage( dstImage, srcImage, InterpolationPixelSize );

    CloseSurface( surfaceRect );
    return True;
  }


  HOOK Hook_zCBinkPlayer_PlayFrame PATCH_IF( &zCBinkPlayer::PlayFrame, &zCBinkPlayer::PlayFrame_Union, ReadOption( "DEBUG", "FixBink" ) );

  int zCBinkPlayer::PlayFrame_Union() {
    if( !BeginFrame() || !BlitFrame() )
      return False;

    EndFrame();
    return True;
  };


  void zCBinkPlayer::EndFrame() {
    PlayWaitNextFrame();
    PlayGotoNextFrame();
    zrenderer->Vid_Blit( True, Null, Null );
    sysEvent();
  }
}