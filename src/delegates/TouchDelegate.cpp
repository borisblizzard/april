/// @file
/// @version 5.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <hltypes/hlog.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "TouchDelegate.h"

namespace april
{
	static const gvec2f _unusedPosition(-10000.0f, 10000.0f);

	TouchDelegate::TouchDelegate()
	{
	}

	TouchDelegate::~TouchDelegate()
	{
	}

	gvec2f TouchDelegate::getCurrentTouch(int index) const
	{
		return this->currentTouches.tryGet(index, _unusedPosition);
	}

	void TouchDelegate::onTouchDown(int index)
	{
		hlog::debug(logTag, "Event onTouchDown() was not implemented.");
	}

	void TouchDelegate::onTouchUp(int index)
	{
		hlog::debug(logTag, "Event onTouchUp() was not implemented.");
	}

	void TouchDelegate::onTouchMove(int index)
	{
		hlog::debug(logTag, "Event onTouchMove() was not implemented.");
	}

	void TouchDelegate::onTouchCancel(int index)
	{
		hlog::debug(logTag, "Event onTouchCancel() was not implemented.");
	}

	void TouchDelegate::onTouch(const harray<gvec2f>& touches)
	{
		hlog::debug(logTag, "Event onTouch() was not implemented.");
	}

}
