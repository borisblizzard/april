﻿/// @file
/// @version 5.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines a UWP App.

#ifdef _UWP_WINDOW
#ifndef APRIL_UWP_APP_H
#define APRIL_UWP_APP_H

#include <gtypes/Vector2.h>
#include <hltypes/harray.h>

#include "Keys.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;

namespace april
{
	class Texture;
	
	ref class UWP_App sealed : public Core::IFrameworkView
	{
	public:
		UWP_App();

		inline bool isVisible() { return this->visible; } // can't use HL_DEFINE, const is not supported in WinRT types

		// IFrameworkView methods.
		virtual void Initialize(Core::CoreApplicationView^ applicationView);
		virtual void SetWindow(CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

		void updateMainThread();

		void setWindowTitle(Platform::String^ title); // can't use chstr, const is not supported in WinRT types
		void refreshCursor();
		void showVirtualKeyboard();
		void hideVirtualKeyboard();

		void onVirtualKeyboardShow(_In_ InputPane^ sender, _In_ InputPaneVisibilityEventArgs^ args);
		void onVirtualKeyboardHide(_In_ InputPane^ sender, _In_ InputPaneVisibilityEventArgs^ args);

		void onTouchDown(_In_ CoreWindow^ sender, _In_ PointerEventArgs^ args);
		void onTouchUp(_In_ CoreWindow^ sender, _In_ PointerEventArgs^ args);
		void onTouchMove(_In_ CoreWindow^ sender, _In_ PointerEventArgs^ args);
		void onMouseScroll(_In_ CoreWindow^ sender, _In_ PointerEventArgs^ args);
		void onKeyDown(_In_ CoreWindow^ sender, _In_ KeyEventArgs^ args);
		void onKeyUp(_In_ CoreWindow^ sender, _In_ KeyEventArgs^ args);
		void onCharacterReceived(_In_ CoreWindow^ sender, _In_ CharacterReceivedEventArgs^ args);
		void onAcceleratorKeyActivated(_In_ CoreDispatcher^ sender, _In_ AcceleratorKeyEventArgs^ args);

	protected:
		// Application lifecycle events
		void onUnhandledErrorDetected(Platform::Object^ sender, UnhandledErrorDetectedEventArgs^ args);
		void onActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args);
		void onSuspending(Platform::Object^ sender, SuspendingEventArgs^ args);
		void onResuming(Platform::Object^ sender, Platform::Object^ args);
		// CoreWindow events
		void onWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args);
		void onWindowFocusChanged(CoreWindow^ window, WindowActivatedEventArgs^ args);
		void onVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args);
		void onWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args);
		// DisplayInformation events
		void onDpiChanged(DisplayInformation^ sender, Platform::Object^ args);
		void onOrientationChanged(DisplayInformation^ sender, Platform::Object^ args);

		void _processWindowSizeChange(float width, float height);
		void _processWindowFocusChange(bool focused);

	private:
		bool running;
		bool visible;
		CoreCursor^ defaultCursor;
		bool scrollHorizontal;
		harray<unsigned int> pointerIds;
		int64_t startTime;
		april::Key currentButton;
		/// @brief This is required due to multi-threading issues, because InputPane cannot be called in a non-main thread
		Platform::String^ windowTitleRequested;
		bool refreshCursorRequested;
		bool virtualKeyboardCurrentState;
		bool virtualKeyboardRequestState;

		gvec2f _transformPosition(float x, float y);
		void _resetTouches();

	};

}
#endif
#endif
