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
/// Defines a generic application.

#ifndef APRIL_APPLICATION_H
#define APRIL_APPLICATION_H

#include <hltypes/harray.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hstring.h>

#include "aprilExport.h"
#include "Timer.h"

namespace april
{
	/// @brief Defines a generic application.
	class aprilExport Application
	{
	public:
		/// @brief Basic constructor.
		/// @param[in] aprilApplicationInit Initialization callback.
		/// @param[in] aprilApplicationDestroy Destruction callback.
		Application(void (*aprilApplicationInit)(), void (*aprilApplicationDestroy)());
		/// @brief Destructor.
		virtual ~Application();

		/// @brief Get launch arguments.
		HL_DEFINE_GETSET(harray<hstr>, args, Args);
		/// @brief Get running flag.
		HL_DEFINE_IS(running, Running);
		/// @brief The current FPS.
		HL_DEFINE_GETSET(int, fps, Fps);
		/// @brief The FPS resolution.
		HL_DEFINE_GETSET(float, fpsResolution, FpsResolution);
		/// @brief The maximum allowed time-delta between frames.
		/// @note Limiting this makes sense, because on weak hardware configurations it allows that large frameskips don't result in too large time skips.
		HL_DEFINE_GETSET(float, timeDeltaMaxLimit, TimeDeltaMaxLimit);

		/// @brief Calls the initialization procedure defined by the user.
		/// @param[in] args Launch arguments.
		void init();
		/// @brief Calls the destruction procedure defined by the user.
		void destroy();

		/// @brief Starts the main loop.
		/// @note This is usually called internally in some implementations, but it's possible to call it manually if a custom april::__mainStandard implementation is used.
		void enterMainLoop();
		/// @brief Performs the update of one frame.
		/// @note This is usually called internally in some implementations, but it's possible to call it manually if a custom april::__mainStandard implementation is used.
		virtual void update();

	protected:
		/// @brief Launch arguments.
		harray<hstr> args;
		/// @brief Initialization callback.
		void (*aprilApplicationInit)();
		/// @brief Destruction callback.
		void (*aprilApplicationDestroy)();
		/// @brief Running flag.
		bool running;
		/// @brief Whether automatic presentFrame() implementation is used by the underlying system.
		bool autoPresentFrame;
		/// @brief The Timer object used for timing purposes.
		Timer timer;
		/// @brief FPS of the last mesaure.
		int fps;
		/// @brief Current counter for FPS calculation.
		int fpsCount;
		/// @brief Current timer for FPS calculation.
		float fpsTimer;
		/// @brief FPS update resolution.
		float fpsResolution;
		/// @brief Maximum allowed time-delta that are propagated into the UpdateDelegate.
		/// @note Limiting this makes sense, because on weak hardware configurations it allows that large frameskips don't result in too large time skips.
		float timeDeltaMaxLimit;

	};

	/// @brief The global Application instance.
	aprilExport extern april::Application* application;

}
#endif