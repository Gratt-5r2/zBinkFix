// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
#define BINKCPY_FLAGS BINKSURFACE32A | BINKCOPYALL | BINKNOSKIP

  static bool IsFixBinkEnabled             = CkeckFixBinkEnabled();
  static int* BinkImageBuffer              = Null;
  static int InterpolationPixelSize        = BinkGetInterpolationPixelSize();
  static int InterpolationPixelSizeCurrent = InterpolationPixelSize;
  static uint NextFrameTime                = Invalid;


  inline void UpdateBinkBufferSize( int*& buffer, int imageSizeX, int imageSizeY ) {
    int totalLength = imageSizeX * imageSizeY * sizeof( int ) * 10;
    buffer = (int*)shi_realloc( buffer, totalLength );
  }


  uint TimeAccumulator( const uint& time ) {
    static uint totalTime = 0;
    static const uint timelineLength = 30;
    static uint timeLinePosition = 0;
    static uint timeLine[timelineLength];

    if( time == Invalid ) {
      totalTime = 0;
      timeLinePosition = 0;
      ZeroMemory( &timeLine, sizeof( timeLine ) );
      return 0;
    }

    uint id = timeLinePosition++ % timelineLength;
    totalTime -= timeLine[id];
    totalTime += time;
    timeLine[id] = time;
    return totalTime / timelineLength;
  }


  RECT MakeRect( const int& width, const int& height ) {
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    return rect;
  }


  void zCBinkPlayer::GetBinkSize( int& x, int& y ) {
    ulong* xy = (ulong*)mVideoHandle;
    x = xy[0];
    y = xy[1];
  }


  ulong zCBinkPlayer::GetBinkFrameRate() {
    ulong frameRate = ((ulong*)mVideoHandle)[5];
    if( frameRate > 60 )
      frameRate = 60;
    else if( frameRate == 0 )
      frameRate = 1;

    return frameRate;
  }


  ulong zCBinkPlayer::GetBinkFrame() {
    return ((ulong*)mVideoHandle)[3];
  }


  void zCBinkPlayer::SetBinkFrame( const ulong& frame ) {
    ((ulong*)mVideoHandle)[3] = frame;
  }


  ulong zCBinkPlayer::GetBinkFramesCount() {
    return ((ulong*)mVideoHandle)[2];
  }


  int zCBinkPlayer::BeginFrame() {
    sysEvent();
    if( !IsPlaying() )
      return False;

    zCVideoPlayer::PlayFrame();
    PlayHandleEvents();
    BinkDoFrame( mVideoHandle );
    return True;
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


  int zCBinkPlayer::BlitFrame( const bool& idle ) {
    if( !idle && NextFrameTime != Invalid ) {
      if( Timer::GetTime() < NextFrameTime )
        return True;
      
      NextFrameTime = Invalid;
    }

    zImageSize screenSize, videoSize, surfaceSize;
    GetScreenSize( screenSize.X, screenSize.Y );
    GetBinkSize( videoSize.X, videoSize.Y );

    RECT screenRect    = MakeRect( screenSize.X, screenSize.Y );
    RECT imageRect     = MakeRect( videoSize.X,  videoSize.Y );
    RECT surfaceRect   = PlaceImageToScreen( screenRect, imageRect );
    surfaceSize.X      = surfaceRect.right - surfaceRect.left;
    surfaceSize.Y      = surfaceRect.bottom - surfaceRect.top;
    surfaceSize.Pitch4 = screenSize.Pitch4;

    DDSURFACEDESC2 ddsd;
    if( !OpenSurface( surfaceRect, ddsd ) )
      return False;

    surfaceSize.Pitch4 = ddsd.lPitch / sizeof( int );
    videoSize.Pitch4 = videoSize.X;

    if( idle ) {
      BinkInterpolationTable.Update( surfaceSize, videoSize );
      UpdateBinkBufferSize( BinkImageBuffer, videoSize.X, videoSize.Y );
      CloseSurface( surfaceRect );
      return True;
    }

    BinkCopyToBuffer( mVideoHandle, BinkImageBuffer, videoSize.Pitch4 * sizeof( int ), videoSize.Y, 0, 0, BINKCPY_FLAGS);

    zBinkImage dstImage( (int*)ddsd.lpSurface, surfaceSize );
    zBinkImage srcImage( BinkImageBuffer, videoSize );
    ResizeImage( dstImage, srcImage, InterpolationPixelSizeCurrent );
    CloseSurface( surfaceRect );
    return True;
  }


  HOOK Hook_zCBinkPlayer_OpenVideo PATCH_IF( &zCBinkPlayer::OpenVideo, &zCBinkPlayer::OpenVideo_Union, IsFixBinkEnabled );

#pragma warning(push)
#pragma warning(disable:4244)
  int zCBinkPlayer::OpenVideo_Union( zSTRING name ) {
    TimeAccumulator( Invalid );
    InterpolationPixelSizeCurrent = InterpolationPixelSize;
    int ok = THISCALL( Hook_zCBinkPlayer_OpenVideo )( name );
    if( ok ) {
      BlitFrame( true );
      if( dynamic_cast<zCSndSys_MSS*>(zsound) )
        BinkSetVolume( mVideoHandle, 0, mSoundOn ? 65536 * mSoundVolume : 0 );
    }

    return ok;
  }
#pragma warning(pop)



  HOOK Hook_zCBinkPlayer_PlayFrame PATCH_IF( &zCBinkPlayer::PlayFrame, &zCBinkPlayer::PlayFrame_Union, IsFixBinkEnabled );

  int zCBinkPlayer::PlayFrame_Union() {
    static Chronograph chrono;
    chrono.StartCounter();

    if( !BeginFrame() || !BlitFrame() )
      return False;

    int64 counter = chrono.GetCounter()/* - 5*/;
    ulong spd = 1000 / GetBinkFrameRate();
    if( counter > spd ) {
      uint now = Timer::GetTime();
      uint dim = counter - spd + 5;
      NextFrameTime = now + dim;
      
      static bool optimizationEnabled = BinkGetDecreaseQuelityWithLags();
      if( optimizationEnabled ) {
        if( InterpolationPixelSizeCurrent < 4 && InterpolationPixelSizeCurrent != 0 ) {
          uint avgTime = TimeAccumulator( counter );
          if( avgTime > spd + spd / 4 ) {
            if( ++InterpolationPixelSizeCurrent >= 4 )
              InterpolationPixelSizeCurrent = 0;

            cmd << "Reduce quality:" << InterpolationPixelSizeCurrent << endl;
            TimeAccumulator( Invalid );
          }
        }
      }
    }

    EndFrame();
    return True;
  };


  void zCBinkPlayer::EndFrame() {
    PlayWaitNextFrame();
    PlayGotoNextFrame();
    zrenderer->Vid_Blit( True, Null, Null );
  }



  HOOK Hook_zCBinkPlayer_PlayHandleEvents PATCH_IF( &oCBinkPlayer::PlayHandleEvents, &oCBinkPlayer::PlayHandleEvents_Union, IsFixBinkEnabled );

#pragma warning(push)
#pragma warning(disable:4244)
  int oCBinkPlayer::PlayHandleEvents_Union() {
    if( disallowInputHandling || !mVideoHandle || zCBinkPlayer::PlayHandleEvents() )
      return FALSE;

    uint16 key = zinput->GetKey( False, False );
    zinput->ProcessInputEvents();

    if( !extendedKeys ) {
      switch( key ) {
        case KEY_SPACE:
        case KEY_ESCAPE:
          Stop();
          return True;
      }

      return False;
    }

    switch( key ) {
      case KEY_HOME:
      {
        BinkGoto( mVideoHandle, 1, 0 );
        return True;
      }

      case KEY_RIGHT:
      {
        ulong nextFrame = GetBinkFrame() + 30;
        ulong framesNum = GetBinkFramesCount();
        if( nextFrame >= framesNum )
          nextFrame = framesNum - 1;

        BinkGoto( mVideoHandle, nextFrame, 1 );
        return True;
      }

      case KEY_LEFT:
      {
        ulong nextFrame = GetBinkFrame() - min( GetBinkFrame(), 30 );
        BinkGoto( mVideoHandle, nextFrame, 1 );
        return True;
      }

      case KEY_SPACE:
      {
        mPaused ? Unpause() : Pause();
        return True;
      }

      case KEY_ESCAPE:
      {
        Stop();
        return True;
      }

      case KEY_DOWN:
      {
        mSoundVolume -= 0.1f;
        if( mSoundVolume < 0.0f )
          mSoundVolume = 0.0f;

        BinkSetVolume( mVideoHandle, 0, 65536 * mSoundVolume );
        return True;
      }

      case KEY_UP:
      {
        mSoundVolume += 0.1f;
        if( mSoundVolume > 1.0f )
          mSoundVolume = 1.0f;

        BinkSetVolume( mVideoHandle, 0, 65536 * mSoundVolume );
        return True;
      }

      case KEY_Q:
      {
        if( mSoundOn ) {
          BinkSetVolume( mVideoHandle, 0, 0 );
          mSoundOn = False;
        }
        else {
          BinkSetVolume( mVideoHandle, 0, 65536 * mSoundVolume );
          mSoundOn = True;
        }
        return True;
      }
    }
  }
#pragma warning(pop)
}