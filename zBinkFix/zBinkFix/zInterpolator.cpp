// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  static zBinkInterpolationTable BinkInterpolationTable;
  static Semaphore InterpolationThreadsLimit( BinkGetInterpolationThreadsCount() );


  struct zInterpolator {
    zBinkImage* DstImage;
    const zBinkImage* SrcImage;
    int PixelSize;
    Switch InterpolationEvent;
    Thread InterpolationThread;
    int StartY;
    int EndY;

    zInterpolator() {
      DstImage = Null;
      SrcImage = Null;
      PixelSize = 0;
      InterpolationEvent.TurnOff();
      InterpolationThread.Init( &zInterpolator::InterpolateImagePart );
      InterpolationThread.Detach( this );
    }

    static void InterpolateImagePart( zInterpolator* info ) {
      while( true ) {
        info->InterpolationEvent.WaitOn();
        InterpolationThreadsLimit.Enter();

              zBinkImage& dstImage = *info->DstImage;
        const zBinkImage& srcImage = *info->SrcImage;
        const int pixelSize        = info->PixelSize;
        const int startY           = info->StartY;
        const int endY             = info->EndY;
        const int endX             = dstImage.Size.X;

        if( pixelSize == 1 ) {
          for( int dy = startY; dy < endY; dy++ ) {
            for( int dx = 0; dx < endX; dx++ ) {
              int value = BinkInterpolationTable.GetInterpolatedColor( srcImage, dx, dy );
              dstImage( BinkInterpolationTable.GetLinearIndex( dx, dy ) ) = value;
            }
          }
        }
        else {
          for( int dy = startY; dy < endY; dy += pixelSize ) {
            for( int dx = 0; dx < endX; dx += pixelSize ) {

              int value = BinkInterpolationTable.GetInterpolatedColor( srcImage, dx, dy );
              int endQY = min2( dy + pixelSize, endY );
              int endQX = min2( dx + pixelSize, endX );

              for( int qy = dy; qy < endQY; qy++ ) {
                int lineY = BinkInterpolationTable.GetLineStart( qy );
                for( int qx = dx; qx < endQX; qx++ )
                  dstImage( lineY + qx ) = value;
              }
            }
          }
        }

        InterpolationThreadsLimit.Leave();
        info->InterpolationEvent.TurnOff();
      }
    }
  };


	void CopyImage( zBinkImage& dst, const zBinkImage& src ) {
    for( int dy = 0; dy < dst.Size.Y; dy++ ) {
      int dlny = dy * dst.Size.Pitch4;
      int slny = dy * src.Size.Pitch4;
      for( int dx = 0; dx < dst.Size.X; dx++ )
        dst( dlny + dx ) = src( slny + dx );
    }
  }


  void ResizeImageLinear( zBinkImage& dst, const zBinkImage& src ) {
    for( int dy = 0; dy < dst.Size.Y; dy++ ) {
      int sy   = src.Size.Y * dy / dst.Size.Y;
      int dlny = dy * dst.Size.Pitch4;
      int slny = sy * src.Size.Pitch4;

      for( int dx = 0; dx < dst.Size.X; dx++ ) {
        int sx = src.Size.X * dx / dst.Size.X;
        dst( dlny + dx ) = src( slny + sx );
      }
    }
  }


  void ResizeImage( zBinkImage& dst, const zBinkImage& src, const int& pixelSize ) {
    if( dst.Size == src.Size ) {
      CopyImage( dst, src );
      return;
    }

    if( pixelSize <= 0 || dst.Size < src.Size ) {
      ResizeImageLinear( dst, src );
      return;
    }

    BinkInterpolationTable.Update( dst.Size, src.Size );
    static zInterpolator* interpolation = new zInterpolator[INTERPOLATION_INFO_DIM];
    int yStep = dst.Size.Y / INTERPOLATION_INFO_DIM;
    int yStart = 0;
    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ ) {
      interpolation[i].DstImage  = &dst;
      interpolation[i].SrcImage  = &src;
      interpolation[i].PixelSize = pixelSize;
      interpolation[i].StartY    = yStart;
      interpolation[i].EndY      = min( yStart + yStep, dst.Size.Y );
      interpolation[i].InterpolationEvent.TurnOn();
      yStart += yStep;

      // if( i == 0 || i == INTERPOLATION_INFO_DIM - 1 )
      //   interpolation[i].PixelSize += 1;
      // if( i == 1 || i == INTERPOLATION_INFO_DIM - 2 )
      //   interpolation[i].PixelSize += 1;
      // if( i == 2 || i == INTERPOLATION_INFO_DIM - 3 )
      //   interpolation[i].PixelSize += 1;
    }

    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ )
      interpolation[i].InterpolationEvent.WaitOff();
  }
}