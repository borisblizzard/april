/// @file
/// @version 4.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#define __HL_INCLUDE_PLATFORM_HEADERS
#include <hltypes/hplatform.h>

#include <gl/GL.h>
#define GL_GLEXT_PROTOTYPES
#include <gl/glext.h>

#include <hltypes/hlog.h>
#include <hltypes/hstring.h>

#include "CustomRenderSystem.h"
#include "CustomTexture.h"

#define CUSTOM_RENDERSYS ((CustomRenderSystem*)april::rendersys)

CustomTexture::CustomTexture(bool fromResource) : Texture(fromResource), textureId(0), glFormat(0), internalFormat(0)
{
}

CustomTexture::~CustomTexture()
{
}

bool CustomTexture::_deviceCreateTexture(unsigned char* data, int size, april::Texture::Type type)
{
	glGenTextures(1, &this->textureId);
	if (this->textureId == 0)
	{
		return false;
	}
	this->firstUpload = true;
	return true;
}

bool CustomTexture::_deviceDestroyTexture()
{
	if (this->textureId != 0)
	{
		glDeleteTextures(1, &this->textureId);
		this->textureId = 0;
		return true;
	}
	return false;
}

void CustomTexture::_assignFormat()
{
	switch (this->format)
	{
	case april::Image::FORMAT_ARGB:
	case april::Image::FORMAT_XRGB:
	case april::Image::FORMAT_RGBA:
	case april::Image::FORMAT_RGBX:
	case april::Image::FORMAT_ABGR:
	case april::Image::FORMAT_XBGR:
		this->glFormat = this->internalFormat = GL_RGBA;
		break;
	case april::Image::FORMAT_BGRA:
	case april::Image::FORMAT_BGRX:
		this->glFormat = GL_RGBA;
		this->internalFormat = GL_RGBA;
		break;
	case april::Image::FORMAT_RGB:
		this->glFormat = this->internalFormat = GL_RGB;
		break;
	case april::Image::FORMAT_BGR:
		this->glFormat = GL_RGB;
		this->internalFormat = GL_RGB;
		break;
	case april::Image::FORMAT_ALPHA:
		this->glFormat = this->internalFormat = GL_ALPHA;
		break;
	case april::Image::FORMAT_GRAYSCALE:
		this->glFormat = this->internalFormat = GL_LUMINANCE;
		break;
	case april::Image::FORMAT_COMPRESSED:
		this->glFormat = this->internalFormat = 0;
		break;
	case april::Image::FORMAT_PALETTE:
		this->glFormat = this->internalFormat = 0;
		break;
	default:
		this->glFormat = this->internalFormat = GL_RGBA;
		break;
	}
}

void CustomTexture::_setCurrentTexture()
{
	CUSTOM_RENDERSYS->deviceState->texture = this;
	CUSTOM_RENDERSYS->_setDeviceTexture(this);
	CUSTOM_RENDERSYS->_setDeviceTextureFilter(this->filter);
	CUSTOM_RENDERSYS->_setDeviceTextureAddressMode(this->addressMode);
}

april::Texture::Lock CustomTexture::_tryLockSystem(int x, int y, int w, int h)
{
	april::Texture::Lock lock;
	april::Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(this->format);
	int gpuBpp = april::Image::getFormatBpp(nativeFormat);
	lock.activateLock(0, 0, w, h, x, y, new unsigned char[w * h * gpuBpp], w, h, nativeFormat);
	lock.systemBuffer = lock.data;
	return lock;
}

bool CustomTexture::_unlockSystem(Lock& lock, bool update)
{
	if (lock.systemBuffer == NULL)
	{
		return false;
	}
	if (update)
	{
		if (this->format != april::Image::FORMAT_COMPRESSED && this->format != april::Image::FORMAT_PALETTE)
		{
			this->_setCurrentTexture();
			if (this->width == lock.w && this->height == lock.h)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, lock.data);
			}
			else
			{
				if (this->firstUpload)
				{
					this->_uploadClearData();
				}
				glTexSubImage2D(GL_TEXTURE_2D, 0, lock.dx, lock.dy, lock.w, lock.h, this->glFormat, GL_UNSIGNED_BYTE, lock.data);
			}
		}
		delete[] lock.data;
		this->firstUpload = false;
	}
	return update;
}

bool CustomTexture::_uploadToGpu(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, april::Image::Format srcFormat)
{
	if (this->format == april::Image::FORMAT_COMPRESSED || this->format == april::Image::FORMAT_PALETTE)
	{
		return false;
	}
	this->load();
	this->_setCurrentTexture();
	if (sx == 0 && dx == 0 && sy == 0 && dy == 0 && sw == this->width && srcWidth == this->width && sh == this->height && srcHeight == this->height)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, srcData);
	}
	else
	{
		if (this->firstUpload)
		{
			this->_uploadClearData();
		}
		int srcBpp = april::Image::getFormatBpp(srcFormat);
		if (sx == 0 && dx == 0 && srcWidth == this->width && sw == this->width)
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, sw, sh, this->glFormat, GL_UNSIGNED_BYTE, &srcData[(sx + sy * srcWidth) * srcBpp]);
		}
		else
		{
			for_iter(j, 0, sh)
			{
				glTexSubImage2D(GL_TEXTURE_2D, 0, dx, (dy + j), sw, 1, this->glFormat, GL_UNSIGNED_BYTE, &srcData[(sx + (sy + j) * srcWidth) * srcBpp]);
			}
		}
	}
	this->firstUpload = false;
	return true;
}

void CustomTexture::_uploadClearData()
{
	int size = this->getByteSize();
	unsigned char* clearColor = new unsigned char[size];
	memset(clearColor, 0, size);
	glTexImage2D(GL_TEXTURE_2D, 0, this->internalFormat, this->width, this->height, 0, this->glFormat, GL_UNSIGNED_BYTE, clearColor);
	delete[] clearColor;
}
