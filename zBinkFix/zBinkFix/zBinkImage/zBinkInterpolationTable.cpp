// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  inline void PointDim( zBinkPoint& col, const uint16& value ) {
    const uint16 dim = 255;
    col.r = (byte)((uint16)col.r * value / dim);
    col.g = (byte)((uint16)col.g * value / dim);
    col.b = (byte)((uint16)col.b * value / dim);
  }


  inline void PointAdd( zBinkPoint& pointA, zBinkPoint& pointB ) {
    pointA.r += pointB.r;
    pointA.g += pointB.g;
    pointA.b += pointB.b;
  }


  inline float MakeCorrectSingleIndex( const int& dstWidth, const int& srcWidth, const int& srcPos ) {
    return (float)dstWidth * (float)srcPos / (float)srcWidth;
  }


  zBinkInterpolationTable::zBinkInterpolationTable() {
    Table = Null;
    LinesIndexes = Null;
  }


#pragma warning(push)
#pragma warning(disable:4244)
  void zBinkInterpolationTable::Update( const zImageSize& bigSize, const zImageSize& smallSize ) {
    if( BigSize == bigSize && SmallSize == smallSize )
      return;

    if( Table != Null ) {
      delete[] Table;
      delete[] LinesIndexes;
    }

    BigSize = bigSize;
    SmallSize = smallSize;
    Table = new zInterpolationPoint[BigSize.X * BigSize.Y];
    LinesIndexes = new uint[bigSize.Y];

    for( int dy = 0; dy < bigSize.Y; dy++ ) {
      LinesIndexes[dy] = dy * bigSize.Pitch4;

      float ySmallPos = MakeCorrectSingleIndex( smallSize.Y, bigSize.Y, dy );
      for( int dx = 0; dx < bigSize.X; dx++ ) {
        float xSmallPos = MakeCorrectSingleIndex( smallSize.X, bigSize.X, dx );

        zInterpolationPoint& point = Table[xy2i( dx, dy, bigSize.X )];
        float xIdLow  = floor( xSmallPos );
        float xIdHigh = ceil ( xSmallPos );
        float yIdLow  = floor( ySmallPos );
        float yIdHigh = ceil ( ySmallPos );

        if( xIdHigh >= smallSize.X ) xIdHigh = xIdLow;
        if( yIdHigh >= smallSize.Y ) yIdHigh = yIdLow;

        point.PixelWeight[0] = SafeDivide( xIdHigh - xSmallPos, xIdHigh - xIdLow );
        point.PixelWeight[1] = 1.0f - point.PixelWeight[0];
        point.PixelWeight[2] = SafeDivide( yIdHigh - ySmallPos, yIdHigh - yIdLow );
        point.PixelWeight[3] = 1.0f - point.PixelWeight[2];

        point.PixelIndex[0] = xy2i( xIdLow,  yIdLow,  smallSize.X );
        point.PixelIndex[1] = xy2i( xIdHigh, yIdLow,  smallSize.X );
        point.PixelIndex[2] = xy2i( xIdLow , yIdHigh, smallSize.X );
        point.PixelIndex[3] = xy2i( xIdHigh, yIdHigh, smallSize.X );

        point.Divisor[0] = ceil( 253.9f * point.PixelWeight[0] );
        point.Divisor[1] = ceil( 253.9f * point.PixelWeight[1] );
        point.Divisor[2] = ceil( 253.9f * point.PixelWeight[2] );
        point.Divisor[3] = ceil( 253.9f * point.PixelWeight[3] );
      }
    }
  }
#pragma warning(pop)


  int zBinkInterpolationTable::GetInterpolatedColor( const zBinkImage& smallImage, const int& xBig, const int& yBig ) {
    zInterpolationPoint& interpPoint = Table[xy2i( xBig, yBig, BigSize.X )];

    zBinkPoint color1 = smallImage.ImagePoints[interpPoint.PixelIndex[0]];
    zBinkPoint color2 = smallImage.ImagePoints[interpPoint.PixelIndex[1]];
    zBinkPoint color3 = smallImage.ImagePoints[interpPoint.PixelIndex[2]];
    zBinkPoint color4 = smallImage.ImagePoints[interpPoint.PixelIndex[3]];

    if( color1.binary == color4.binary )
      return color1.binary;

    PointDim( color1, interpPoint.Divisor[0] );
    PointDim( color2, interpPoint.Divisor[1] );
    PointAdd( color1, color2 );
    
    PointDim( color3, interpPoint.Divisor[0] );
    PointDim( color4, interpPoint.Divisor[1] );
    PointAdd( color3, color4 );
    
    PointDim( color1, interpPoint.Divisor[2] );
    PointDim( color3, interpPoint.Divisor[3] );
    PointAdd( color1, color3 );
    return color1.binary;
  }


  int zBinkInterpolationTable::GetLinearColor( const zBinkImage& smallImage, const int& xBig, const int& yBig ) {
    zInterpolationPoint& interpPoint = Table[xy2i( xBig, yBig, BigSize.X )];
    return smallImage.ImagePoints[interpPoint.PixelIndex[0]].binary;
  }


  zBinkInterpolationTable::~zBinkInterpolationTable() {
    if( Table != Null )
      delete[] Table;
  }
}