/// @file
/// @version 1.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines methods for loading a PVR image.

#ifndef APRILPIX_IMAGE_PVR_H
#define APRILPIX_IMAGE_PVR_H

#include <april/Image.h>
#include <hltypes/harray.h>
#include <hltypes/hsbase.h>
#include <hltypes/hstring.h>

#include "aprilpixExport.h"

namespace aprilpix
{
	class aprilpixExport ImagePvr : public april::Image
	{
	public:
		~ImagePvr();

		static april::Image* load(hsbase& stream);
		static april::Image* loadMetaData(hsbase& stream);
	
	protected:
		ImagePvr();

	};

};

#endif

