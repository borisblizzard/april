/// @file
/// @version 3.6
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Defines an OpenGLES2 specific texture.

#ifdef _OPENGLES2
#ifndef APRIL_OPENGLES2_TEXTURE_H
#define APRIL_OPENGLES2_TEXTURE_H

#include <hltypes/hstring.h>

#include "Color.h"
#include "OpenGLES_Texture.h"

namespace april
{
	class OpenGLES2_Texture : public OpenGLES_Texture
	{
	public:
		OpenGLES2_Texture(bool fromResource);
		~OpenGLES2_Texture();

	};

}

#endif
#endif
