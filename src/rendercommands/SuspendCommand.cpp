/// @file
/// @version 4.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <hltypes/hlog.h>

#include "SuspendCommand.h"
#include "RenderSystem.h"

namespace april
{
	SuspendCommand::SuspendCommand(const RenderState& state) : RenderCommand(state)
	{
	}
	
	void SuspendCommand::execute()
	{
		hlog::error("OK", "Suspend");
		april::rendersys->_deviceSuspend();
	}
	
}