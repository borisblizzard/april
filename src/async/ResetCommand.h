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
/// Defines a reset command.

#ifndef APRIL_RESET_COMMAND_H
#define APRIL_RESET_COMMAND_H

#include <gtypes/Vector2.h>

#include "StateUpdateCommand.h"

namespace april
{
	class ResetCommand : public StateUpdateCommand
	{
	public:
		ResetCommand(const RenderState& state, cgvec2i windowSize);

		bool isFinalizer() const override { return true; }
		bool isSystemCommand() const override { return true; }

		void execute() override;

	protected:
		gvec2i windowSize;

	};
	
}
#endif
