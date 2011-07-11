/************************************************************************************\
This source file is part of the Awesome Portable Rendering Interface Library         *
For latest info, see http://libapril.sourceforge.net/                                *
**************************************************************************************
Copyright (c) 2010 Kresimir Spes, Ivan Vucica                                        *
*                                                                                    *
* This program is free software; you can redistribute it and/or modify it under      *
* the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
\************************************************************************************/
#ifdef USE_IL
#include <IL/il.h>
#endif

#include <hltypes/harray.h>
#include <hltypes/hfile.h>
#include <hltypes/hstring.h>
#include <hltypes/util.h>

#include "Color.h"
#include "ImageSource.h"
#include "RenderSystem.h"

namespace april
{
	Texture::Texture()
	{
		mFilename = "";
		mUnusedTimer = 0;
		mTextureFilter = Linear;
		mTextureWrapping = true;
	}

	Texture::~Texture()
	{
		foreach (Texture*, it, mDynamicLinks)
		{
			(*it)->removeDynamicLink(this);
		}
	}
	
	Color Texture::getPixel(int x, int y)
	{
		return APRIL_COLOR_CLEAR; // TODO
	}
	
	void Texture::setPixel(int x, int y, Color color)
	{
		// TODO
	}
	
	void Texture::fillRect(int x, int y, int w, int h, Color color)
	{
		// TODO
	}
	
	void Texture::blit(int x, int y, Texture* texture, int sx, int sy, int sw, int sh, unsigned char alpha)
	{
		// TODO
	}

	void Texture::stretchBlit(int x, int y, int w, int h, Texture* texture, int sx, int sy, int sw, int sh, unsigned char alpha)
	{
		// TODO
	}

	void Texture::fillRect(grect rect, Color color)
	{
		fillRect((float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h, color);
	}
	
	Color Texture::getInterpolatedPixel(float x, float y)
	{
		Color result;
		int x0 = (int)x;
		int y0 = (int)x;
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		float rx0 = x - x0;
		float ry0 = y - y0;
		float rx1 = 1.0f - rx0;
		float ry1 = 1.0f - ry0;
		if (rx0 != 0.0f && ry0 != 0.0f)
		{
			Color tl = this->getPixel(x0, y0);
			Color tr = this->getPixel(x1, y0);
			Color bl = this->getPixel(x0, y1);
			Color br = this->getPixel(x1, y1);
			result = (tl * ry1 + bl * ry0) * rx1 + (tr * ry1 + br * ry0) * rx0;
		}
		else if (rx0 != 0.0f)
		{
			Color tl = this->getPixel(x0, y0);
			Color tr = this->getPixel(x1, y0);
			result = tl * rx1 + tr * rx0;
		}
		else if (ry0 != 0.0f)
		{
			Color tl = this->getPixel(x0, y0);
			Color bl = this->getPixel(x0, y1);
			result = tl * ry1 + bl * ry0;
		}
		else
		{
			result = this->getPixel(x0, y0);
		}
		return result;
	}
	
	void Texture::update(float time_increase)
	{
		if (mDynamic && isLoaded())
		{
			float max_time = rendersys->getIdleTextureUnloadTime();
			if (max_time > 0)
			{
				if (mUnusedTimer > max_time)
				{
					unload();
				}
				mUnusedTimer += time_increase;
			}
		}
	}
	
	void Texture::addDynamicLink(Texture* lnk)
	{
		if (!mDynamicLinks.contains(lnk))
		{
			mDynamicLinks += lnk;
			lnk->addDynamicLink(this);
		}
	}
	
	void Texture::removeDynamicLink(Texture* lnk)
	{
		if (mDynamicLinks.contains(lnk))
		{
			mDynamicLinks -= lnk;
		}
	}
	
	void  Texture::_resetUnusedTimer(bool recursive)
	{
		mUnusedTimer = 0;
		if (recursive)
		{
			foreach (Texture*, it, mDynamicLinks)
			{
				(*it)->_resetUnusedTimer(0);
			}
		}
	}
/************************************************************************************/
	RAMTexture::RAMTexture(chstr filename, bool dynamic)
	{
		mFilename = filename;
		mBuffer = NULL;
		if (!dynamic)
		{
			load();
		}
	}

	RAMTexture::RAMTexture(int w, int h)
	{
		mWidth = w;
		mHeight = h;
		mBuffer = createEmptyImage(w, h);
		mFilename = "";
	}

	RAMTexture::~RAMTexture()
	{
		unload();
	}
	
	void RAMTexture::load()
	{
		if (!mBuffer)
		{
			april::log("loading RAM texture '" + mFilename + "'");
			mBuffer = loadImage(mFilename);
			mWidth = mBuffer->w;
			mHeight = mBuffer->h;
		}
	}
	
	void RAMTexture::unload()
	{
		if (mBuffer)
		{
			april::log("unloading RAM texture '" + mFilename + "'");
			delete mBuffer;
			mBuffer = NULL;
		}
	}
	
	bool RAMTexture::isLoaded()
	{
		return (mBuffer != NULL);
	}
	
	Color RAMTexture::getPixel(int x, int y)
	{
		if (!mBuffer)
		{
			load();
		}
		mUnusedTimer = 0;
		return mBuffer->getPixel(x, y);
	}
	
	void RAMTexture::setPixel(int x, int y, Color c)
	{
		if (!mBuffer)
		{
			load();
		}
		mUnusedTimer = 0;
		mBuffer->setPixel(x, y, c);
	}
	
	Color RAMTexture::getInterpolatedPixel(float x, float y)
	{
		if (!mBuffer)
		{
			load();
		}
		mUnusedTimer = 0;
		return mBuffer->getInterpolatedPixel(x, y);
	}
	
	int RAMTexture::getSizeInBytes()
	{
		return (mWidth * mHeight * 3);
	}

}
