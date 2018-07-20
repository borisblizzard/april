/// @file
/// @version 5.1
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _UWP_WINDOW
#include <hltypes/hfile.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hlog.h>
#include <hltypes/hrdir.h>
#include <hltypes/hthread.h>

#include "april.h"
#include "DirectX12_RenderSystem.h"
#include "Platform.h"
#include "RenderSystem.h"
#include "SystemDelegate.h"
#include "Timer.h"
#include "UWP.h"
#include "UWP_Cursor.h"
#include "UWP_Window.h"

using namespace Windows::UI::ViewManagement;

#define DX12_RENDERSYS ((DirectX12_RenderSystem*)rendersys)

namespace april
{
	UWP_Window::UWP_Window() : Window()
	{
		this->name = april::WindowType::UWP.getName();
		this->width = 0;
		this->height = 0;
		this->delaySplash = 0.0f;
		this->cursorExtensions += ".ani";
		this->cursorExtensions += ".cur";
	}

	UWP_Window::~UWP_Window()
	{
		this->destroy();
	}

	void UWP_Window::_systemCreate(int width, int height, bool fullscreen, chstr title, Window::Options options)
	{
		if (options.minimized)
		{
			options.minimized = false;
			hlog::warn(logTag, "Option 'minimized' is not supported on window system: " + this->name);
		}
		Rect rect = CoreWindow::GetForCurrentThread()->Bounds;
		width = (int)rect.Width;
		height = (int)rect.Height;
		// TODOuwp - implement
		//ApplicationView::GetForCurrentView()->FullScreenSystemOverlayMode = Windows::UI::ViewManagement::ApplicationView::FullScreenSystemOverlayMode::TryEnterFullScreenMode();
		//ApplicationView::GetForCurrentView()->IsFullScreenMode = fullscreen;
		fullscreen = ApplicationView::GetForCurrentView()->IsFullScreenMode;
		Window::_systemCreate(width, height, fullscreen, title, options);
		this->width = width;
		this->height = height;
		this->delaySplash = 0.0f;
		this->backButtonSystemHandling = false;
		this->cursorMappings.clear();
		this->inputMode = InputMode::Touch;
		this->setCursorVisible(true);
		return;
	}
	
	hstr UWP_Window::getParam(chstr param)
	{
#ifndef _WINP8
		if (param == UWP_CURSOR_MAPPINGS)
		{
			harray<hstr> mappings;
			foreach_m (int, it, this->cursorMappings)
			{
				mappings += hsprintf("%u %s", it->second, it->first.cStr());
			}
			return mappings.joined('\n');
		}
#else
		if (param == WINP8_BACK_BUTTON_SYSTEM_HANDLING)
		{
			return hstr(this->backButtonSystemHandling ? "1" : "0");
		}
#endif
		if (param == UWP_DELAY_SPLASH)
		{
			return hstr(this->delaySplash);
		}
		return "";
	}

	void UWP_Window::setParam(chstr param, chstr value)
	{
#ifndef _WINP8
		if (param == UWP_CURSOR_MAPPINGS)
		{
			this->cursorMappings.clear();
			harray<hstr> lines = value.split('\n', -1, true);
			harray<hstr> data;
			foreach (hstr, it, lines)
			{
				data = (*it).split(' ', 1);
				if (data.size() == 2)
				{
					this->cursorMappings[data[1]] = (int)data[0];
				}
			}
			return;
		}
#else
		if (param == WINP8_BACK_BUTTON_SYSTEM_HANDLING)
		{
			this->backButtonSystemHandling = (value != "0");
			return;
		}
#endif
		if (param == UWP_DELAY_SPLASH)
		{
			this->delaySplash = (float)value;
			return;
		}
		Window::setParam(param, value);
	}

	void UWP_Window::setTitle(chstr title)
	{
		hlog::warn(logTag, "Window::setTitle() does nothing on WinRT.");
	}
	
	void* UWP_Window::getBackendId() const
	{
		// TODO ?
		return 0;
	}

	void UWP_Window::_systemSetResolution(int width, int height, bool fullscreen)
	{
		this->width = width;
		this->height = height;
		this->fullscreen = this->fullscreen;
		this->_setRenderSystemResolution(this->width, this->height, this->fullscreen);
	}

	bool UWP_Window::update(float timeDelta)
	{
		ID3D12CommandQueue* commandQueue = DX12_RENDERSYS->getCommandQueue();
		PIXBeginEvent(commandQueue, 0, L"update()");
		bool result = Window::update(timeDelta);
		PIXEndEvent(commandQueue);
		return result;
	}

	void UWP_Window::checkEvents()
	{
		// TODOuwp - implement this
		/*
		if (this->visible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
		*/
		CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
		Window::checkEvents();
	}

	void UWP_Window::showVirtualKeyboard()
	{
		//UWP::App->Overlay->showKeyboard();
	}

	void UWP_Window::hideVirtualKeyboard()
	{
		//UWP::App->Overlay->hideKeyboard();
	}

	hstr UWP_Window::findCursorFile(chstr filename) const
	{
		if (filename != "")
		{
			foreachc (hstr, it, this->cursorExtensions)
			{
				if (this->cursorMappings.hasKey(filename))
				{
					return hstr(this->cursorMappings.tryGet(filename, 0));
				}
			}
		}
		return "0";
	}

	Cursor* UWP_Window::_createCursor(bool fromResource)
	{
		return new UWP_Cursor(fromResource);
	}
	
	void UWP_Window::_refreshCursor()
	{
		UWP::App->refreshCursor();
	}
	
}
#endif