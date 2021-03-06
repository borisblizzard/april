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
/// Defines a Win32 window.

#ifdef _WIN32_WINDOW
#ifndef APRIL_WIN32_WINDOW_H
#define APRIL_WIN32_WINDOW_H

#define __HL_INCLUDE_PLATFORM_HEADERS
#include <hltypes/hplatform.h>
#include <hltypes/hstring.h>

#include "aprilExport.h"
#include "Timer.h"
#include "Window.h"

#ifdef _WIN32_XINPUT
#define XINPUT_USE_9_1_0
#include <Xinput.h>
#ifndef XUSER_MAX_COUNT
#define XUSER_MAX_COUNT 4
#endif
#endif

namespace april
{
	class Cursor;

	class Win32_Window : public Window
	{
	public:
		Win32_Window();
		~Win32_Window();

		void setTitle(chstr title);
		bool isCursorVisible() const;
		HL_DEFINE_GET(int, width, Width);
		HL_DEFINE_GET(int, height, Height);
		void* getBackendId() const;
		HCURSOR getCursorHandle() const;

		bool update(float timeDelta);
		void checkEvents();

		void queueControllerInput(const ControllerEvent::Type& type, int controllerIndex, const Button& buttonCode, float axisValue) override;

		static LRESULT CALLBACK childProcessCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		
	protected:
		HWND hWnd;
		HCURSOR defaultCursor;
		int width;
		int height;
		hstr fpsTitle;
		bool refreshCursorRequested;
#ifdef _WIN32_XINPUT
		XINPUT_STATE xinputStates[XUSER_MAX_COUNT];
		bool connectedControllers[XUSER_MAX_COUNT];
#endif

		void _systemCreate(int width, int height, bool fullscreen, chstr title, Window::Options options);
		void _systemDestroy();
		
		Cursor* _createCursor(bool fromResource);

#ifdef _WIN32_XINPUT
		void _checkXInputControllerStates();
#endif
		void _setupStyles(DWORD& style, DWORD& exstyle, bool fullscreen);
		void _adjustWindowSizeForClient(int x, int y, int& width, int& height, DWORD style, DWORD exstyle);
		void _refreshCursor();
		void _updateCursorPosition();
		
		void _systemSetResolution(int width, int height, bool fullscreen);

		void _presentFrame(bool systemEnabled);

		static LRESULT CALLBACK _mainProcessCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		int _mouseMessages;

	};

}
#endif
#endif
