/// @file
/// @version 4.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _OPENGL
#define __HL_INCLUDE_PLATFORM_HEADERS
#include <hltypes/hplatform.h>

#include <hltypes/hlog.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "Image.h"
#include "OpenGL_RenderSystem.h"
#include "OpenGL_Texture.h"

#define APRIL_OGL_RENDERSYS ((OpenGL_RenderSystem*)april::rendersys)

namespace april
{
	bool OpenGL_Texture::_preventRecursion = false;

	OpenGL_Texture::OpenGL_Texture(bool fromResource) : Texture(fromResource), textureId(0), glFormat(0), internalFormat(0)
	{
	}

	OpenGL_Texture::~OpenGL_Texture()
	{
	}

	bool OpenGL_Texture::_deviceCreateTexture(unsigned char* data, int size, Type type)
	{
		glGenTextures(1, &this->textureId);
		if (this->textureId == 0)
		{
			return false;
		}
		this->firstUpload = true;
		return true;
	}
	
	bool OpenGL_Texture::_deviceDestroyTexture()
	{
		if (this->textureId != 0)
		{
			glDeleteTextures(1, &this->textureId);
			this->textureId = 0;
			this->firstUpload = true;
			return true;
		}
		return false;
	}

	void OpenGL_Texture::_assignFormat()
	{
		if (this->format == Image::FORMAT_ARGB || this->format == Image::FORMAT_XRGB || this->format == Image::FORMAT_RGBA ||
			this->format == Image::FORMAT_RGBX || this->format == Image::FORMAT_ABGR || this->format == Image::FORMAT_XBGR)
		{
			this->glFormat = this->internalFormat = GL_RGBA;
		}
		else if (this->format == Image::FORMAT_BGRA || this->format == Image::FORMAT_BGRX)
		{
#if !defined(_ANDROID) && !defined(_WIN32)
#ifndef __APPLE__
			this->glFormat = GL_BGRA; // for optimizations
#else
			this->glFormat = GL_BGRA_EXT; // iOS doesn't accept BGR. This option hasn't been tested.
#endif
#else
			this->glFormat = GL_RGBA;
#endif
			this->internalFormat = GL_RGBA;
		}
		else if (this->format == Image::FORMAT_RGB)
		{
			this->glFormat = this->internalFormat = GL_RGB;
		}
		else if (this->format == Image::FORMAT_BGR)
		{
#if !defined(_ANDROID) && !defined(_WIN32)
#ifndef __APPLE__
			this->glFormat = GL_BGR; // for optimizations
#else
			this->glFormat = GL_BGRA_EXT; // iOS doesn't accept BGR. This option hasn't been tested.
#endif
#else
			this->glFormat = GL_RGB;
#endif
			this->internalFormat = GL_RGB;
		}
		else if (this->format == Image::FORMAT_ALPHA)
		{
			this->glFormat = this->internalFormat = GL_ALPHA;
		}
		else if (this->format == Image::FORMAT_GRAYSCALE)
		{
			this->glFormat = this->internalFormat = GL_LUMINANCE;
		}
		else if (this->format == Image::FORMAT_COMPRESSED)
		{
			this->glFormat = this->internalFormat = 0; // compressed image formats will set these values as they need to
		}
		else if (this->format == Image::FORMAT_PALETTE)
		{
			this->glFormat = this->internalFormat = 0; // paletted image formats will set these values as they need to
		}
		else
		{
			this->glFormat = this->internalFormat = GL_RGBA;
		}
	}

	void OpenGL_Texture::_setCurrentTexture()
	{
		APRIL_OGL_RENDERSYS->_setDeviceTexture(this);
		APRIL_OGL_RENDERSYS->_setDeviceTextureFilter(this->filter);
		APRIL_OGL_RENDERSYS->_setDeviceTextureAddressMode(this->addressMode);
		APRIL_OGL_RENDERSYS->deviceState->texture = this;
	}

	Texture::Lock OpenGL_Texture::_tryLockSystem(int x, int y, int w, int h)
	{
		Lock lock;
		Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(this->format);
		int gpuBpp = Image::getFormatBpp(nativeFormat);
		lock.activateLock(0, 0, w, h, x, y, new unsigned char[w * h * gpuBpp], w, h, nativeFormat);
		lock.systemBuffer = lock.data;
		return lock;
	}

