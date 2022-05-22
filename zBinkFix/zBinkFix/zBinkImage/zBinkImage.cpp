// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
#define INTERPOLATION_INFO_DIM 16

  int BinkGetInterpolationThreadsCount() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo( &sysinfo );
    int threadsCount = sysinfo.dwNumberOfProcessors / 2;
    Union.GetSysPackOption().Read( threadsCount, "DEBUG", "FixBink_InterpCpuCount", max( threadsCount, 2 ) );
    return min( threadsCount, INTERPOLATION_INFO_DIM );
  }


  int BinkGetInterpolationPixelSize() {
    int pixelSize = 2;
    Union.GetSysPackOption().Read( pixelSize, "DEBUG", "FixBink_InterpPixelSize", pixelSize );
    return pixelSize;
  }


  


  zBinkInterpolationTable::zBinkInterpolationTable() {
    Table = Null;
  }


  inline float Divide( const float& a, const float& b ) {
    return b == 0.0f ? 0.0f : a / b;
  }


  inline float GetDstPart1D( const int& dstWidth, const int& srcWidth, const int& srcPos ) {
    return (float)dstWidth * (float)srcPos / (float)srcWidth;
  }


  void zBinkInterpolationTable::Update( const zImageSize& bigSize, const zImageSize& smallSize ) {
    if( BigSize == bigSize && SmallSize == smallSize )
      return;

    if( Table != Null )
      delete[] Table;

    BigSize = bigSize;
    SmallSize = smallSize;
    Table = new zInterpolationPoint[BigSize.X * BigSize.Y];

    for( int dy = 0; dy < bigSize.Y; dy++ ) {
      float ySmallPos = GetDstPart1D( smallSize.Y, bigSize.Y, dy );
      for( int dx = 0; dx < bigSize.X; dx++ ) {
        float xSmallPos = GetDstPart1D( smallSize.X, bigSize.X, dx );

        zInterpolationPoint& point = Table[lindex( dx, dy, bigSize.X )];
        float xIdLow  = floor( xSmallPos );
        float xIdHigh = ceil ( xSmallPos );
        float yIdLow  = floor( ySmallPos );
        float yIdHigh = ceil ( ySmallPos );

        if( xIdHigh >= smallSize.X ) xIdHigh = xIdLow;
        if( yIdHigh >= smallSize.Y ) yIdHigh = yIdLow;

        point.PixelWeight[0] = Divide( xIdHigh - xSmallPos, xIdHigh - xIdLow );
        point.PixelWeight[1] = 1.0f - point.PixelWeight[0];
        point.PixelWeight[2] = Divide( yIdHigh - ySmallPos, yIdHigh - yIdLow );
        point.PixelWeight[3] = 1.0f - point.PixelWeight[2];

        point.PixelIndex[0] = lindex( xIdLow,  yIdLow,  smallSize.X );
        point.PixelIndex[1] = lindex( xIdHigh, yIdLow,  smallSize.X );
        point.PixelIndex[2] = lindex( xIdLow , yIdHigh, smallSize.X );
        point.PixelIndex[3] = lindex( xIdHigh, yIdHigh, smallSize.X );
      }
    }
  }


  int zBinkInterpolationTable::GetInterpolatedColor( const zBinkImage& smallImage, const int& xBig, const int& yBig ) {
    zBinkImage::zBinkPoint outPoint;
    zInterpolationPoint& interpPoint = Table[lindex( xBig, yBig, BigSize.X )];
    for( int c = 0; c < 3; c++ ) {
      int quadColor[] = {
        smallImage.ImagePoints[interpPoint.PixelIndex[0]].rgba[c],
        smallImage.ImagePoints[interpPoint.PixelIndex[1]].rgba[c],
        smallImage.ImagePoints[interpPoint.PixelIndex[2]].rgba[c],
        smallImage.ImagePoints[interpPoint.PixelIndex[3]].rgba[c]
      };

      int topColor     = quadColor[0] * interpPoint.PixelWeight[0] + quadColor[1] * interpPoint.PixelWeight[1];
      int bottomColor  = quadColor[2] * interpPoint.PixelWeight[0] + quadColor[3] * interpPoint.PixelWeight[1];
      outPoint.rgba[c] = topColor     * interpPoint.PixelWeight[2] + bottomColor  * interpPoint.PixelWeight[3];
    }

    outPoint.a = 255;
    return outPoint.binary;
  }


  int zBinkInterpolationTable::GetLinearColor( const zBinkImage& smallImage, const int& xBig, const int& yBig ) {
    zInterpolationPoint& interpPoint = Table[lindex( xBig, yBig, BigSize.X )];
    return smallImage.ImagePoints[interpPoint.PixelIndex[0]].binary;
  }


  zBinkInterpolationTable::~zBinkInterpolationTable() {
    if( Table != Null )
      delete[] Table;
  }




  zBinkImage::zBinkImage( int* image, const zImageSize& size ) {
    ImageBinary = image;
    Size = size;
  }


  int& zBinkImage::operator()( const int& x, const int& y ) {
    return ImageBinary[y * Size.Pitch32 + x];
  }


  int zBinkImage::operator()( const int& x, const int& y ) const {
    return ImageBinary[y * Size.Pitch32 + x];
  }


  static zBinkInterpolationTable BinkInterpolationTable;
  static Semaphore InterpolationThreadsLimit( BinkGetInterpolationThreadsCount() );


  struct zInterpolationInfo {
    zBinkImage* DstImage;
    const zBinkImage* SrcImage;
    int PixelSize;
    Switch InterpolationEvent;
    Thread InterpolationThread;
    int StartY;
    int EndY;

    zInterpolationInfo() {
      DstImage = Null;
      SrcImage = Null;
      PixelSize = 0;
      InterpolationEvent.TurnOff();
      InterpolationThread.Init( &zInterpolationInfo::InterpolateImagePart );
      InterpolationThread.Detach( this );
    }

    static void InterpolateImagePart( zInterpolationInfo* info ) {
      while( true ) {
        info->InterpolationEvent.WaitOn();
        InterpolationThreadsLimit.Enter();

              zBinkImage& dstImage = *info->DstImage;
        const zBinkImage& srcImage = *info->SrcImage;
        const int PixelSize        = info->PixelSize;
        const int StartY           = info->StartY;
        const int EndY             = info->EndY;

        if( PixelSize == 0 ) {
          for( uint dy = StartY; dy < EndY; dy++ ) {
            for( uint dx = 0; dx < dstImage.Size.X; dx++ ) {
              int value = BinkInterpolationTable.GetInterpolatedColor( srcImage, dx, dy );
              dstImage( dx, dy ) = value;
            }
          }
        }
        else {
          int xI = 0;
          int yI = 0;

          int yIterator = 0;
          for( int dy = StartY; dy < EndY; dy++ ) {

            bool yDoInterp = false;
            if( --yIterator <= 0 ) {
              yI = dy;
              yIterator = PixelSize;
              yDoInterp = true;
            }

            int xIterator = 0;
            for( int dx = 0; dx < dstImage.Size.X; dx++ ) {
              bool xDoInterp = false;
              if( --xIterator <= 0 ) {
                xI = dx;
                xIterator = PixelSize;
                xDoInterp = true;
              }

              if( yDoInterp && xDoInterp ) {
                int value = BinkInterpolationTable.GetInterpolatedColor( srcImage, dx, dy );
                dstImage( dx, dy ) = value;
              }
              else
                dstImage( dx, dy ) = dstImage( xI, yI );
            }
          }
          
        }

        InterpolationThreadsLimit.Leave();
        info->InterpolationEvent.TurnOff();
      }
    }
  };


  void ResizeImage( zBinkImage& dst, const zBinkImage& src ) {
    for( uint dy = 0; dy < dst.Size.Y; dy++ ) {
      int sy = src.Size.Y * dy / dst.Size.Y;
      for( uint dx = 0; dx < dst.Size.X; dx++ ) {
        int sx = src.Size.X * dx / dst.Size.X;
        dst( dx, dy ) = src( sx, sy );
      }
    }
  }


  void InterpolateImage( zBinkImage& dst, const zBinkImage& src, const int& pixelSize ) {
    if( pixelSize <= 0 || dst.Size < src.Size ) {
      ResizeImage( dst, src );
      return;
    }

    BinkInterpolationTable.Update( dst.Size, src.Size );
    static zInterpolationInfo* interpolation = new zInterpolationInfo[INTERPOLATION_INFO_DIM];
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
    }

    for( int i = 0; i < INTERPOLATION_INFO_DIM; i++ )
      interpolation[i].InterpolationEvent.WaitOff();
  }
}