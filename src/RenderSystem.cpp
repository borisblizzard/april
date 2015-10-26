/// @file
/// @version 4.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <stdio.h>
#include <algorithm>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include <hltypes/harray.h>
#include <hltypes/hfile.h>
#include <hltypes/hlog.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hresource.h>
#include <hltypes/hstring.h>
#include <hltypes/hthread.h>

#include "april.h"
#include "aprilUtil.h"
#include "Image.h"
#include "RenderSystem.h"
#include "RenderState.h"
#include "PixelShader.h"
#include "Platform.h"
#include "Texture.h"
#include "TextureAsync.h"
#include "VertexShader.h"
#include "Window.h"

namespace april
{
	// optimizations, but they are not thread-safe
	static PlainVertex pv[5];
	static TexturedVertex tv[5];
	
	RenderSystem* rendersys = NULL;

	RenderSystem::DisplayMode::DisplayMode(int width, int height, int refreshRate)
	{
		this->width = width;
		this->height = height;
		this->refreshRate = refreshRate;
	}

	RenderSystem::DisplayMode::~DisplayMode()
	{
	}
	
	bool RenderSystem::DisplayMode::operator==(const DisplayMode& other) const
	{
		return (this->width == other.width && this->height == other.height && this->refreshRate == other.refreshRate);
	}

	bool RenderSystem::DisplayMode::operator!=(const DisplayMode& other) const
	{
		return (this->width != other.width || this->height != other.height || this->refreshRate != other.refreshRate);
	}
	
	hstr RenderSystem::DisplayMode::toString()
	{
		return hsprintf("%dx%d@%dHz", this->width, this->height, this->refreshRate);
	}
	
	RenderSystem::Options::Options()
	{
		this->depthBuffer = false;
	}

	RenderSystem::Options::~Options()
	{
	}

	RenderSystem::Caps::Caps()
	{
		this->maxTextureSize = 0;
		this->npotTexturesLimited = false;
		this->npotTextures = false;
		this->textureFormats += Image::FORMAT_ABGR;
		this->textureFormats += Image::FORMAT_RGBA;
		this->textureFormats += Image::FORMAT_ARGB;
		this->textureFormats += Image::FORMAT_BGRA;
		this->textureFormats += Image::FORMAT_ABGR;
		this->textureFormats += Image::FORMAT_RGBX;
		this->textureFormats += Image::FORMAT_XRGB;
		this->textureFormats += Image::FORMAT_BGRX;
		this->textureFormats += Image::FORMAT_XBGR;
		this->textureFormats += Image::FORMAT_RGB;
		this->textureFormats += Image::FORMAT_BGR;
		this->textureFormats += Image::FORMAT_ALPHA;
		this->textureFormats += Image::FORMAT_GRAYSCALE;
		this->textureFormats += Image::FORMAT_PALETTE;
	}

	RenderSystem::Caps::~Caps()
	{
	}

	hstr RenderSystem::Options::toString()
	{
		harray<hstr> options;
		if (this->depthBuffer)
		{
			options += "depth-buffer";
		}
		if (options.size() == 0)
		{
			options += "none";
		}
		return options.joined(',');
	}
	
	RenderSystem::RenderSystem()
	{
		this->name = "Generic";
		this->created = false;
		this->state = NULL;
		this->deviceState = NULL;
		//this->textureFilter = Texture::FILTER_UNDEFINED;
		//this->textureAddressMode = Texture::ADDRESS_UNDEFINED;
	}
	
	RenderSystem::~RenderSystem()
	{
		this->destroy();
		if (this->state != NULL)
		{
			delete this->state;
		}
		if (this->deviceState != NULL)
		{
			delete this->deviceState;
		}
	}
	
	bool RenderSystem::create(RenderSystem::Options options)
	{
		if (!this->created)
		{
			hlog::writef(logTag, "Creating rendersystem: '%s' (options: %s)", this->name.cStr(), options.toString().cStr());
			this->created = true;
			this->options = options;
			this->state->reset();
			this->deviceState->reset();
			return true;
		}
		this->state->reset();
		this->deviceState->reset();
		return false;
	}
	
