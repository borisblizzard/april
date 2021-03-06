/// @file
/// @version 5.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines a MacOSX window using Apple Cocoa API.

#ifndef APRIL_MAC_WINDOW_H
#define APRIL_MAC_WINDOW_H

#include <hltypes/hmutex.h>
#include "Window.h"
#include "Platform.h"

namespace april
{
    class QueuedEvent;
    
	class Mac_Window : public Window
	{
	public:
		Mac_Window();
		~Mac_Window();

		HL_DEFINE_GET(int, width, Width);
		HL_DEFINE_GET(int, height, Height);
		void* getBackendId() const;
		
		void setTitle(chstr title);
		hstr getParam(chstr param);
		void setParam(chstr param, chstr value);
		
		bool isCursorVisible() const;
		void setCursor(Cursor* value);
		void setCursorVisible(bool visible);

		void checkEvents();
		bool update(float timeDelta);

		void setSystemWindowSize(int width, int height);
		void setFullscreenFlag(bool value);

		void OnAppGainedFocus();
		void OnAppLostFocus();
		
		void onFocusChanged(bool value);
		
		bool shouldIgnoreUpdate();
		void setIgnoreUpdateFlag(bool value);
        
		bool displayLinkIgnoreSystemRedraw;
		bool retainLoadingOverlay;
		bool fastHideLoadingOverlay;
		bool ignoreUpdate;
		hmutex ignoreUpdateMutex;
		bool splashScreenFadeout;
		hstr splashScreenDelay;
		bool disableCursorCheck;
		bool fpsCounter;
		hstr fpsTitle;

		float scalingFactor;
        hmutex renderThreadSyncMutex;
		
	protected:
		int width;
		int height;
		
		Cursor* _createCursor(bool fromResource);
		void _systemCreate(int width, int height, bool fullscreen, chstr title, Window::Options options);
		void _systemDestroy();
		
		void _systemSetResolution(int width, int height, bool fullscreen);
		
		void _presentFrame(bool systemEnabled);
		
	};
	
    bool isUsingCVDisplayLink();
}

bool isPreLion();
bool isLionOrNewer();

#endif
