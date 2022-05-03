// Supported with union (c) 2020 Union team

// User API for zCBinkPlayer
// Add your methods here

int zCBinkPlayer::BeginFrame();
int zCBinkPlayer::PlayFrame_Union();
void zCBinkPlayer::EndFrame();
void DrawFrameOnView( zCView* view );
void GetBinkSize( int& x, int& y );
void CorrectViewAspectRatio( zCView* view );