	bool RenderSystem::destroy()
	{
		if (this->created)
		{
			hlog::writef(logTag, "Destroying rendersystem '%s'.", this->name.cStr());
			// first wait for queud textures to cancel
			harray<Texture*> textures = this->getTextures();
			if (this->hasAsyncTexturesQueued())
			{
				foreach (Texture*, it, textures)
				{
					if ((*it)->isAsyncLoadQueued())
					{
						(*it)->unload(); // to cancel all async loads
					}
				}
				this->waitForAsyncTextures();
			}
			// creating a copy (again), because deleting a texture modifies this->textures
			textures = this->getTextures();
			foreach (Texture*, it, textures)
			{
				delete (*it);
			}
			this->created = false;
			this->state->reset();
			this->deviceState->reset();
			return true;
		}
		return false;
	}
	
	void RenderSystem::reset()
	{
		hlog::write(logTag, "Resetting rendersystem.");
	}

	harray<Texture*> RenderSystem::getTextures()
	{
		hmutex::ScopeLock lock(&this->texturesMutex);
		return this->textures;
	}

	harray<RenderSystem::DisplayMode> RenderSystem::getSupportedDisplayModes()
	{
		harray<RenderSystem::DisplayMode> result;
		gvec2 resolution = april::getSystemInfo().displayResolution;
		result += RenderSystem::DisplayMode((int)resolution.x, (int)resolution.y, 60);
		return result;
	}

	RenderSystem::Caps RenderSystem::getCaps()
	{
		if (this->caps.maxTextureSize == 0)
		{
			this->_setupCaps();
		}
		return this->caps;
	}
	
	void RenderSystem::setViewport(grect value)
	{
		this->viewport = value;
	}

	void RenderSystem::setDepthBuffer(bool enabled, bool writeEnabled)
	{
		if (this->options.depthBuffer)
		{
			this->state->depthBuffer = enabled;
			this->state->depthBufferWrite = writeEnabled;
		}
		else
		{
			hlog::error(logTag, "Cannot change depth-buffer state, RenderSystem was not created with this option!");
		}
	}

	void RenderSystem::setOrthoProjection(grect rect)
	{
		// TODOaa - change and improve this implementation
		// also: this variable needs to be updated in ::setProjectionMatrix() as well in order to prevent a stale value when using getOrthoProjection()
		this->state->orthoProjection = rect;
		rect -= rect.getSize() * this->getPixelOffset() / april::window->getSize();
		this->state->projectionMatrix.setOrthoProjection(rect);
		this->state->projectionMatrixChanged = true;
	}

	void RenderSystem::setOrthoProjection(grect rect, float nearZ, float farZ)
	{
		// TODOaa - change and improve this implementation
		// also: this variable needs to be updated in ::setProjectionMatrix() as well in order to prevent a stale value when using getOrthoProjection()
		this->state->orthoProjection = rect;
		rect -= rect.getSize() * this->getPixelOffset() / april::window->getSize();
		this->state->projectionMatrix.setOrthoProjection(rect, nearZ, farZ);
		this->state->projectionMatrixChanged = true;
	}

	void RenderSystem::setOrthoProjection(gvec2 size)
	{
		this->setOrthoProjection(grect(0.0f, 0.0f, size));
	}

	void RenderSystem::setOrthoProjection(gvec2 size, float nearZ, float farZ)
	{
		this->setOrthoProjection(grect(0.0f, 0.0f, size), nearZ, farZ);
	}

	void RenderSystem::setModelviewMatrix(gmat4 matrix)
	{
		this->state->modelviewMatrix = matrix;
		this->state->modelviewMatrixChanged = true;
	}
	
	void RenderSystem::setProjectionMatrix(gmat4 matrix)
	{
		this->state->projectionMatrix = matrix;
		this->state->projectionMatrixChanged = true;
	}

	int64_t RenderSystem::getVRamConsumption()
	{
		int64_t result = 0LL;
		harray<Texture*> textures = this->getTextures();
		foreach (Texture*, it, textures)
		{
			result += (int64_t)(*it)->getCurrentVRamSize();
		}
		return result;
	}

	int64_t RenderSystem::getRamConsumption()
	{
		int64_t result = 0LL;
		harray<Texture*> textures = this->getTextures();
		foreach (Texture*, it, textures)
		{
			result += (int64_t)(*it)->getCurrentRamSize();
		}
		return result;
	}

