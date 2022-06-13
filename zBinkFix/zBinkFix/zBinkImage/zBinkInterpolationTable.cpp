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


  inline float MakeScaledIndex( const int& dstWidth, const int& srcWidth, const int& srcPos ) {
    return (float)dstWidth * (float)srcPos / (float)srcWidth;
  }


  zBinkInterpolationTable::zBinkInterpolationTable() {
    Table = Null;
    LinesImageIndexes = Null;
    LinesTableIndexes = Null;
  }


#pragma warning(push)
#pragma warning(disable:4244)
  void zBinkInterpolationTable::Update( const zImageSize& bigSize, const zImageSize& smallSize ) {
    if( BigSize == bigSize && SmallSize == smallSize ) {

      // Update pitch of the image. DirectX7 surface
      // may have a double buffering and that buffers
      // may have different widths
      if( BigSize.Pitch4 != bigSize.Pitch4 ) {
        BigSize.Pitch4 = bigSize.Pitch4;
        for( int dy = 0; dy < bigSize.Y; dy++ )
          LinesImageIndexes[dy] = dy * bigSize.Pitch4;
      }

      return;
    }

    Clear();

    // Create a special table of the information
    // about the image transformation
    BigSize = bigSize;
    SmallSize = smallSize;
    Table = new zInterpolationPoint[BigSize.X * BigSize.Y];
    LinesImageIndexes = new uint[bigSize.Y];
    LinesTableIndexes = new uint[bigSize.Y];

    for( int dy = 0; dy < bigSize.Y; dy++ ) {
      // Create indexes of rows starts to
      // quick access to rows by y coordinate
      LinesImageIndexes[dy] = dy * bigSize.Pitch4;
      LinesTableIndexes[dy] = dy * bigSize.X;

      // Projection X and Y coordinates
      // of the surface to the source image
      float ySmallPos = MakeScaledIndex( smallSize.Y, bigSize.Y, dy );
      for( int dx = 0; dx < bigSize.X; dx++ ) {
        float xSmallPos = MakeScaledIndex( smallSize.X, bigSize.X, dx );

        // Define 4 nearests pixels by XY
        zInterpolationPoint& point = Table[xy2i( dx, dy, bigSize.X )];
        float xIdLow  = floor( xSmallPos );
        float xIdHigh = ceil ( xSmallPos );
        float yIdLow  = floor( ySmallPos );
        float yIdHigh = ceil ( ySmallPos );

        if( xIdHigh >= smallSize.X ) xIdHigh = xIdLow;
        if( yIdHigh >= smallSize.Y ) yIdHigh = yIdLow;

        // Define pixels priorities by distance to XY
        point.PixelWeight[0] = SafeDivide( xIdHigh - xSmallPos, xIdHigh - xIdLow );
        point.PixelWeight[1] = 1.0f - point.PixelWeight[0];
        point.PixelWeight[2] = SafeDivide( yIdHigh - ySmallPos, yIdHigh - yIdLow );
        point.PixelWeight[3] = 1.0f - point.PixelWeight[2];

        // Set information about 4 nearest pixels
        // in the source imabe by destination XY
        point.PixelIndex[0] = xy2i( xIdLow,  yIdLow,  smallSize.X );
        point.PixelIndex[1] = xy2i( xIdHigh, yIdLow,  smallSize.X );
        point.PixelIndex[2] = xy2i( xIdLow , yIdHigh, smallSize.X );
        point.PixelIndex[3] = xy2i( xIdHigh, yIdHigh, smallSize.X );

        // Create an integer divisors to fast divide pixel
        // datas to the weight. First value is a special
        // constant to get a cleaner picture on the top colors
        point.Divisor[0] = ceil( 253.9f * point.PixelWeight[0] );
        point.Divisor[1] = ceil( 253.9f * point.PixelWeight[1] );
        point.Divisor[2] = ceil( 253.9f * point.PixelWeight[2] );
        point.Divisor[3] = ceil( 253.9f * point.PixelWeight[3] );
      }
    }
  }
#pragma warning(pop)


  int zBinkInterpolationTable::GetInterpolatedColor( const zBinkImage& smallImage, const int& xBig, const int& yBig ) {
    // Get the current pixel information
    zInterpolationPoint& interpPoint = Table[xy2i( xBig, yBig, BigSize.X )];

    // Get first diagonal pixels. Don't
    // interpolate them if pixels if equal.
    zBinkPoint color1 = smallImage.ImagePoints[interpPoint.PixelIndex[0]];
    zBinkPoint color4 = smallImage.ImagePoints[interpPoint.PixelIndex[3]];
    if( color1.binary == color4.binary )
      return color1.binary;

    // Get second diagonal
    zBinkPoint color2 = smallImage.ImagePoints[interpPoint.PixelIndex[1]];
    zBinkPoint color3 = smallImage.ImagePoints[interpPoint.PixelIndex[2]];

    // Get the best color of the top edge
    PointDim( color1, interpPoint.Divisor[0] );
    PointDim( color2, interpPoint.Divisor[1] );
    PointAdd( color1, color2 );
    
    // Get the best color of the bottom edge
    PointDim( color3, interpPoint.Divisor[0] );
    PointDim( color4, interpPoint.Divisor[1] );
    PointAdd( color3, color4 );
    
    // Get the best color of the vertical
    // edge by best horizontals edges
    PointDim( color1, interpPoint.Divisor[2] );
    PointDim( color3, interpPoint.Divisor[3] );
    PointAdd( color1, color3 );

    // Return interpolated data
    return color1.binary;
  }


  int zBinkInterpolationTable::GetLinearColor( const zBinkImage& smallImage, const int& xBig, const int& yBig ) {
    //zInterpolationPoint& interpPoint = Table[LinesTableIndexes[yBig] + xBig];
    zInterpolationPoint& interpPoint = Table[xy2i( xBig, yBig, BigSize.X )];
    return smallImage.ImagePoints[interpPoint.PixelIndex[0]].binary;
  }


  void zBinkInterpolationTable::Clear() {
    BigSize.Reset();
    SmallSize.Reset();
    if( Table ) {
      delete[] Table;
      Table = Null;
    }
    if( LinesImageIndexes ) {
      delete[] LinesImageIndexes;
      LinesImageIndexes = Null;
    }
    if( LinesTableIndexes ) {
      delete[] LinesTableIndexes;
      LinesTableIndexes = Null;
    }
  }


  zBinkInterpolationTable::~zBinkInterpolationTable() {
    Clear();
  }
}