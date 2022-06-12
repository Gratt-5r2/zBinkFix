// Supported with union (c) 2020 Union team

// User API for zCBinkPlayer
// Add your methods here

int zCBinkPlayer::BeginFrame();
int zCBinkPlayer::PlayFrame_Union();
int zCBinkPlayer::PlayFrameBigScreen();
void zCBinkPlayer::EndFrame( const bool& blit = true );
int zCBinkPlayer::CloseVideo_Union();
void DrawFrameOnView( zCView* view );
void GetBinkSize( int& x, int& y );
ulong GetBinkFrameRate();
ulong GetBinkFrame();
void SetBinkFrame( const ulong& frame );
ulong GetBinkFramesCount();
void CorrectViewAspectRatio( zCView* view );
int BlitFrame( const bool& idle = false );
int OpenSurface( RECT& rect, DDSURFACEDESC2& ddsd );
int CloseSurface();
int CloseSurface( RECT& rect );
int OpenVideo_Union( zSTRING );
ulong GetBinkFrameTime();
void ClearScreen();
RECT* GetBinkRects();
bool GetChangedRegion( RECT& region );