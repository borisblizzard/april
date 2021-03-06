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
/// Defines a UWP window.

#ifdef _UWP_WINDOW
#ifndef APRIL_UWP_WINDOW_H
#define APRIL_UWP_WINDOW_H

#include <gtypes/Matrix4.h>
#include <hltypes/hplatform.h>
#include <hltypes/hstring.h>

#include "aprilExport.h"
#include "Color.h"
#include "Timer.h"
#include "Window.h"
#include "UWP.h"

#define UWP_CURSOR_MAPPINGS "cursor_mappings"

namespace april
{
	class Texture;
	ref class UWP_App;
	
	class UWP_Window : public Window
	{
	public:
		friend ref class UWP_App;

		UWP_Window();
		~UWP_Window();
		
		hstr getParam(chstr param);
		void setParam(chstr param, chstr value);
		
		void setTitle(chstr title);
		HL_DEFINE_GET(int, width, Width);
		HL_DEFINE_GET(int, height, Height);
		HL_DEFINE_GET(void*, backendId, BackendId);

		bool update(float timeDelta);

		void checkEvents();
		hstr findCursorFile(chstr filename) const;
		
		void showVirtualKeyboard();
		void hideVirtualKeyboard();
		void changeSize(float w, float h); // required override instead of normal size changing
		
	protected:
		int width;
		int height;
		hstr fpsTitle;
		bool backButtonSystemHandling;
		hmap<hstr, int> cursorMappings;
		void* backendId;

		void _systemCreate(int width, int height, bool fullscreen, chstr title, Window::Options options);
		
		Cursor* _createCursor(bool fromResource);
		void _refreshCursor();

		void _systemSetResolution(int width, int height, bool fullscreen);
		
	};
	
}
#endif
#endif