	int64_t RenderSystem::getAsyncRamConsumption()
	{
		int64_t result = 0LL;
		harray<Texture*> textures = this->getTextures();
		foreach (Texture*, it, textures)
		{
			result += (int64_t)(*it)->getCurrentAsyncRamSize();
		}
		return result;
	}

	bool RenderSystem::hasAsyncTexturesQueued()
	{
		return TextureAsync::isRunning();
	}

	Texture* RenderSystem::getRenderTarget()
	{
		hlog::warnf(logTag, "Render targets are not implemented in render system '%s'!", this->name.cStr());
		return NULL;
	}

	void RenderSystem::setRenderTarget(Texture* texture)
	{
		hlog::warnf(logTag, "Render targets are not implemented in render system '%s'!", this->name.cStr());
	}

	void RenderSystem::setPixelShader(april::PixelShader* pixelShader)
	{
		hlog::warnf(logTag, "Pixel shaders are not implemented in render system '%s'!", this->name.cStr());
	}

	void RenderSystem::setVertexShader(april::VertexShader* vertexShader)
	{
		hlog::warnf(logTag, "Vertex shaders are not implemented in render system '%s'!", this->name.cStr());
	}

	void RenderSystem::waitForAsyncTextures(float timeout)
	{
		float time = timeout;
		while ((time > 0.0f || timeout <= 0.0f) && this->hasAsyncTexturesQueued())
		{
			hthread::sleep(0.1f);
			time -= 0.0001f;
			TextureAsync::update();
		}
	}

	Texture* RenderSystem::createTextureFromResource(chstr filename, Texture::Type type, Texture::LoadMode loadMode)
	{
		return this->_createTextureFromSource(true, filename, type, loadMode);
	}

	Texture* RenderSystem::createTextureFromResource(chstr filename, Image::Format format, Texture::Type type, Texture::LoadMode loadMode)
	{
		return this->_createTextureFromSource(true, filename, type, loadMode, format);
	}

	Texture* RenderSystem::createTextureFromFile(chstr filename, Texture::Type type, Texture::LoadMode loadMode)
	{
		return this->_createTextureFromSource(false, filename, type, loadMode);
	}

	Texture* RenderSystem::createTextureFromFile(chstr filename, Image::Format format, Texture::Type type, Texture::LoadMode loadMode)
	{
		return this->_createTextureFromSource(false, filename, type, loadMode, format);
	}

	Texture* RenderSystem::_createTextureFromSource(bool fromResource, chstr filename, Texture::Type type, Texture::LoadMode loadMode, Image::Format format)
	{
		if (format != Image::FORMAT_INVALID && !this->getCaps().textureFormats.has(format))
		{
			hlog::errorf(logTag, "Cannot create texture '%s', the texture format '%d' is not supported!", filename.cStr(), format);
			return NULL;
		}
		hstr name = (fromResource ? this->findTextureResource(filename) : this->findTextureFile(filename));
		if (name == "")
		{
			return NULL;
		}
		Texture* texture = this->_createTexture(fromResource);
		bool result = (format == Image::FORMAT_INVALID ? texture->_create(name, type, loadMode) : texture->_create(name, format, type, loadMode));
		if (result)
		{
			if (loadMode == Texture::LOAD_IMMEDIATE)
			{
				result = texture->load();
			}
			else if (loadMode == Texture::LOAD_ASYNC || loadMode == Texture::LOAD_ASYNC_ON_DEMAND)
			{
				result = texture->loadAsync();
			}
		}
		if (!result)
		{
			delete texture;
			return NULL;
		}
		hmutex::ScopeLock lock(&this->texturesMutex);
		this->textures += texture;
		return texture;
	}

	Texture* RenderSystem::createTexture(int w, int h, unsigned char* data, Image::Format format, Texture::Type type)
	{
		Texture* texture = this->_createTexture(true);
		if (!texture->_create(w, h, data, format, type))
		{
			delete texture;
			return NULL;
		}
		hmutex::ScopeLock lock(&this->texturesMutex);
		this->textures += texture;
		return texture;
	}

	Texture* RenderSystem::createTexture(int w, int h, Color color, Image::Format format, Texture::Type type)
	{
		Texture* texture = this->_createTexture(true);
		if (!texture->_create(w, h, color, format, type))
		{
			delete texture;
			return NULL;
		}
		hmutex::ScopeLock lock(&this->texturesMutex);
		this->textures += texture;
		return texture;
	}

