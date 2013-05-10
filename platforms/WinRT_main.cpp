/// @file
/// @author  Boris Mikic
/// @version 3.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php

#ifdef _WIN32
#include <hltypes/hplatform.h>
#if _HL_WINRT
#include <hltypes/harray.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hstring.h>

#include "main.h"
#include "RenderSystem.h"
#include "Window.h"
#include "WinRT_ViewSource.h"
#include "WinRT_View.h"

using namespace Windows::ApplicationModel::Core;

int april_main(void (*anAprilInit)(const harray<hstr>&), void (*anAprilDestroy)(), const harray<hstr>& args, int argc, char** argv)
{
	april::WinRT::Args = args;
	april::WinRT::Init = anAprilInit;
	april::WinRT::Destroy = anAprilDestroy;
	CoreApplication::Run(ref new april::WinRT_ViewSource());
	return 0;
}
#endif
#endif