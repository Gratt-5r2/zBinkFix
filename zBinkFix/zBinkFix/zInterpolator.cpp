// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  static int InterpolationPixelSizeLow     = 0;
  static int InterpolationPixelSize        = BinkGetInterpolationPixelSize();
  static int InterpolationPixelSizeMin     = BinkGetInterpolationPixelSizeMin();
  static int InterpolationPixelSizeCurrent = InterpolationPixelSize;

  enum zScalingMethod {
    zScaling_Copy,
    zScaling_Linear,
    zScaling_Interp
  };

  static zBinkInterpolationTable BinkInterpolationTable;
  static Semaphore ImageScalingTasksLimit( BinkGetInterpolationThreadsCount() );


  struct zImageScalingTask {
    zScalingMethod Method;
    zBinkImage* DstImage;
    const zBinkImage* SrcImage;
    int PixelSize;
    Switch InterpolationEvent;
    Thread InterpolationThread;
    int StartX;
    int StartY;
    int EndX;
    int EndY;

    zImageScalingTask() {
      DstImage = Null;
      SrcImage = Null;
      PixelSize = 0;
      InterpolationEvent.TurnOff();
      InterpolationThread.Init( &zImageScalingTask::ScaleImagePartAsync );
      InterpolationThread.Detach( this );
    }


    /*
     * Smothnened scaling algorithm
     */
    static void ScaleImagePart_Interp( zImageScalingTask* info ) {
            zBinkImage& dstImage = *info->DstImage;
      const zBinkImage& srcImage = *info->SrcImage;
      const int pixelSize        = info->PixelSize;
      const int yStart           = info->StartY;
      const int yEnd             = info->EndY;
      const int xEnd             = dstImage.Size.X;

      // Calculate and place interpolated
      // color data to the dest image by XY
      if( pixelSize == 1 ) {
        for( int dy = yStart; dy < yEnd; dy++ ) {
          for( int dx = 0; dx < xEnd; dx++ ) {
            int value = BinkInterpolationTable.GetInterpolatedColor( srcImage, dx, dy );
            dstImage( BinkInterpolationTable.GetLinearIndex( dx, dy ) ) = value;
          }
        }
      }
      // Do this same, but skip several pixels if
      // the interpolation quality is not the best.
      else {
        for( int dy = yStart; dy < yEnd; dy += pixelSize ) {
          for( int dx = 0; dx < xEnd; dx += pixelSize ) {

            int value = BinkInterpolationTable.GetInterpolatedColor( srcImage, dx, dy );
            int endQY = min2( dy + pixelSize, yEnd );
            int endQX = min2( dx + pixelSize, xEnd );

            // This is a quad region the size of the interpolation
            // pixel size. Used to accelerate calculations 
            for( int qy = dy; qy < endQY; qy++ ) {
              int lineY = BinkInterpolationTable.GetLineStart( qy );
              for( int qx = dx; qx < endQX; qx++ )
                dstImage( lineY + qx ) = value;
            }
          }
        }
      }
    }


    /*
     * Linear copying algorithm
     */
    static void ScaleImagePart_Copy( zImageScalingTask* info ) {
            zBinkImage& dstImage = *info->DstImage;
      const zBinkImage& srcImage = *info->SrcImage;
      const int ySstart           = info->StartY;
      const int yEnd             = info->EndY;
      const int xEnd             = dstImage.Size.X;

      // Calculate source image indexes
      // and place values to the dest image
      for( int dy = ySstart; dy < yEnd; dy++ ) {
        int dlny = dy * dstImage.Size.Pitch4;
        int slny = dy * srcImage.Size.Pitch4;
        for( int dx = 0; dx < xEnd; dx++ )
          dstImage( dlny + dx ) = srcImage( slny + dx );
      }
    }


    /*
     * Linear scaling algorithm
     */
    static void ScaleImagePart_Linear( zImageScalingTask* info ) {
            zBinkImage& dstImage = *info->DstImage;
      const zBinkImage& srcImage = *info->SrcImage;
      const int yStart           = info->StartY;
      const int yEnd             = info->EndY;
      const int xEnd             = dstImage.Size.X;

      // Calculate source image indexes
      // and place values to the dest image
      for( int dy = yStart; dy < yEnd; dy++ ) {
        int sy   = srcImage.Size.Y * dy / dstImage.Size.Y;
        int dlny = dy * dstImage.Size.Pitch4;
        int slny = sy * srcImage.Size.Pitch4;
    
        for( int dx = 0; dx < xEnd; dx++ ) {
          int sx = srcImage.Size.X * dx / dstImage.Size.X;
          dstImage( dlny + dx ) = srcImage( slny + sx );
        }
      }
    }


    static void ScaleImagePartAsync( zImageScalingTask* scaler ) {
      while( true ) {
        // Wait command to start
        scaler->InterpolationEvent.WaitOn();
        ImageScalingTasksLimit.Enter();

             if( scaler->Method == zScalingMethod::zScaling_Interp ) ScaleImagePart_Interp( scaler );
        else if( scaler->Method == zScalingMethod::zScaling_Copy   ) ScaleImagePart_Copy  ( scaler );
        else if( scaler->Method == zScalingMethod::zScaling_Linear ) ScaleImagePart_Linear( scaler );
        
        // Send command to stop
        ImageScalingTasksLimit.Leave();
        scaler->InterpolationEvent.TurnOff();
      }
    }
  };


  static zImageScalingTask* ImageScalingTasks = CHECK_THIS_ENGINE ? new zImageScalingTask[INTERPOLATION_INFO_DIM] : Null;