	void RenderSystem::destroyTexture(Texture* texture)
	{
		texture->unload();
		texture->waitForAsyncLoad(); // waiting for all async stuff to finish
		hmutex::ScopeLock lock(&this->texturesMutex);
		this->textures -= texture;
		lock.release();
		delete texture;
	}

	PixelShader* RenderSystem::createPixelShaderFromResource(chstr filename)
	{
		return this->_createPixelShaderFromSource(true, filename);
	}

	PixelShader* RenderSystem::createPixelShaderFromFile(chstr filename)
	{
		return this->_createPixelShaderFromSource(false, filename);
	}

	PixelShader* RenderSystem::createPixelShader()
	{
		return this->_createPixelShader();
	}

	VertexShader* RenderSystem::createVertexShaderFromResource(chstr filename)
	{
		return this->_createVertexShaderFromSource(true, filename);
	}

	VertexShader* RenderSystem::createVertexShaderFromFile(chstr filename)
	{
		return this->_createVertexShaderFromSource(false, filename);
	}

	VertexShader* RenderSystem::createVertexShader()
	{
		return this->_createVertexShader();
	}

	PixelShader* RenderSystem::_createPixelShaderFromSource(bool fromResource, chstr filename)
	{
		PixelShader* shader = this->_createPixelShader();
		if (shader != NULL)
		{
			bool loaded = (fromResource ? shader->loadResource(filename) : shader->loadFile(filename));
			if (!loaded)
			{
				delete shader;
				shader = NULL;
			}
		}
		return shader;
	}

	VertexShader* RenderSystem::_createVertexShaderFromSource(bool fromResource, chstr filename)
	{
		VertexShader* shader = this->_createVertexShader();
		if (shader != NULL)
		{
			bool loaded = (fromResource ? shader->loadResource(filename) : shader->loadFile(filename));
			if (!loaded)
			{
				delete shader;
				shader = NULL;
			}
		}
		return shader;
	}

	PixelShader* RenderSystem::_createPixelShader()
	{
		hlog::warnf(logTag, "Pixel shaders are not implemented in render system '%s'!", this->name.cStr());
		return NULL;
	}

	VertexShader* RenderSystem::_createVertexShader()
	{
		hlog::warnf(logTag, "Vertex shaders are not implemented in render system '%s'!", this->name.cStr());
		return NULL;
	}

	void RenderSystem::destroyPixelShader(PixelShader* shader)
	{
		delete shader;
	}

	void RenderSystem::destroyVertexShader(VertexShader* shader)
	{
		delete shader;
	}

	void RenderSystem::unloadTextures()
	{
		harray<Texture*> textures = this->getTextures();
		foreach (Texture*, it, textures)
		{
			(*it)->unload();
		}
	}

	void RenderSystem::_setResolution(int w, int h, bool fullscreen)
	{
		hlog::warnf(logTag, "Changing resolutions is not implemented in render system '%s'!", this->name.cStr());
	}

	void RenderSystem::setIdentityTransform()
	{
		this->state->modelviewMatrix.setIdentity();
		this->state->modelviewMatrixChanged = true;
	}
	
	void RenderSystem::translate(float x, float y, float z)
	{
		this->state->modelviewMatrix.translate(x, y, z);
		this->state->modelviewMatrixChanged = true;
	}
	
	void RenderSystem::rotate(float angle, float ax, float ay, float az)
	{
		this->state->modelviewMatrix.rotate(ax, ay, az, angle);
		this->state->modelviewMatrixChanged = true;
	}	
	
	void RenderSystem::scale(float s)
	{
		this->state->modelviewMatrix.scale(s);
		this->state->modelviewMatrixChanged = true;
	}
	
	void RenderSystem::scale(float sx, float sy, float sz)
	{
		this->state->modelviewMatrix.scale(sx, sy, sz);
		this->state->modelviewMatrixChanged = true;
	}
	
	void RenderSystem::lookAt(const gvec3& eye, const gvec3& target, const gvec3& up)
	{
		this->state->modelviewMatrix.lookAt(eye, target, up);
		this->state->modelviewMatrixChanged = true;
	}
		
	void RenderSystem::setPerspective(float fov, float aspect, float nearClip, float farClip)
	{
		this->state->projectionMatrix.setPerspective(fov, aspect, nearClip, farClip);
		this->state->projectionMatrixChanged = true;
	}





