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
/// Defines an interface for implementation if a custom virtual keyboard is required.

#ifndef APRIL_VIRTUAL_KEYBOARD_H
#define APRIL_VIRTUAL_KEYBOARD_H

#include <hltypes/harray.h>
#include <hltypes/hltypesUtil.h>

#include "aprilExport.h"
#include "Keys.h"

namespace april
{
	/// @brief Defines an interface for implementation if a custom virtual keyboard is required.
	class aprilExport VirtualKeyboard
	{
	public:
		/// @brief Basic constructor.
		VirtualKeyboard();
		/// @brief Destructor.
		virtual ~VirtualKeyboard();

		/// @brief Visibility flag.
		HL_DEFINE_IS(visible, Visible);
		/// @brief Height ratio of the screen.
		HL_DEFINE_GET(float, heightRatio, HeightRatio);

		/// @brief Shows the virtual keyboard.
		void showKeyboard(bool forced);
		/// @brief Hides the virtual keyboard.
		void hideKeyboard(bool forced);

		/// @brief Renders the virtual keyboard.
		void drawKeyboard();

	protected:
		/// @brief Visibility flag.
		bool visible;
		/// @brief Height ratio of the screen.
		float heightRatio;

		/// @brief Shows the VirtualKeyboard.
		/// @param[in] forced Whether to force display.
		/// @return Height ratio of the screen.
		virtual float _showKeyboard(bool forced) = 0;
		/// @brief Shows the VirtualKeyboard.
		/// @param[in] forced Whether to force display.
		/// @return True if successful.
		virtual bool _hideKeyboard(bool forced) = 0;

		/// @brief Draws the VirtualKeyboard.
		virtual void _drawKeyboard() = 0;

	};

}
#endif
