// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
#define BINKCPY_FLAGS BINKSURFACE32A | BINKCOPYALL | BINKNOSKIP
#define USEBACKBUFFER 0
#if USEBACKBUFFER != 0
#define xd3d_Buffer xd3d_pbackBuffer
#else
#define xd3d_Buffer xd3d_pfrontBuffer
#endif

  static bool IsFixBinkEnabled = CkeckFixBinkEnabled();
  static int* BinkImageBuffer  = Null;
  static uint NextFrameTime    = Invalid;


  inline void UpdateBinkBufferSize( int*& buffer, int imageSizeX, int imageSizeY ) {
    int totalLength = imageSizeX * imageSizeY * sizeof( int ) * 10;
    buffer = (int*)shi_realloc( buffer, totalLength );
  }


  // This function accumulates any frame times and returns
  // best average time to define/change the smoothness quality
  uint AccumulateTime( const uint& time, const uint& default = 0 ) {
    static uint totalTime = 0;
    static const uint timelineLength = 60;
    static uint timeLinePosition = 0;
    static uint timeLine[timelineLength];

    if( time == Invalid ) {
      timeLinePosition = 0;
      if( default > 0 ) {
        totalTime = default * timelineLength;
        for( uint i = 0; i < timelineLength; i++ )
          timeLine[i] = default;
      }
      else {
        totalTime = 0;
        ZeroMemory( &timeLine, sizeof( timeLine ) );
      }

      return 0;
    }

    int length = time > totalTime ? 2 : 1;
    for( int i = 0; i <= length; i++ ) {
      uint id = timeLinePosition++ % timelineLength;
      totalTime -= timeLine[id];
      totalTime += time;
      timeLine[id] = time;
    }

    return totalTime / timelineLength;
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


  ulong zCBinkPlayer::GetBinkFrameTime() {
    return ((ulong*)mVideoHandle)[5] / ((ulong*)mVideoHandle)[6];
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


  RECT* zCBinkPlayer::GetBinkRects() {
    return (RECT*)&(((ulong*)mVideoHandle)[13]);
  }


  int zCBinkPlayer::BeginFrame() {
    if( !IsPlaying() )
      return False;

    zCVideoPlayer::PlayFrame();
    PlayHandleEvents();
    BinkDoFrame( mVideoHandle );
    return True;
  }


  int zCBinkPlayer::OpenSurface( RECT& rect, DDSURFACEDESC2& ddsd ) {
    zCRnd_D3D* rnd = dynamic_cast<zCRnd_D3D*>( zrenderer );
    if( !rnd )
      return False;

    ZeroMemory( &ddsd, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( DDSURFACEDESC2 );
    int ok = rnd->xd3d_Buffer->Lock( &rect, &ddsd, DDLOCK_NOSYSLOCK | DDLOCK_WAIT | DDLOCK_WRITEONLY, Null );
    return ok == DD_OK;
  }


  int zCBinkPlayer::CloseSurface() {
    zrenderer->Vid_Unlock();
    return True;
  }


  int zCBinkPlayer::CloseSurface( RECT& rect ) {
    zCRnd_D3D* rnd = dynamic_cast<zCRnd_D3D*>(zrenderer);
    if( !rnd )
      return False;

    // rnd->xd3d_Buffer->Unlock( &rect );
    rnd->xd3d_Buffer->Unlock( 0 );
    return True;
  }


  bool zCBinkPlayer::GetChangedRegion( BINKRECT& region ) {
    // Get all changes areas of the bink frame
    long rectsFound = BinkGetRects( mVideoHandle, 0x8000000 );
    if( !rectsFound )
      return false;

    // Create one big area from smalls
    BINKRECT* rects = GetBinkRects();
    RECT rectDst = BinkRectToRect( rects[0] );
    for( long i = 1; i < rectsFound; i++ ) {
      RECT rectSrc = BinkRectToRect( rects[i] );
      if( rectSrc.left   < rectDst.left   ) rectDst.left   = rectSrc.left;
      if( rectSrc.top    < rectDst.top    ) rectDst.top    = rectSrc.top;
      if( rectSrc.right  > rectDst.right  ) rectDst.right  = rectSrc.right;
      if( rectSrc.bottom > rectDst.bottom ) rectDst.bottom = rectSrc.bottom;
    }

    region = RectToBinkRect( rectDst );
    return true;
  }


  int zCBinkPlayer::BlitFrame( const bool& idle ) {
    BINKRECT changedRegion;
    if( !idle ) {
      // Don't draw this frame if the
      // time of the skip has not passed
      if( NextFrameTime != Invalid ) {
        if( Timer::GetTime() < NextFrameTime )
          return False;

        NextFrameTime = Invalid;
      }

      // Get information about the biggers area which
      // has been changed from the previous frame
      if( !GetChangedRegion( changedRegion ) )
        return False;
    }

    zImageSize screenSize, videoSize, surfaceSize;
    GetScreenSize( screenSize.X, screenSize.Y );
    GetBinkSize( videoSize.X, videoSize.Y );

    // Create the best rectangle proportional to
    // the video in the center of the screen
    RECT screenRect    = MakeRect( screenSize.X, screenSize.Y );
    RECT imageRect     = MakeRect( videoSize.X,  videoSize.Y );
    RECT surfaceRect   = PlaceImageToScreen( screenRect, imageRect );
    surfaceSize.X      = surfaceRect.right - surfaceRect.left;
    surfaceSize.Y      = surfaceRect.bottom - surfaceRect.top;
    surfaceSize.Pitch4 = screenSize.Pitch4;

    // Trying to get surface editable memory
    DDSURFACEDESC2 ddsd;
    if( !OpenSurface( surfaceRect, ddsd ) )
      return False;

    surfaceSize.Pitch4 = ddsd.lPitch / sizeof( int );
    videoSize.Pitch4 = videoSize.X;

    // If this frame if the Idle - update base information
    // about resolution and size factor of the video
    if( idle ) {
      InterpolationPixelSizeLow = surfaceSize.GetScaleFactor( videoSize );
      BinkFixReport( string::Combine( "video quality levels: %i / %i", InterpolationPixelSizeCurrent, InterpolationPixelSizeLow ) );
      BinkInterpolationTable.Update( surfaceSize, videoSize );
      UpdateBinkBufferSize( BinkImageBuffer, videoSize.X, videoSize.Y );
      CloseSurface( surfaceRect );
      return True;
    }

    BinkInterpolationTable.Update( surfaceSize, videoSize );

    // Copying the video data to the buffer
    BinkCopyToBuffer( mVideoHandle, BinkImageBuffer, videoSize.Pitch4 * sizeof( int ), videoSize.Y, 0, 0, BINKCPY_FLAGS );

    // Trying to resize the image
    zBinkImage dstImage( (int*)ddsd.lpSurface, surfaceSize );
    zBinkImage srcImage( BinkImageBuffer, videoSize );
    ResizeImageRegion( dstImage, srcImage, changedRegion, InterpolationPixelSizeCurrent );

    // Apply surface changes
    CloseSurface( surfaceRect );
#if USEBACKBUFFER != False
    zCRnd_D3D* rnd = (zCRnd_D3D*)zrenderer;
    rnd->xd3d_pfrontBuffer->BltFast( surfaceRect.left, surfaceRect.top, rnd->xd3d_pbackBuffer, &surfaceRect, 0 );
#endif
    return True;
  }


  void zCBinkPlayer::ClearScreen() {
    zrenderer->Vid_Clear( (zCOLOR)GFX_BLACK, 3 );
    zrenderer->Vid_Blit( True, Null, Null );
  }


  HOOK Hook_zCBinkPlayer_OpenVideo PATCH_IF( &zCBinkPlayer::OpenVideo, &zCBinkPlayer::OpenVideo_Union, IsFixBinkEnabled );

#pragma warning(push)
#pragma warning(disable:4244)
  int zCBinkPlayer::OpenVideo_Union( zSTRING name ) {
    int ok = THISCALL( Hook_zCBinkPlayer_OpenVideo )( name );
    if( ok ) {
      // Clear the screen and play the idle frame
      // to collect information about the surface
      ClearScreen();
      BlitFrame( true );

      if( dynamic_cast<zCSndSys_MSS*>(zsound) )
        BinkSetVolume( mVideoHandle, 0, mSoundOn ? 65536 * mSoundVolume : 0 );

           // Choose the optimal Quality <-> Speed
           // via screen resolution
           if( IsResMoreThan6K() ) InterpolationPixelSizeCurrent = InterpolationPixelSizeLow + 1;
      else if( IsResMoreThan5K() ) InterpolationPixelSizeCurrent = (InterpolationPixelSize + InterpolationPixelSizeLow) / 2;
      else if( IsResMoreThan4K() ) InterpolationPixelSizeCurrent = InterpolationPixelSize + 1;
      else                         InterpolationPixelSizeCurrent = InterpolationPixelSize;

      // Reset the time accumulator by
      // the expected middle frame time
      AccumulateTime( Invalid, 500 / GetBinkFrameRate() );
    }

    return ok;
  }
#pragma warning(pop)


  HOOK Hook_zCBinkPlayer_CloseVideo PATCH_IF( &zCBinkPlayer::CloseVideo, &zCBinkPlayer::CloseVideo_Union, IsFixBinkEnabled );

  int zCBinkPlayer::CloseVideo_Union() {
    int ok = THISCALL( Hook_zCBinkPlayer_CloseVideo )();
    if( ok ) {
      if( BinkImageBuffer ) {
        shi_free( BinkImageBuffer );
        BinkImageBuffer = Null;
      }
      BinkInterpolationTable.Clear();
    }

    return ok;
  }



  HOOK Hook_zCBinkPlayer_PlayFrame PATCH_IF( &zCBinkPlayer::PlayFrame, &zCBinkPlayer::PlayFrame_Union, IsFixBinkEnabled );

  int zCBinkPlayer::PlayFrame_Union() {
    // Call simplified algorithm if
    // the screen resolution so big
    if( IsResMoreThan4K() )
      return PlayFrameBigScreen();

    static Timer timer;
    static Chronograph chrono;
    chrono.StartCounter();
    if( !BeginFrame() )
      return False;

    // Try to draw bink buffer
    // to the DirectX7 surface
    bool frameBlited  = BlitFrame();
    int& pixelSize    = InterpolationPixelSizeCurrent;
    int& pixelSizeLow = InterpolationPixelSizeLow;
    uint timePassed   = (uint)chrono.GetCounter();
    uint averageTime  = AccumulateTime( timePassed );
    uint videoFps     = GetBinkFrameRate();
    uint videoSpd     = 1000 / videoFps;

    // Skip frames if the video is late
    if( averageTime + 2 >= videoSpd ) {
      uint dim = averageTime + 2 - videoSpd;
      NextFrameTime = Timer::GetTime() + dim;
    }

    // Skip vidblit if
    // video drawing is failed 
    if( !frameBlited ) {
      EndFrame( false );
      return False;
    }

    // Check the average frame time
    // and choose the smoothness quality
    if( timer[1].Await( 500 ) ) {
      if( averageTime > videoSpd * 0.85 ) {
        if( pixelSize <= pixelSizeLow ) {
          pixelSize++;
          BinkFixReport( string::Combine( "quality (-)    lvl: %i / %i    fps: %i / %i", pixelSize, pixelSizeLow, averageTime, videoSpd ) );
          AccumulateTime( 0 );
        }
      }
      else if( pixelSize > InterpolationPixelSizeMin ) {
        if( averageTime < videoSpd * 0.45 ) {
          pixelSize--;
          BinkFixReport( string::Combine( "quality (+)    lvl: %i / %i    fps: %i / %i", pixelSize, pixelSizeLow, averageTime, videoSpd ) );
          AccumulateTime( videoSpd );
        }
      }
    }

    EndFrame();
    return True;
  }


  int zCBinkPlayer::PlayFrameBigScreen() {
    static Timer timer;
    static Chronograph chrono;
    chrono.StartCounter();
    if( !BeginFrame() )
      return False;

    int& pixelSize    = InterpolationPixelSizeCurrent;
    int& pixelSizeLow = InterpolationPixelSizeLow;
    uint videoSpd     = 1000 / GetBinkFrameRate();

    // If video so big, don't use
    // smoothness to accelerate drawing
    int xVid, yVid;
    GetBinkSize( xVid, yVid );
    if( xVid >= 1280 && yVid >= 720 )
      pixelSize = pixelSizeLow + 1;

    // Try to draw bink buffer
    // to the DirectX7 surface
    if( !BlitFrame() ) {
      EndFrame( false );
      return True;
    }

    // Skip frames if the video is late
    uint timePassed = (uint)chrono.GetCounter();
    uint averageTime = AccumulateTime( timePassed );
    if( timePassed > videoSpd ) {
      uint dim = timePassed - videoSpd;
      NextFrameTime = Timer::GetTime() + dim + 2;
    }

    // Check the average frame time
    // and choose the smoothness quality
    if( timer[1].Await( 500 ) ) {
      if( averageTime > videoSpd * 1.5 ) {
        if( pixelSize <= pixelSizeLow ) {
          pixelSize++;
          BinkFixReport( string::Combine( "quality (-)    lvl: %i / %i    fps: %i / %i", pixelSize, pixelSizeLow, averageTime, videoSpd ) );
          AccumulateTime( 0 );
        }
      }
      else if( pixelSize > InterpolationPixelSizeMin ) {
        if( averageTime < videoSpd * 0.45 ) {
          pixelSize--;
          BinkFixReport( string::Combine( "quality (+)    lvl: %i / %i    fps: %i / %i", pixelSize, pixelSizeLow, averageTime, videoSpd ) );
          AccumulateTime( videoSpd );
        }
      }
    }

    EndFrame();
    return True;
  }


  void zCBinkPlayer::EndFrame( const bool& blit ) {
    PlayWaitNextFrame();
    PlayGotoNextFrame();
    if( blit ) {
      zCRnd_D3D* rnd = (zCRnd_D3D*)zrenderer;
      rnd->Vid_Blit( True, Null, Null );
    }
  }



  HOOK Hook_zCBinkPlayer_PlayHandleEvents PATCH_IF( &oCBinkPlayer::PlayHandleEvents, &oCBinkPlayer::PlayHandleEvents_Union, IsFixBinkEnabled );

#pragma warning(push)
#pragma warning(disable:4244)
  int oCBinkPlayer::PlayHandleEvents_Union() {
    static Timer timer;
    if( timer[1].Await() )
      sysEvent();

    if( disallowInputHandling || !mVideoHandle || zCBinkPlayer::PlayHandleEvents() )
      return FALSE;

    uint16 key = zinput->GetKey( False, False );
    zinput->ProcessInputEvents();

    if( !extendedKeys ) {
      switch( key ) {
        case KEY_SPACE:
          if( zKeyPressed( KEY_LSHIFT ) ) {
            mPaused ? Unpause() : Pause();
            return True;
          }
        case KEY_ESCAPE:
        case KEY_RETURN:
            Stop();
          return True;
      }

      return False;
    }

    switch( key ) {
      case KEY_HOME: {
        BinkGoto( mVideoHandle, 1, 0 );
        return True;
      }

      case KEY_RIGHT: {
        ulong nextFrame = GetBinkFrame() + 30;
        ulong framesNum = GetBinkFramesCount();
        if( nextFrame >= framesNum )
          nextFrame = framesNum - 1;

        BinkGoto( mVideoHandle, nextFrame, 1 );
        return True;
      }

      case KEY_LEFT: {
        ulong nextFrame = GetBinkFrame() - min( GetBinkFrame(), 30 );
        BinkGoto( mVideoHandle, nextFrame, 1 );
        return True;
      }

      case KEY_SPACE: {
        mPaused ? Unpause() : Pause();
        return True;
      }

      case KEY_ESCAPE: {
        Stop();
        return True;
      }

      case KEY_DOWN: {
        mSoundVolume -= 0.1f;
        if( mSoundVolume < 0.0f )
          mSoundVolume = 0.0f;

        BinkSetVolume( mVideoHandle, 0, 65536 * mSoundVolume );
        return True;
      }

      case KEY_UP: {
        mSoundVolume += 0.1f;
        if( mSoundVolume > 1.0f )
          mSoundVolume = 1.0f;

        BinkSetVolume( mVideoHandle, 0, 65536 * mSoundVolume );
        return True;
      }

      case KEY_Q: {
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

    return False;
  }
#pragma warning(pop)
}