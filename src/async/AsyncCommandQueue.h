/// @file
/// @version 4.5
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines a async command queue.

#ifndef APRIL_ASYNC_COMMAND_QUEUE_H
#define APRIL_ASYNC_COMMAND_QUEUE_H

#include <hltypes/harray.h>

#include "RenderCommand.h"

namespace april
{
	class AsyncCommand;

	class AsyncCommandQueue
	{
	public:
		harray<AsyncCommand*> commands;

		AsyncCommandQueue();
		~AsyncCommandQueue();
		
	};
	
}
#endif