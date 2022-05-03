// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	zCView* CreateFrameView() {
		zCView* view = new zCView();
		screen->InsertItem( view );
		view->SetSize( 8192, 8192 );
		view->backTex = zrenderer->CreateTexture();
		screen->RemoveItem( view );
		return view;
	}


	void zCBinkPlayer::GetBinkSize( int& x, int& y ) {
		ulong* xy = (ulong*)mVideoHandle;
		x = xy[0];
		y = xy[1];
	}


	int zCBinkPlayer::BeginFrame() {
		if( !IsPlaying() )
			return False;

		zCVideoPlayer::PlayFrame();
		PlayHandleEvents();
		BinkDoFrame( mVideoHandle );
		return True;
	}


	void zCBinkPlayer::DrawFrameOnView( zCView* view ) {
		void* texBuffer = Null;
		int w, h, perfomance, pitchX = 0;
		GetBinkSize( w, h );
		perfomance = max3( w, h, 512 );

		zCTexConGeneric* texConvert = (zCTexConGeneric*)zrenderer->CreateTextureConvert();
		zCTextureInfo binkFrameInfo = texConvert->GetTextureInfo();
		binkFrameInfo.texFormat = zRND_TEX_FORMAT_BGRA_8888;
		binkFrameInfo.numMipMap = 1;
		binkFrameInfo.sizeX = w;
		binkFrameInfo.sizeY = h;
		texConvert->SetTextureInfo( binkFrameInfo );

		texConvert->Lock( zTEX_LOCK_WRITE );
		texConvert->GetTextureBuffer( 0, texBuffer, pitchX );
		BinkCopyToBuffer( mVideoHandle, texBuffer, pitchX, binkFrameInfo.sizeY, 0, 0, BINKSURFACE32A | BINKCOPYALL | BINKNOSKIP );
		texConvert->Unlock();

		zCTextureInfo texInfo = texConvert->GetTextureInfo();
		texInfo.texFormat = zCTexture::CalcNextBestTexFormat( zRND_TEX_FORMAT_RGB_565 );
		texInfo.sizeX = perfomance;
		texInfo.sizeY = perfomance / 2;
		zCTextureConvert::CorrectPow2( texInfo.sizeX, texInfo.sizeY );

		texConvert->ConvertTextureFormat( texInfo );
		texConvert->CopyContents( texConvert, view->backTex );
		delete texConvert;
	}


#define HORIZONRAL_ASPECT_RATIO 1

	void zCBinkPlayer::CorrectViewAspectRatio( zCView* view ) {
		int w, h;
		GetBinkSize( w, h );

		float sx = (float)w;
		float sy = (float)h;

		float tx = (float)zrenderer->vid_xdim;
		float ty = (float)zrenderer->vid_ydim;

#if HORIZONRAL_ASPECT_RATIO == 1
		// get ratio full multiplier
		float mx = sx / tx;
		float my = sy / ty;

		// correct at local ratio
		int newx = (int)(8192.0f * (mx / my));
		view->SetSize( newx, 8192 );
		view->SetPos( (8192 - newx) / 2, 0 );
#else
		// get ratio full multiplier
		float mx = sx / tx;
		float my = sy / ty;

		// correct at local ratio
		int newy = (int)(8192.0f * (my / mx));
		view->SetSize( 8192, newy );
		view->SetPos( 0, (8192 - newy) / 2 );
#endif
	}


  HOOK Hook_zCBinkPlayer_PlayFrame PATCH_IF( &zCBinkPlayer::PlayFrame, &zCBinkPlayer::PlayFrame_Union, ReadOption( "DEBUG", "FixBink" ) );

	int zCBinkPlayer::PlayFrame_Union() {
		if( !BeginFrame() )
			return False;

		static zCView* view = CreateFrameView();
		screen->InsertItem( view );
		DrawFrameOnView( view );
		CorrectViewAspectRatio( view );
		screen->Render();
		screen->RemoveItem( view );

		EndFrame();
		return True;
	};


	void zCBinkPlayer::EndFrame() {
		PlayWaitNextFrame();
		PlayGotoNextFrame();
		zrenderer->Vid_Blit( True, Null, Null );
		sysEvent();
	}
}