	bool OpenGL_Texture::_unlockSystem(Lock& lock, bool update)
	{
		if (lock.systemBuffer == NULL)
		{
			return false;
		}
		if (update)
		{
			if (this->format != Image::FORMAT_COMPRESSED && this->format != Image::FORMAT_PALETTE)
			{
				this->_setCurrentTexture();
				if (this->width == lock.w && this->height == lock.h)
				{
					this->_uploadPotSafeData(lock.data);
				}
				else
				{
					if (this->firstUpload)
					{
						this->_uploadPotSafeClearData();
					}
					glTexSubImage2D(GL_TEXTURE_2D, 0, lock.dx, lock.dy, lock.w, lock.h, this->glFormat, GL_UNSIGNED_BYTE, lock.data);
				}
				this->firstUpload = false;
			}
		}
		delete[] lock.data;
		return update;
	}

	bool OpenGL_Texture::_uploadToGpu(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		if (this->format == Image::FORMAT_COMPRESSED || this->format == Image::FORMAT_PALETTE)
		{
			return false;
		}
		this->_setCurrentTexture();
		if (sx == 0 && dx == 0 && sy == 0 && dy == 0 && sw == this->width && srcWidth == this->width && sh == this->height && srcHeight == this->height)
		{
			this->_uploadPotSafeData(srcData);
		}
		else
		{
			if (this->firstUpload)
			{
				this->_uploadPotSafeClearData();
			}
			int srcBpp = Image::getFormatBpp(srcFormat);
			if (sx == 0 && dx == 0 && srcWidth == this->width && sw == this->width)
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, sw, sh, this->glFormat, GL_UNSIGNED_BYTE, &srcData[(sx + sy * srcWidth) * srcBpp]);
			}
			else
			{
				for_iter (j, 0, sh)
				{
					glTexSubImage2D(GL_TEXTURE_2D, 0, dx, (dy + j), sw, 1, this->glFormat, GL_UNSIGNED_BYTE, &srcData[(sx + (sy + j) * srcWidth) * srcBpp]);
				}
			}
		}
		this->firstUpload = false;
		return true;
	}

	void OpenGL_Texture::_uploadPotSafeData(unsigned char* data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, data);
		GLenum glError = glGetError();
		SAFE_TEXTURE_UPLOAD_CHECK(glError, glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, data));
		RenderSystem::Caps caps = april::rendersys->getCaps();
		if (glError == GL_INVALID_VALUE && !caps.npotTexturesLimited && !caps.npotTextures)
		{
			int w = this->width;
			int h = this->height;
			unsigned char* newData = this->_createPotData(w, h, data);
			this->_setCurrentTexture(); // has to call this again after _createPotData(), because some internal properties could have changed
			glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, w, h, 0, this->glFormat, GL_UNSIGNED_BYTE, newData);
			glError = glGetError();
			SAFE_TEXTURE_UPLOAD_CHECK(glError, glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, w, h, 0, this->glFormat, GL_UNSIGNED_BYTE, newData));
			delete[] newData;
		}
	}

	void OpenGL_Texture::_uploadPotSafeClearData()
	{
		int size = this->getByteSize();
		unsigned char* clearColor = new unsigned char[size];
		memset(clearColor, 0, size);
		glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, clearColor);
		GLenum glError = glGetError();
		SAFE_TEXTURE_UPLOAD_CHECK(glError, glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, clearColor));
		delete[] clearColor;
		RenderSystem::Caps caps = april::rendersys->getCaps();
		if (glError == GL_INVALID_VALUE && !caps.npotTexturesLimited && !caps.npotTextures)
		{
			int w = this->width;
			int h = this->height;
			clearColor = this->_createPotClearData(w, h); // can create POT sized data
			this->_setCurrentTexture(); // has to call this again after _createPotData(), because some internal properties could have changed
			glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, clearColor);
			glError = glGetError();
			SAFE_TEXTURE_UPLOAD_CHECK(glError, glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, clearColor));
			delete[] clearColor;
		}
	}

}
#endif