#pragma warning(push)
#pragma warning(disable:4244)
  void ResizeImageRegion( zBinkImage& dst, const zBinkImage& src, const BINKRECT& srcRect, const int& pixelSize ) {
    if( srcRect.left >= srcRect.right || srcRect.top >= srcRect.bottom )
      return;

    int pixelSizeLow = InterpolationPixelSizeLow;
    zScalingMethod method;

         if( dst.Size == src.Size )     method = zScalingMethod::zScaling_Copy;   // Interpolation not needed
    else if( dst.Size <  src.Size )     method = zScalingMethod::zScaling_Linear; // Downscale image
    else if( pixelSize > pixelSizeLow ) method = zScalingMethod::zScaling_Linear; // Minimal upscale quality
    else if( pixelSize <= 0 )           method = zScalingMethod::zScaling_Linear; // Interpolation disabled
    else                                method = zScalingMethod::zScaling_Interp; // Interpolatoin enabled

    ulong dstRegX      = MakeScaledIndex( dst.Size.X, src.Size.X, srcRect.left );
    ulong dstRegY      = MakeScaledIndex( dst.Size.X, src.Size.X, srcRect.top );
    ulong dstRegWidth  = MakeScaledIndex( dst.Size.Y, src.Size.Y, srcRect.right );
    ulong dstRegHeight = MakeScaledIndex( dst.Size.Y, src.Size.Y, srcRect.bottom );

    int yStart = dstRegY;
    int yEnd   = dstRegY + dstRegHeight;
    int yStep  = max2( dstRegHeight / INTERPOLATION_INFO_DIM, 1 );

    // Create context of the editable image region. In this algirithm
    // used 16 threads. Count of the active threads defined 
    // by Plugin automatically or custom in the SystemPack.ini
    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ ) {
      auto& task = ImageScalingTasks[i];

      task.Method    = method;
      task.DstImage  = &dst;
      task.SrcImage  = &src;
      task.PixelSize = pixelSize;
      task.StartX    = dstRegX;
      task.StartY    = yStart;
      task.EndX      = dstRegX + dstRegWidth;
      task.EndY      = yStart + yStep;
      task.InterpolationEvent.TurnOn();
      yStart += yStep;

      // Correct the last thread by image bottom edge
      if( yStart >= yEnd || i == INTERPOLATION_INFO_DIM - 1 ) {
        task.EndY = yEnd;
        break;
      }
    }

    // Wait until all threads finish working
    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ )
      ImageScalingTasks[i].InterpolationEvent.WaitOff();
  }

  
  void ResizeImage( zBinkImage& dst, const zBinkImage& src, const int& pixelSize ) {
    RECT region;
    region.left   = 0;
    region.top    = 0;
    region.right  = src.Size.X;
    region.bottom = src.Size.Y;
    ResizeImageRegion( dst, src, region, pixelSize );
  }
#pragma warning(pop)
}