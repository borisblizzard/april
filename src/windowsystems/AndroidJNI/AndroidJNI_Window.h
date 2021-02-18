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
/// Defines an Android JNI window.

#ifdef _ANDROIDJNI_WINDOW
#ifndef APRIL_ANDROID_JNI_WINDOW_H
#define APRIL_ANDROID_JNI_WINDOW_H

#include <hltypes/harray.h>
#include <hltypes/hstring.h>

#include "aprilExport.h"
#include "Timer.h"
#include "Window.h"

namespace april
{
	class Cursor;

	class AndroidJNI_Window : public Window
	{
	public:
		AndroidJNI_Window();
		~AndroidJNI_Window();
		
		inline void setTitle(chstr title) override { }
		inline bool isCursorVisible() const override { return false; }
		inline void setCursorVisible(bool value) override { }
		HL_DEFINE_GET_OVERRIDE(int, width, Width);
		HL_DEFINE_GET_OVERRIDE(int, height, Height);
		void* getBackendId() const override;
		
		bool update(float timeDelta) override;
		
		void queueTouchInput(TouchEvent::Type type, int index, cgvec2f position) override;
		void queueControllerInput(ControllerEvent::Type type, int controllerIndex, const Button& buttonCode, float axisValue) override;

		void showVirtualKeyboard() override;
		void hideVirtualKeyboard() override;

		void handleFocusChange(bool focused) override;
		void handleActivityChange(bool active) override;
		
	protected:
		int width;
		int height;
		bool forcedFocus;

		void _systemCreate(int width, int height, bool fullscreen, chstr title, Window::Options options) override;
		
		Cursor* _createCursor(bool fromResource) override;
		void _refreshCursor() override { }
		
		void _presentFrame(bool systemEnabled) override;

	};

}
#endif
#endif