// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
#define INTERPOLATION_INFO_DIM 16

  struct zBinkImage {
    struct zBinkPoint {
      union {
        struct {
          byte r, g, b, a;
        };
        byte rgba[4];
        int binary;
      };

      zBinkPoint() { }
      zBinkPoint( const zBinkPoint& point ) : binary( point.binary ) { }
      zBinkPoint( const byte& _r, const byte& _g, const byte& _b ) : r( _r ), g( _g ), b( _b ) {}
      bool operator == ( const zBinkPoint& point ) {
        return 
          r == point.r &&
          g == point.g && 
          b == point.b; 
      }
    };

    union {
      int* ImageBinary;
      zBinkPoint* ImagePoints;
    };

    zImageSize Size;

    zBinkImage( int* image, const zImageSize& size );
    int& operator()( const int& x, const int& y );
    int operator()( const int& x, const int& y ) const;
    int& operator()( const int& index );
    int operator()( const int& index ) const;
  };


  struct zBinkInterpolationTable {
    struct zInterpolationPoint {
      int PixelIndex[4];
      float PixelWeight[4];
      uint16 Divisor[4];
    };

    zInterpolationPoint* Table;
    uint* LinesIndexes;
    zImageSize BigSize;
    zImageSize SmallSize;

    zBinkInterpolationTable();
    void Update( const zImageSize& bigSize, const zImageSize& smallSize );
    int GetInterpolatedColor( const zBinkImage& smallImage, const int& xBig, const int& yBig );
    int GetLinearColor( const zBinkImage& smallImage, const int& xBig, const int& yBig );
    int GetLineStart( const int& y ) { return LinesIndexes[y]; }
    int GetLinearIndex( const int& x, const int& y ) { return LinesIndexes[y] + x; }
    ~zBinkInterpolationTable();
  };

  typedef zBinkImage::zBinkPoint zBinkPoint;

  void ResizeImage( zBinkImage& dst, const zBinkImage& src, const int& pixelSize = 0 );
}