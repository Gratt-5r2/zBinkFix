// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  static int InterpolationPixelSizeLow     = 0;
  static int InterpolationPixelSize        = BinkGetInterpolationPixelSize();
  static int InterpolationPixelSizeMin     = BinkGetInterpolationPixelSizeMin();
  static int InterpolationPixelSizeCurrent = InterpolationPixelSize;

  enum zResizeMethod {
    zResize_Copy,
    zResize_Linear,
    zResize_Interp
  };

  static zBinkInterpolationTable BinkInterpolationTable;
  static Semaphore InterpolationThreadsLimit( BinkGetInterpolationThreadsCount() );


  struct zImageScaler {
    zResizeMethod Method;
    zBinkImage* DstImage;
    const zBinkImage* SrcImage;
    int PixelSize;
    Switch InterpolationEvent;
    Thread InterpolationThread;
    int StartX;
    int StartY;
    int EndX;
    int EndY;

    zImageScaler() {
      DstImage = Null;
      SrcImage = Null;
      PixelSize = 0;
      InterpolationEvent.TurnOff();
      InterpolationThread.Init( &zImageScaler::ResizeImagePartAsync );
      InterpolationThread.Detach( this );
    }


    /*
     * Smothness resize algorithm
     */
    static void ResizeImagePart_Interp( zImageScaler* info ) {
            zBinkImage& dstImage = *info->DstImage;
      const zBinkImage& srcImage = *info->SrcImage;
      const int pixelSize        = info->PixelSize;
      const int startY           = info->StartY;
      const int endY             = info->EndY;
      const int endX             = dstImage.Size.X;

      // Calculate and place interpolated
      // color data to the dest image by XY
      if( pixelSize == 1 ) {
        for( int dy = startY; dy < endY; dy++ ) {
          for( int dx = 0; dx < endX; dx++ ) {
            int value = BinkInterpolationTable.GetInterpolatedColor( srcImage, dx, dy );
            dstImage( BinkInterpolationTable.GetLinearIndex( dx, dy ) ) = value;
          }
        }
      }
      // Do this came, but skip several pixels if the interpolation
      // quality is not the best. In skipped segments will
      // be setted information from the previous pixels.
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
    }


    /*
     * Linear resize algorithm
     */
    static void ResizeImagePart_Copy( zImageScaler* info ) {
            zBinkImage& dstImage = *info->DstImage;
      const zBinkImage& srcImage = *info->SrcImage;
      const int startY           = info->StartY;
      const int endY             = info->EndY;
      const int endX             = dstImage.Size.X;

      // Calculate source image indexes
      // and place values to the dest image
      for( int dy = startY; dy < endY; dy++ ) {
        int dlny = dy * dstImage.Size.Pitch4;
        int slny = dy * srcImage.Size.Pitch4;
        for( int dx = 0; dx < endX; dx++ )
          dstImage( dlny + dx ) = srcImage( slny + dx );
      }
    }


    /*
     * Linear copying algorithm
     */
    static void ResizeImagePart_Linear( zImageScaler* info ) {
            zBinkImage& dstImage = *info->DstImage;
      const zBinkImage& srcImage = *info->SrcImage;
      const int startY           = info->StartY;
      const int endY             = info->EndY;
      const int endX             = dstImage.Size.X;

      // Calculate source image indexes
      // and place values to the dest image
      for( int dy = startY; dy < endY; dy++ ) {
        int sy   = srcImage.Size.Y * dy / dstImage.Size.Y;
        int dlny = dy * dstImage.Size.Pitch4;
        int slny = sy * srcImage.Size.Pitch4;
    
        for( int dx = 0; dx < endX; dx++ ) {
          int sx = srcImage.Size.X * dx / dstImage.Size.X;
          dstImage( dlny + dx ) = srcImage( slny + sx );
        }
      }
    }


    static void ResizeImagePartAsync( zImageScaler* scaler ) {
      while( true ) {
        // Wait command to start
        scaler->InterpolationEvent.WaitOn();
        InterpolationThreadsLimit.Enter();

             if( scaler->Method == zResize_Interp ) ResizeImagePart_Interp( scaler );
        else if( scaler->Method == zResize_Copy   ) ResizeImagePart_Copy  ( scaler );
        else if( scaler->Method == zResize_Linear ) ResizeImagePart_Linear( scaler );
        
        // Send command to stop
        InterpolationThreadsLimit.Leave();
        scaler->InterpolationEvent.TurnOff();
      }
    }
  };


  typedef int(*gpuScaleImageFunc)( int* dstImage, int dstX, int dstY, int dstPitch32, int* srcImage, int srcX, int srcY, int srcPitch32, bool interpolation );
  gpuScaleImageFunc ImportGpuScaleImage() {
    HMODULE module = LoadLibraryAST( "oGraphics.dll" );
    void* proc = GetProcAddress( module, "?gpuScaleImage@@YAHPAHHHH0HHH_N@Z" );
    return (gpuScaleImageFunc)proc;
  }

  /*
    static gpuScaleImageFunc gpuScaleImage = ImportGpuScaleImage();
    if( gpuScaleImage ) {
      gpuScaleImage( dst.ImageBinary, dst.Size.X, dst.Size.Y, dst.Size.Pitch4, src.ImageBinary, src.Size.X, src.Size.Y, src.Size.Pitch4, true );
      return;
    }
  */


  static zImageScaler* interpolation = new zImageScaler[INTERPOLATION_INFO_DIM];
  static int interpolationEnum = 0;

  void ResizeImage( zBinkImage& dst, const zBinkImage& src, const int& pixelSize ) {
    int pixelSizeLow = InterpolationPixelSizeLow;
    zResizeMethod method;

         if( dst.Size == src.Size )     method = zResizeMethod::zResize_Copy;
    else if( pixelSize > pixelSizeLow ) method = zResizeMethod::zResize_Linear;
    else if( dst.Size < src.Size )      method = zResizeMethod::zResize_Linear;
    else if( pixelSize <= 0 )           method = zResizeMethod::zResize_Linear;
    else                                method = zResizeMethod::zResize_Interp;

    int yStep = ceil((double)dst.Size.Y / INTERPOLATION_INFO_DIM);
    int yStart = 0;

    // Create context of the editable image region. In this algirithm
    // used 16 threads. Count of the active threads defined 
    //by Plugin automatically or custom in the SystemPack.ini
    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ ) {
      interpolation[i].Method    = method;
      interpolation[i].DstImage  = &dst;
      interpolation[i].SrcImage  = &src;
      interpolation[i].PixelSize = pixelSize;
      interpolation[i].StartY    = yStart;
      interpolation[i].EndY      = min2( yStart + yStep, dst.Size.Y );
      interpolation[i].InterpolationEvent.TurnOn();
      yStart += yStep;

      // Correct last thread by image bottom edge
      if( i == INTERPOLATION_INFO_DIM - 1 )
        interpolation[i].EndY = dst.Size.Y;
    }

    // Wait until all threads finish working
    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ )
      interpolation[i].InterpolationEvent.WaitOff();
  }


  void ResizeImage( zBinkImage& dst, const zBinkImage& src, const RECT& srcRect, const int& pixelSize ) {
    int pixelSizeLow = InterpolationPixelSizeLow;
    zResizeMethod method;

         if( dst.Size == src.Size )     method = zResizeMethod::zResize_Copy;
    else if( pixelSize > pixelSizeLow ) method = zResizeMethod::zResize_Linear;
    else if( dst.Size < src.Size )      method = zResizeMethod::zResize_Linear;
    else if( pixelSize <= 0 )           method = zResizeMethod::zResize_Linear;
    else                                method = zResizeMethod::zResize_Interp;

    ulong dstRegX      = MakeCorrectSingleIndex( dst.Size.X, src.Size.X, srcRect.left );
    ulong dstRegY      = MakeCorrectSingleIndex( dst.Size.X, src.Size.X, srcRect.top );
    ulong dstRegWidth  = MakeCorrectSingleIndex( dst.Size.Y, src.Size.Y, srcRect.right );
    ulong dstRegHeight = MakeCorrectSingleIndex( dst.Size.Y, src.Size.Y, srcRect.bottom );

    int yStep = ceil((double)(dstRegY + dstRegHeight) / INTERPOLATION_INFO_DIM);
    if( yStep == 0 )
      yStep = 1;

    int yStart = dstRegY;
    int yEnd = dstRegY + dstRegHeight;

    // Create context of the editable image region. In this algirithm
    // used 16 threads. Count of the active threads defined 
    //by Plugin automatically or custom in the SystemPack.ini
    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ ) {
      interpolation[i].Method    = method;
      interpolation[i].DstImage  = &dst;
      interpolation[i].SrcImage  = &src;
      interpolation[i].PixelSize = pixelSize;
      interpolation[i].StartX    = dstRegX;
      interpolation[i].StartY    = yStart;
      interpolation[i].EndX      = dstRegX + dstRegWidth;
      interpolation[i].EndY      = min2( yStart + yStep, yEnd );
      interpolation[i].InterpolationEvent.TurnOn();
      yStart += yStep;

      // Correct last thread by image bottom edge
      if( i == INTERPOLATION_INFO_DIM - 1 )
        interpolation[i].EndY = yEnd;

      if( yStart >= yEnd )
        break;
    }

    // Wait until all threads finish working
    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ )
      interpolation[i].InterpolationEvent.WaitOff();
  }
}