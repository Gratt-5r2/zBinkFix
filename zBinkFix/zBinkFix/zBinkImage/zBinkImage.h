// Supported with union (c) 2020 Union team
// Union HEADER file

namespace GOTHIC_ENGINE {
	struct zBinkImage {
		struct zBinkPoint {
			union {
				struct {
					byte r, g, b, a;
				};
				byte rgba[4];
				int binary;
			};
		};

		union {
			int* ImageBinary;
			zBinkPoint* ImagePoints;
		};

		zImageSize Size;

		zBinkImage( int* image, const zImageSize& size );
		int& operator()( const int& x, const int& y );
		int operator()( const int& x, const int& y ) const;
	};



	struct zBinkInterpolationTable {
		struct zInterpolationPoint {
			int PixelIndex[4];
			float PixelWeight[4];
		};

		zInterpolationPoint* Table;
		zImageSize BigSize;
		zImageSize SmallSize;

		zBinkInterpolationTable();
		void Update( const zImageSize& bigSize, const zImageSize& smallSize );
		int GetInterpolatedColor( const zBinkImage& smallImage, const int& xBig, const int& yBig );
		int GetLinearColor( const zBinkImage& smallImage, const int& xBig, const int& yBig );
		~zBinkInterpolationTable();
	};



	void InterpolateImage( zBinkImage& dst, const zBinkImage& src, const int& pixelSize = 0 );
}