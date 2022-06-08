// Supported with union (c) 2020 Union team

// User API for zCBinkPlayer
// Add your methods here

int zCBinkPlayer::BeginFrame();
int zCBinkPlayer::PlayFrame_Union();
void zCBinkPlayer::EndFrame();
void DrawFrameOnView( zCView* view );
void GetBinkSize( int& x, int& y );
ulong GetBinkFrameRate();
ulong GetBinkFrame();
void SetBinkFrame( const ulong& frame );
ulong GetBinkFramesCount();
void CorrectViewAspectRatio( zCView* view );
int BlitFrame( const bool& idle = false );
int OpenSurface( zTRndSurfaceDesc& srf );
int OpenSurface( RECT& rect, DDSURFACEDESC2& ddsd );
int CloseSurface();
int CloseSurface( RECT& rect );
int OpenVideo_Union( zSTRING );