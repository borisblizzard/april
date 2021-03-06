/// @file
/// @version 5.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "CreateCommand.h"
#include "RenderSystem.h"

namespace april
{
	CreateCommand::CreateCommand(const RenderSystem::Options& options) :
		AsyncCommand()
	{
		this->options = options;
	}

	void CreateCommand::execute()
	{
		april::rendersys->_systemCreate(this->options);
	}

}
