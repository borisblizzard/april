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
/// Defines a forced state update command.

#ifndef APRIL_STATE_UPDATE_COMMAND_H
#define APRIL_STATE_UPDATE_COMMAND_H

#include "AsyncCommand.h"
#include "RenderState.h"

namespace april
{
	class StateUpdateCommand : public AsyncCommand
	{
	public:
		StateUpdateCommand(const RenderState& state);

		RenderState* getState() { return &this->state; }
		bool isUseState() const override { return true; }
		bool isSystemCommand() const override { return true; }
		bool isRepeatable() const override { return true; }

		void execute() override;

	protected:
		RenderState state;

	};
	
}
#endif