	void RenderSystem::_updateDeviceState(bool forceUpdate)
	{
		if (forceUpdate || this->state->modelviewMatrixChanged)
		{
			if (forceUpdate || this->deviceState->modelviewMatrix != this->state->modelviewMatrix)
			{
				this->_setDeviceModelviewMatrix(this->state->modelviewMatrix);
				this->deviceState->modelviewMatrix = this->state->modelviewMatrix;
			}
			this->state->modelviewMatrixChanged = false;
		}
		if (forceUpdate || this->state->projectionMatrixChanged)
		{
			if (forceUpdate || this->deviceState->projectionMatrix != this->state->projectionMatrix)
			{
				this->_setDeviceProjectionMatrix(this->state->projectionMatrix);
				this->deviceState->projectionMatrix = this->state->projectionMatrix;
			}
			this->state->projectionMatrixChanged = false;
		}
		if (forceUpdate || this->deviceState->depthBuffer != this->state->depthBuffer || this->deviceState->depthBufferWrite != this->state->depthBufferWrite)
		{
			this->_setDeviceDepthBuffer(this->state->depthBuffer, this->state->depthBufferWrite);
			this->deviceState->depthBuffer = this->state->depthBuffer;
			this->deviceState->depthBufferWrite = this->state->depthBufferWrite;
		}
		this->deviceState->update();
	}

	void RenderSystem::render(RenderOperation renderOperation, PlainVertex* v, int nVertices)
	{
		this->_updateDeviceState();
		this->_render(renderOperation, v, nVertices);
	}

	void RenderSystem::render(RenderOperation renderOperation, PlainVertex* v, int nVertices, Color color)
	{
		this->_updateDeviceState();
		this->_render(renderOperation, v, nVertices, color);
	}

	void RenderSystem::render(RenderOperation renderOperation, TexturedVertex* v, int nVertices)
	{
		this->_updateDeviceState();
		this->_render(renderOperation, v, nVertices);
	}

	void RenderSystem::render(RenderOperation renderOperation, TexturedVertex* v, int nVertices, Color color)
	{
		this->_updateDeviceState();
		this->_render(renderOperation, v, nVertices, color);
	}

	void RenderSystem::render(RenderOperation renderOperation, ColoredVertex* v, int nVertices)
	{
		this->_updateDeviceState();
		this->_render(renderOperation, v, nVertices);
	}

	void RenderSystem::render(RenderOperation renderOperation, ColoredTexturedVertex* v, int nVertices)
	{
		this->_updateDeviceState();
		this->_render(renderOperation, v, nVertices);
	}

	void RenderSystem::drawRect(grect rect, Color color)
	{
		pv[0].x = rect.x;			pv[0].y = rect.y;			pv[0].z = 0.0f;
		pv[1].x = rect.x + rect.w;	pv[1].y = rect.y;			pv[1].z = 0.0f;
		pv[2].x = rect.x + rect.w;	pv[2].y = rect.y + rect.h;	pv[2].z = 0.0f;
		pv[3].x = rect.x;			pv[3].y = rect.y + rect.h;	pv[3].z = 0.0f;
		pv[4].x = rect.x;			pv[4].y = rect.y;			pv[4].z = 0.0f;
		this->render(RO_LINE_STRIP, pv, 5, color);
	}

	void RenderSystem::drawFilledRect(grect rect, Color color)
	{
		pv[0].x = rect.x;			pv[0].y = rect.y;			pv[0].z = 0.0f;
		pv[1].x = rect.x + rect.w;	pv[1].y = rect.y;			pv[1].z = 0.0f;
		pv[2].x = rect.x;			pv[2].y = rect.y + rect.h;	pv[2].z = 0.0f;
		pv[3].x = rect.x + rect.w;	pv[3].y = rect.y + rect.h;	pv[3].z = 0.0f;
		this->render(RO_TRIANGLE_STRIP, pv, 4, color);
	}
	
