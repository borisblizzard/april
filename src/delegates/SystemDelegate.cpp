/// @file
/// @author  Boris Mikic
/// @version 3.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#include <hltypes/hlog.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "SystemDelegate.h"

namespace april
{
	SystemDelegate::SystemDelegate()
	{
	}

	SystemDelegate::~SystemDelegate()
	{
	}

	bool SystemDelegate::onQuit(bool canCancel)
	{
		hlog::debug(april::logTag, "Event onQuit() was not implemented.");
		return true;
	}

	void SystemDelegate::onWindowSizeChanged(int width, int height, Window::DeviceOrientation deviceOrientation)
	{
		hlog::debug(april::logTag, "Event onWindowSizeChanged() was not implemented.");
	}

	void SystemDelegate::onWindowFocusChanged(bool focused)
	{
		hlog::debug(april::logTag, "Event onWindowFocusChanged() was not implemented.");
	}

	void SystemDelegate::onVirtualKeyboardVisibilityChanged(bool visible)
	{
		hlog::debug(april::logTag, "Event onVirtualKeyboardVisibilityChanged() was not implemented.");
	}

	bool SystemDelegate::onHandleUrl(chstr url)
	{
		hlog::debug(april::logTag, "Event onHandleUrl() was not implemented.");
		return false;
	}

	void SystemDelegate::onLowMemoryWarning()
	{
		hlog::debug(april::logTag, "Event onLowMemoryWarning() was not implemented.");
	}

}
