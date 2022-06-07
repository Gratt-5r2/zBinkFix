// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
  zBinkImage::zBinkImage( int* image, const zImageSize& size ) {
    ImageBinary = image;
    Size = size;
  }


  int& zBinkImage::operator()( const int& x, const int& y ) {
    return ImageBinary[y * Size.Pitch4 + x];
  }


  int zBinkImage::operator()( const int& x, const int& y ) const {
    return ImageBinary[y * Size.Pitch4 + x];
  }


  int& zBinkImage::operator()( const int& index ) {
    return ImageBinary[index];
  }


  int zBinkImage::operator()( const int& index ) const {
    return ImageBinary[index];
  }
}