	void RenderSystem::drawTexturedRect(grect rect, grect src)
	{
		tv[0].x = rect.x;			tv[0].y = rect.y;			tv[0].z = 0.0f;	tv[0].u = src.x;			tv[0].v = src.y;
		tv[1].x = rect.x + rect.w;	tv[1].y = rect.y;			tv[1].z = 0.0f;	tv[1].u = src.x + src.w;	tv[1].v = src.y;
		tv[2].x = rect.x;			tv[2].y = rect.y + rect.h;	tv[2].z = 0.0f;	tv[2].u = src.x;			tv[2].v = src.y + src.h;
		tv[3].x = rect.x + rect.w;	tv[3].y = rect.y + rect.h;	tv[3].z = 0.0f;	tv[3].u = src.x + src.w;	tv[3].v = src.y + src.h;
		this->render(RO_TRIANGLE_STRIP, tv, 4);
	}
	
	void RenderSystem::drawTexturedRect(grect rect, grect src, Color color)
	{
		tv[0].x = rect.x;			tv[0].y = rect.y;			tv[0].z = 0.0f;	tv[0].u = src.x;			tv[0].v = src.y;
		tv[1].x = rect.x + rect.w;	tv[1].y = rect.y;			tv[1].z = 0.0f;	tv[1].u = src.x + src.w;	tv[1].v = src.y;
		tv[2].x = rect.x;			tv[2].y = rect.y + rect.h;	tv[2].z = 0.0f;	tv[2].u = src.x;			tv[2].v = src.y + src.h;
		tv[3].x = rect.x + rect.w;	tv[3].y = rect.y + rect.h;	tv[3].z = 0.0f;	tv[3].u = src.x + src.w;	tv[3].v = src.y + src.h;
		this->render(RO_TRIANGLE_STRIP, tv, 4, color);
	}

	void RenderSystem::presentFrame()
	{
		april::window->presentFrame();
	}
	
	hstr RenderSystem::findTextureResource(chstr filename)
	{
		if (hresource::exists(filename))
		{
			return filename;
		}
		hstr name;
		harray<hstr> extensions = april::getTextureExtensions();
		foreach (hstr, it, extensions)
		{
			name = filename + (*it);
			if (hresource::exists(name))
			{
				return name;
			}
		}
		hstr noExtensionName = hfile::withoutExtension(filename);
		if (noExtensionName != filename)
		{
			foreach (hstr, it, extensions)
			{
				name = noExtensionName + (*it);
				if (hresource::exists(name))
				{
					return name;
				}
			}
		}
		return "";
	}
	
	hstr RenderSystem::findTextureFile(chstr filename)
	{
		if (hfile::exists(filename))
		{
			return filename;
		}
		hstr name;
		harray<hstr> extensions = april::getTextureExtensions();
		foreach (hstr, it, extensions)
		{
			name = filename + (*it);
			if (hfile::exists(name))
			{
				return name;
			}
		}
		hstr noExtensionName = hfile::withoutExtension(filename);
		if (noExtensionName != filename)
		{
			foreach (hstr, it, extensions)
			{
				name = noExtensionName + (*it);
				if (hfile::exists(name))
				{
					return name;
				}
			}
		}
		return "";
	}
	
	april::Image* RenderSystem::takeScreenshot(Image::Format format)
	{
		hlog::warnf(logTag, "Screenshots are not implemented in render system '%s'!", this->name.cStr());
		return NULL;
	}

	unsigned int RenderSystem::_numPrimitives(RenderOperation renderOperation, int nVertices)
	{
		switch (renderOperation)
		{
		case RO_TRIANGLE_LIST:	return nVertices / 3;
		case RO_TRIANGLE_STRIP:	return nVertices - 2;
		case RO_TRIANGLE_FAN:	return nVertices - 2;
		case RO_LINE_LIST:		return nVertices / 2;
		case RO_LINE_STRIP:		return nVertices - 1;
		case RO_POINT_LIST:		return nVertices;
		default:				break;
		}
		return 0;
	}
	
	unsigned int RenderSystem::_limitPrimitives(RenderOperation renderOperation, int nVertices)
	{
		switch (renderOperation)
		{
		case RO_TRIANGLE_LIST:	return nVertices / 3 * 3;
		case RO_TRIANGLE_STRIP:	return nVertices;
		case RO_TRIANGLE_FAN:	return nVertices;
		case RO_LINE_LIST:		return nVertices / 2 * 2;
		case RO_LINE_STRIP:		return nVertices;
		case RO_POINT_LIST:		return nVertices;
		default:				break;
		}
		return nVertices;
	}
	
}
