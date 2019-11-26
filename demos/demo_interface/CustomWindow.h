/// @file
/// @version 5.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifndef CUSTOM_WINDOW_H
#define CUSTOM_WINDOW_H

#define __HL_INCLUDE_PLATFORM_HEADERS
#include <hltypes/hplatform.h>
#include <hltypes/hstring.h>

#include <april/Timer.h>
#include <april/Window.h>

#ifdef _CUSTOM_XINPUT
#define XINPUT_USE_9_1_0
#include <Xinput.h>
#ifndef XUSER_MAX_COUNT
#define XUSER_MAX_COUNT 4
#endif
#endif

namespace april
{
	class CustomWindow : public Window
	{
	public:
		CustomWindow();
		~CustomWindow();

		void setTitle(chstr title);
		bool isCursorVisible() const;
		HL_DEFINE_GET(int, width, Width);
		HL_DEFINE_GET(int, height, Height);
		void* getBackendId() const;
		HCURSOR getCursorHandle() const;

		bool update(float timeDelta);
		void checkEvents();

		void queueControllerInput(const ControllerEvent::Type& type, int controllerIndex, const Button& buttonCode, float axisValue);

		static LRESULT CALLBACK childProcessCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		
	protected:
		HWND hWnd;
		HCURSOR defaultCursor;
		int width;
		int height;
		hstr fpsTitle;
		bool refreshCursorRequested;
#ifdef _CUSTOM_XINPUT
		XINPUT_STATE xinputStates[XUSER_MAX_COUNT];
		bool connectedControllers[XUSER_MAX_COUNT];
#endif

		void _systemCreate(int width, int height, bool fullscreen, chstr title, Window::Options options);
		void _systemDestroy();
		
#ifdef _CUSTOM_XINPUT
		void _checkXInputControllerStates();
#endif
		void _setupStyles(DWORD& style, DWORD& exstyle, bool fullscreen);
		void _adjustWindowSizeForClient(int x, int y, int& width, int& height, DWORD style, DWORD exstyle);
		void _refreshCursor();
		void _updateCursorPosition();
		
		void _systemSetResolution(int width, int height, bool fullscreen);

		static LRESULT CALLBACK _mainProcessCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		int _mouseMessages;

	};

}
#endif
