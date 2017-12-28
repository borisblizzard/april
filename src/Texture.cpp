/// @file
/// @version 5.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <hltypes/harray.h>
#include <hltypes/hfile.h>
#include <hltypes/hlog.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hresource.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "Color.h"
#include "Image.h"
#include "Texture.h"
#include "TextureAsync.h"
#include "RenderSystem.h"

#define HROUND_GRECT(rect) hround(rect.x), hround(rect.y), hround(rect.w), hround(rect.h)
#define HROUND_GVEC2(vec2) hround(vec2.x), hround(vec2.y)

namespace april
{
	HL_ENUM_CLASS_DEFINE(Texture::Type,
	(
		HL_ENUM_DEFINE(Texture::Type, Managed);
		HL_ENUM_DEFINE(Texture::Type, Immutable);
		HL_ENUM_DEFINE(Texture::Type, Volatile);
		HL_ENUM_DEFINE(Texture::Type, RenderTarget);
	));

	HL_ENUM_CLASS_DEFINE(Texture::Filter,
	(
		HL_ENUM_DEFINE(Texture::Filter, Nearest);
		HL_ENUM_DEFINE(Texture::Filter, Linear);
	));

	HL_ENUM_CLASS_DEFINE(Texture::AddressMode,
	(
		HL_ENUM_DEFINE(Texture::AddressMode, Wrap);
		HL_ENUM_DEFINE(Texture::AddressMode, Clamp);
	));

	HL_ENUM_CLASS_DEFINE(Texture::LoadMode,
	(
		HL_ENUM_DEFINE(Texture::LoadMode, Immediate);
		HL_ENUM_DEFINE(Texture::LoadMode, OnDemand);
		HL_ENUM_DEFINE(Texture::LoadMode, Async);
		HL_ENUM_DEFINE(Texture::LoadMode, AsyncDeferredUpload);
	));

	Texture::Lock::Lock()
	{
		this->systemBuffer = NULL;
		this->x = 0;
		this->y = 0;
		this->w = 0;
		this->h = 0;
		this->dx = 0;
		this->dy = 0;
		this->data = NULL;
		this->dataWidth = 0;
		this->dataHeight = 0;
		this->format = Image::Format::Invalid;
		this->locked = false;
		this->failed = true;
		this->renderTarget = false;
	}

	Texture::Lock::~Lock()
	{
	}

	void Texture::Lock::activateFail()
	{
		this->locked = false;
		this->failed = true;
		this->renderTarget = false;
	}

	void Texture::Lock::activateLock(int x, int y, int w, int h, int dx, int dy, unsigned char* data, int dataWidth, int dataHeight, Image::Format format)
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
		this->dx = dx;
		this->dy = dy;
		this->data = data;
		this->dataWidth = dataWidth;
		this->dataHeight = dataHeight;
		this->format = format;
		this->locked = true;
		this->failed = false;
		this->renderTarget = false;
	}

	void Texture::Lock::activateRenderTarget(int x, int y, int w, int h, int dx, int dy, unsigned char* data, int dataWidth, int dataHeight, Image::Format format)
	{
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
		this->dx = dx;
		this->dy = dy;
		this->data = data;
		this->dataWidth = dataWidth;
		this->dataHeight = dataHeight;
		this->format = format;
		this->locked = true;
		this->failed = false;
		this->renderTarget = true;
	}

	Texture::Texture(bool fromResource)
	{
		this->filename = "";
		this->type = Type::Immutable;
		this->loaded = false;
		this->loadMode = LoadMode::Immediate;
		this->format = Image::Format::Invalid;
		this->dataFormat = 0;
		this->width = 0;
		this->height = 0;
		this->effectiveWidth = 1.0f; // used only with software NPOT textures
		this->effectiveHeight = 1.0f; // used only with software NPOT textures
		this->compressedSize = 0; // used in compressed textures only
		this->filter = Filter::Linear;
		this->addressMode = AddressMode::Clamp;
		this->locked = false;
		this->dirty = false;
		this->data = NULL;
		this->dataAsync = NULL;
		this->asyncLoadQueued = false;
		this->asyncLoadDiscarded = false;
		this->fromResource = fromResource;
		this->firstUpload = true;
	}

	bool Texture::_create(chstr filename, Texture::Type type, Texture::LoadMode loadMode)
	{
		this->filename = filename;
		this->type = type;
		this->width = 0;
		this->height = 0;
		this->type = type;
		this->loadMode = loadMode;
		this->format = Image::Format::Invalid;
		this->dataFormat = 0;
		this->data = NULL;
		this->dataAsync = NULL;
		this->asyncLoadQueued = false;
		hlog::write(logTag, "Registering texture: " + this->_getInternalName());
		return true;
	}

	bool Texture::_create(chstr filename, Image::Format format, Texture::Type type, Texture::LoadMode loadMode)
	{
		this->filename = filename;
		this->type = type;
		this->width = 0;
		this->height = 0;
		this->type = type;
		this->loadMode = loadMode;
		this->format = format;
		this->dataFormat = 0;
		this->data = NULL;
		this->dataAsync = NULL;
		this->asyncLoadQueued = false;
		hlog::write(logTag, "Registering texture: " + this->_getInternalName());
		return true;
	}

	bool Texture::_create(int w, int h, unsigned char* data, Image::Format format, Texture::Type type)
	{
		if (w == 0 || h == 0)
		{
			hlog::errorf(logTag, "Cannot create texture with dimensions %d,%d!", w, h);
			return false;
		}
		this->filename = "";
		this->width = w;
		this->height = h;
		this->type = Type::Volatile; // so the write() call later on goes through
		this->loadMode = LoadMode::Immediate;
		int size = 0;
		if (type != Type::Volatile && type != Type::RenderTarget)
		{
			this->format = format;
			size = this->getByteSize();
			this->data = new unsigned char[size];
			this->type = Type::Managed;
		}
		else
		{
			this->format = april::rendersys->getNativeTextureFormat(format);
			size = this->getByteSize();
			this->type = type;
		}
		this->dataAsync = NULL;
		this->asyncLoadQueued = false;
		hlog::write(logTag, "Registering texture: " + this->_getInternalName());
		return this->loadAsync();
	}

	bool Texture::_create(int w, int h, const Color& color, Image::Format format, Texture::Type type)
	{
		if (w == 0 || h == 0)
		{
			hlog::errorf(logTag, "Cannot create texture with dimensions %d,%d!", w, h);
			return false;
		}
		this->filename = "";
		this->width = w;
		this->height = h;
		this->type = Type::Volatile; // so the fillRect() call later on goes through
		this->loadMode = LoadMode::Immediate;
		int size = 0;
		if (type != Type::Volatile && type != Type::RenderTarget)
		{
			this->format = format;
			size = this->getByteSize();
			this->data = new unsigned char[size];
			Image::fillRect(0, 0, this->width, this->height, color, this->data, this->width, this->height, this->format);
			this->type = Type::Managed;
		}
		else
		{
			this->format = april::rendersys->getNativeTextureFormat(format);
			size = this->getByteSize();
			this->type = type;
		}
		this->dataAsync = NULL;
		this->asyncLoadQueued = false;
		hlog::write(logTag, "Registering texture: " + this->_getInternalName());
		int maxTextureSize = april::rendersys->getCaps().maxTextureSize;
		if (maxTextureSize > 0 && (this->width > maxTextureSize || this->height > maxTextureSize))
		{
			hlog::warnf(logTag, "Texture size for '%s' is %d,%d while the reported system max texture size is %d!", this->_getInternalName().cStr(), this->width, this->height, maxTextureSize);
		}
		return this->loadAsync();
	}

	Texture::~Texture()
	{
		if (this->data != NULL)
		{
			delete[] this->data;
		}
		this->asyncLoadQueued = false;
		this->asyncLoadDiscarded = false;
		if (this->dataAsync != NULL)
		{
			delete[] this->dataAsync;
		}
	}

	int Texture::getWidth() const
	{
		if (this->width == 0)
		{
			hlog::warnf(logTag, "Texture '%s' has width = 0 (possibly not loaded yet?)", this->filename.cStr());
		}
		return this->width;
	}

	int Texture::getHeight() const
	{
		if (this->height == 0)
		{
			hlog::warnf(logTag, "Texture '%s' has height = 0 (possibly not loaded yet?)", this->filename.cStr());
		}
		return this->height;
	}

	int Texture::getBpp() const
	{
		if (this->format == Image::Format::Invalid)
		{
			hlog::warnf(logTag, "Texture '%s' has bpp = 0 (possibly not loaded yet?)", this->filename.cStr());
		}
		return this->format.getBpp();
	}

	int Texture::getByteSize() const
	{
		if (this->width == 0 || this->height == 0 || this->format == Image::Format::Invalid)
		{
			hlog::warnf(logTag, "Texture '%s' has byteSize = 0 (possibly not loaded yet?)", this->filename.cStr());
		}
		if (this->compressedSize > 0)
		{
			return this->compressedSize;
		}
		return (this->width * this->height * this->format.getBpp());
	}

	int Texture::getCurrentVRamSize()
	{
		if (this->width == 0 || this->height == 0 || this->format == Image::Format::Invalid || !this->isLoaded())
		{
			return 0;
		}
		if (this->compressedSize > 0)
		{
			return this->compressedSize;
		}
		return (this->width * this->height * this->format.getBpp());
	}

	int Texture::getCurrentRamSize()
	{
		if (this->type == Type::Immutable || this->type == Type::Volatile || this->type == Type::RenderTarget)
		{
			return 0;
		}
		return this->getCurrentVRamSize();
	}

	int Texture::getCurrentAsyncRamSize()
	{
		if (!this->isLoadedAsync())
		{
			return 0;
		}
		if (this->width == 0 || this->height == 0 || this->format == Image::Format::Invalid)
		{
			return 0;
		}
		if (this->compressedSize > 0)
		{
			return this->compressedSize;
		}
		return (this->width * this->height * this->format.getBpp());
	}

	bool Texture::isLoaded()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		return this->loaded;
	}

	bool Texture::isLoadedAsync()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		return (!this->asyncLoadQueued && (this->dataAsync != NULL || (this->filename == "" && this->data != NULL)) && !this->loaded);
	}

	bool Texture::isAsyncLoadQueued()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		return this->asyncLoadQueued;
	}

	bool Texture::isLoadedAny()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		return (this->loaded || this->asyncLoadQueued || this->dataAsync != NULL);
	}

	bool Texture::_isReadable() const
	{
		return (this->type == Type::Managed || this->type == Type::RenderTarget);
	}

	bool Texture::_isWritable() const
	{
		return (this->type != Type::Immutable);
	}

	bool Texture::_isAlterable() const
	{
		return (this->_isReadable() && this->_isWritable());
	}

	hstr Texture::_getInternalName() const
	{
		hstr result;
		if (this->filename != "")
		{
			result += "'" + this->filename + "'";
		}
		else
		{
#ifdef _WIN32
			result += hsprintf("<0x%p>", this);
#else
			result += hsprintf("<%p>", this); // on Unix %p adds the 0x
#endif
		}
		result += " (" + this->type.getName() + ")";
		return result;
	}

	bool Texture::load()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		if (this->loaded)
		{
			return true;
		}
		this->asyncLoadDiscarded = false; // a possible previous unload call must be canceled
		if (this->asyncLoadQueued)
		{
			lock.release();
			this->waitForAsyncLoad();
			return true; // will already call this method again through TextureAsync::update() so it does not need to continue
		}
		lock.release();
		this->loadMetaData();
		return this->loadAsync();
	}

	bool Texture::upload()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		if (this->loaded)
		{
			lock.release();
			this->unlock();
			return true;
		}
		this->asyncLoadDiscarded = false; // a possible previous unload call must be canceled
		if (this->asyncLoadQueued)
		{
			// TODOx - this could be a hazard for deadlocks
			lock.release();
			this->waitForAsyncLoad();
			return true; // will already call this method again through TextureAsync::update() so it does not need to continue
		}
		int size = 0;
		unsigned char* currentData = NULL;
		// no lock required since it only checks for existence, not for manipulation of data
		if (this->data != NULL) // reload from memory
		{
			hlog::write(logTag, "Uploading texture: " + this->_getInternalName());
			currentData = this->data;
			size = this->getByteSize();
		}
		else if (this->dataAsync != NULL) // load from asynchronically loaded data
		{
			hlog::write(logTag, "Uploading async texture: " + this->_getInternalName());
			currentData = this->dataAsync;
			size = this->getByteSize();
		}
		else
		{
			hlog::write(logTag, "Uploading texture: " + this->_getInternalName());
		}
		lock.release();
		// if no cached data and not a volatile texture that was previously loaded and thus has a width and height
		if (currentData == NULL && ((this->type != Type::Volatile && this->type != Type::RenderTarget) || this->width == 0 || this->height == 0))
		{
			if (this->filename == "")
			{
				hlog::error(logTag, "No filename for texture specified!");
				return false;
			}
			// must not call createFromResource() or createFromFile() that converts automatically, because _processImageFormatSupport() needs to be called first
			Image* image = (this->fromResource ? Image::createFromResource(this->filename) : Image::createFromFile(this->filename));
			if (image != NULL)
			{
				image = this->_processImageFormatSupport(image);
			}
			if (image != NULL && this->format != Image::Format::Invalid && Image::needsConversion(image->format, this->format))
			{
				unsigned char* data = NULL;
				if (Image::convertToFormat(image->w, image->h, image->data, image->format, &data, this->format))
				{
					delete[] image->data;
					image->format = this->format;
					image->data = data;
				}
			}
			if (image == NULL)
			{
				hlog::error(logTag, "Failed to load texture: " + this->_getInternalName());
				return false;
			}
			this->width = image->w;
			this->height = image->h;
			this->format = image->format;
			this->dataFormat = image->internalFormat;
			if (this->dataFormat != 0)
			{
				size = image->compressedSize;
				this->compressedSize = size;
			}
			int maxTextureSize = april::rendersys->getCaps().maxTextureSize;
			if (maxTextureSize > 0 && (this->width > maxTextureSize || this->height > maxTextureSize))
			{
				hlog::warnf(logTag, "Texture size for '%s' is %d,%d while the reported system max texture size is %d!", this->_getInternalName().cStr(), this->width, this->height, maxTextureSize);
			}
			currentData = image->data;
			image->data = NULL;
			delete image;
		}
		this->_assignFormat();
		bool result = this->_deviceCreateTexture(currentData, size, this->type);
		if (!result)
		{
			// no lock required since it only checks for existence, not for manipulation of data
			if (currentData != NULL && this->data != currentData)
			{
				delete[] currentData;
			}
			lock.acquire(&this->asyncLoadMutex);
			this->dataAsync = NULL; // not needed anymore and makes isLoadedAsync() return false now
			this->loaded = result;
			return false;
		}
		if (currentData != NULL)
		{
			lock.acquire(&this->asyncDataMutex);
			this->dirty = false;
			if (this->firstUpload)
			{
				if (Image::needsConversion(this->format, april::rendersys->getNativeTextureFormat(this->format)) ||
					!this->_uploadToGpu(0, 0, this->width, this->height, 0, 0, currentData, this->width, this->height, format))
				{
					Lock lock = this->_tryLockSystem(0, 0, this->width, this->height);
					if (!lock.failed)
					{
						Image::write(0, 0, this->width, this->height, 0, 0, currentData, this->width, this->height, format, lock.data, lock.dataWidth, lock.dataHeight, lock.format);
						this->_unlockSystem(lock, true);
					}
				}
			}
			// TODOx - remove condition
			if (/*this->type != Type::Volatile && this->type != Type::RenderTarget &&*/ (this->type != Type::Immutable || this->filename == ""))
			{
				if (this->data != currentData)
				{
					if (this->data != NULL)
					{
						delete[] this->data;
					}
					this->data = currentData;
				}
			}
			else
			{
				delete[] currentData;
				// the used format will be the native format, because there is no intermediate data
				this->format = april::rendersys->getNativeTextureFormat(this->format);
			}
			lock.release();
		}
		// TODOx - remove this code?
		/*
		else if (this->type == Type::Volatile) // when recreating a texture, it is important that it is created empty to avoid problems (e.g. DX9 creates a white initial texture)
		{
			this->_rawClear();
		}
		*/
		this->unlock(); // upload any additional changes
		lock.acquire(&this->asyncLoadMutex);
		this->dataAsync = NULL; // not needed anymore and makes isLoadedAsync() return false now
		this->loaded = result;
		return true;
	}

	bool Texture::loadAsync()
	{
		this->unlock();
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		if (this->dataAsync != NULL || this->loaded)
		{
			return false;
		}
		// TODOx - remove this?
		/*
		if (this->data != NULL || ((this->type == Type::Volatile || this->type == Type::RenderTarget) && this->width > 0 && this->height > 0))
		{
			hstr error = "This texture type does not support async loading! texture: '" + this->_getInternalName() + "', reasons:";
			if (this->data != NULL)
			{
				error += "\ndata isn't NULL.";
			}
			if (this->type == Type::Volatile)
			{
				error += "\ntype is 'volatile'.";
			}
			if (this->type == Type::RenderTarget)
			{
				error += "\ntype is 'render target'.";
			}
			if (this->width > 0)
			{
				error += "\nwidth is larger than 0.";
			}
			if (this->height > 0)
			{
				error += "\nheight is larger than 0.";
			}
			hlog::warn(logTag, error);
			return false;
		}
		if (this->filename == "")
		{
			hlog::error(logTag, "No filename for texture specified!");
			return false;
		}
		*/
		this->asyncLoadDiscarded = false;
		if (!this->asyncLoadQueued) // this check is down here to allow the upper error messages to be displayed
		{
			if (this->filename == "")
			{
				this->_assignFormat();
			}
			this->asyncLoadQueued = TextureAsync::queueLoad(this);
		}
		return this->asyncLoadQueued;
	}

	void Texture::unload()
	{
		if (this->_deviceDestroyTexture())
		{
			hlog::write(logTag, "Unloading texture: " + this->_getInternalName());
		}
		hmutex::ScopeLock lockData(&this->asyncDataMutex);
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		this->loaded = false;
		if (this->asyncLoadQueued)
		{
			this->asyncLoadDiscarded = true;
		}
		if (this->dataAsync != NULL)
		{
			delete[] this->dataAsync;
			this->dataAsync = NULL;
		}
		this->firstUpload = true;
		this->dirty = false;
	}

	bool Texture::loadMetaData()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		hmutex::ScopeLock lockData(&this->asyncDataMutex);
		if (this->loaded)
		{
			return true;
		}
		bool hasData = (this->data != NULL || this->dataAsync != NULL);
		lockData.release();
		lock.release();
		if (!hasData && this->type != Type::Volatile && this->type != Type::RenderTarget && (this->width == 0 || this->height == 0))
		{
			if (this->filename == "")
			{
				hlog::error(logTag, "No filename for texture specified!");
				return false;
			}
			Image* image = (this->fromResource ? Image::readMetaDataFromResource(this->filename) : Image::readMetaDataFromFile(this->filename));
			if (image != NULL)
			{
				image = this->_processImageFormatSupport(image);
			}
			if (image == NULL)
			{
				hlog::error(logTag, "Failed to load texture: " + this->_getInternalName());
				return false;
			}
			this->width = image->w;
			this->height = image->h;
			this->format = image->format;
			this->dataFormat = image->internalFormat;
			if (this->dataFormat != 0)
			{
				this->compressedSize = image->compressedSize;
			}
			delete image;
		}
		return true;
	}

	bool Texture::ensureLoaded()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		if (this->loaded)
		{
			return true;
		}
		if (this->asyncLoadQueued || this->dataAsync != NULL || (this->filename == "" && this->data != NULL))
		{
			lock.release();
			TextureAsync::prioritizeLoad(this);
			while (true)
			{
				lock.acquire(&this->asyncLoadMutex);
				if (this->loaded)
				{
					return true;
				}
				lock.release();
				hthread::sleep(0.001f);
			}
		}
		return false;
	}

	bool Texture::_ensureInternalLoaded()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		if (this->loaded)
		{
			return true;
		}
		if (this->asyncLoadQueued || this->dataAsync != NULL || (this->filename == "" && this->data != NULL))
		{
			lock.release();
			TextureAsync::prioritizeLoad(this);
			while (true)
			{
				TextureAsync::update();
				lock.acquire(&this->asyncLoadMutex);
				if (this->loaded)
				{
					return true;
				}
				lock.release();
			}
		}
		return false;
	}

	void Texture::waitForAsyncLoad(float timeout)
	{
		TextureAsync::prioritizeLoad(this);
		float time = timeout;
		hmutex::ScopeLock lock;
		while (time > 0.0f || timeout <= 0.0f)
		{
			lock.acquire(&this->asyncLoadMutex);
			if (!this->asyncLoadQueued)
			{
				break;
			}
			lock.release();
			hthread::sleep(0.001f);
			time -= 0.000001f;
		}
	}

	hstream* Texture::_prepareAsyncStream()
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		if (!this->asyncLoadQueued || this->asyncLoadDiscarded)
		{
			this->asyncLoadQueued = false;
			this->asyncLoadDiscarded = false;
			return NULL;
		}
		lock.release();
		hstream* stream = NULL;
		if (this->filename != "")
		{
			stream = new hstream();
			if (this->fromResource)
			{
				hresource file;
				file.open(this->filename);
				stream->writeRaw(file);
			}
			else
			{
				hfile file;
				file.open(this->filename);
				stream->writeRaw(file);
			}
			stream->rewind();
		}
		lock.acquire(&this->asyncLoadMutex);
		if (!this->asyncLoadQueued || this->asyncLoadDiscarded)
		{
			this->asyncLoadQueued = false;
			this->asyncLoadDiscarded = false;
			if (stream != NULL)
			{
				delete stream;
			}
			return NULL;
		}
		return stream;
	}

	void Texture::_decodeFromAsyncStream(hstream* stream)
	{
		hmutex::ScopeLock lock(&this->asyncLoadMutex);
		if (!this->asyncLoadQueued || this->asyncLoadDiscarded || this->dataAsync != NULL || this->loaded)
		{
			this->asyncLoadQueued = false;
			this->asyncLoadDiscarded = false;
			return;
		}
		lock.release();
		hlog::write(logTag, "Loading async texture: " + this->_getInternalName());
		if (stream == NULL) // uses data in RAM
		{
			lock.acquire(&this->asyncLoadMutex);
			if (this->asyncLoadQueued && !this->asyncLoadDiscarded)
			{
				this->_assignFormat();
			}
			this->asyncLoadQueued = false;
			this->asyncLoadDiscarded = false;
			return;
		}
		// must not call createFromStream() that converts automatically, because _processImageFormatSupport() needs to be called first
		Image* image = Image::createFromStream(*(hsbase*)stream, "." + hfile::extensionOf(this->filename));
		if (image != NULL)
		{
			image = this->_processImageFormatSupport(image);
		}
		if (image != NULL && this->format != Image::Format::Invalid && Image::needsConversion(image->format, this->format))
		{
			unsigned char* data = NULL;
			if (Image::convertToFormat(image->w, image->h, image->data, image->format, &data, this->format))
			{
				delete[] image->data;
				image->format = this->format;
				image->data = data;
			}
		}
		if (image == NULL)
		{
			hlog::error(logTag, "Failed to load async texture: " + this->_getInternalName());
			lock.acquire(&this->asyncLoadMutex);
			this->asyncLoadQueued = false;
			this->asyncLoadDiscarded = false;
			return;
		}
		this->width = image->w;
		this->height = image->h;
		this->format = image->format;
		this->dataFormat = image->internalFormat;
		if (this->dataFormat != 0)
		{
			this->compressedSize = image->compressedSize;
		}
		int maxTextureSize = april::rendersys->getCaps().maxTextureSize;
		if (maxTextureSize > 0 && (this->width > maxTextureSize || this->height > maxTextureSize))
		{
			hlog::warnf(logTag, "Texture size for '%s' is %d,%d while the reported system max texture size is %d!", this->_getInternalName().cStr(), this->width, this->height, maxTextureSize);
		}
		lock.acquire(&this->asyncLoadMutex);
		if (this->asyncLoadQueued && !this->asyncLoadDiscarded)
		{
			this->_assignFormat();
			this->dataAsync = image->data;
			image->data = NULL;
		}
		this->asyncLoadQueued = false;
		this->asyncLoadDiscarded = false;
		delete image;
	}

	Image* Texture::_processImageFormatSupport(Image* image)
	{
		if (!april::rendersys->getCaps().textureFormats.has(image->format))
		{
			hlog::warn(logTag, "Texture format not supported, trying to convert to an RGBA format: " + this->_getInternalName());
			Image::Format nativeFormat = april::rendersys->getNativeTextureFormat(Image::Format::RGBA);
			if (image->data != NULL)
			{
				bool result = false;
				Image* newImage = NULL;
				if (image->format == Image::Format::Alpha)
				{
					newImage = Image::create(image->w, image->h, april::Color::White, nativeFormat);
					result = newImage->insertAlphaMap(image);
				}
				else
				{
					newImage = Image::create(image->w, image->h, april::Color::Clear, nativeFormat);
					result = newImage->write(0, 0, image->w, image->h, 0, 0, image);
				}
				delete image;
				image = newImage;
				if (!result)
				{
					hlog::error(logTag, "Could not write format: " + this->_getInternalName());
					delete image;
					image = NULL;
				}
			}
			else // might have been a meta data load
			{
				image->format = nativeFormat;
			}
		}
		return image;
	}

	bool Texture::lock()
	{
		// TODOx - remove
		/*
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot use locking, texture is not alterable: " + this->_getInternalName());
			return false;
		}
		if (this->locked)
		{
			return false;
		}
		this->locked = true;
		return true;
		*/
		return false;
	}

	bool Texture::unlock()
	{
		// TODOx - remove
		/*
		if (!this->locked)
		{
			return false;
		}
		this->locked = false;
		if (this->isLoaded() && this->dirty)
		{
			this->_uploadDataToGpu(0, 0, this->getWidth(), this->getHeight());
		}
		this->dirty = false;
		return true;
		*/
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		if (this->dirty)
		{
			this->dirty = false;
			this->_uploadDataToGpu(0, 0, this->width, this->height);
		}
		return false;
	}

	bool Texture::clear()
	{
		if (!this->_isWritable())
		{
			hlog::warn(logTag, "Cannot write texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot write texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		return this->_rawClear();
	}

	bool Texture::_rawClear()
	{
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		memset(this->data, 0, this->getByteSize());
		this->dirty = true;
		return true;
	}

	Color Texture::getPixel(int x, int y)
	{
		Color color = Color::Clear;
		if (!this->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + this->_getInternalName());
			return color;
		}
		if (this->data != NULL)
		{
			color = Image::getPixel(x, y, this->data, this->width, this->height, this->format);
		}
		return color;
	}

	bool Texture::setPixel(int x, int y, const Color& color)
	{
		if (!this->_isWritable())
		{
			hlog::warn(logTag, "Cannot write texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot write texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		return this->_rawSetPixel(x, y, color);
	}

	bool Texture::_rawSetPixel(int x, int y, const Color& color)
	{
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::setPixel(x, y, color, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	Color Texture::getInterpolatedPixel(float x, float y)
	{
		Color color = Color::Clear;
		if (!this->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + this->_getInternalName());
			return color;
		}
		if (this->data != NULL)
		{
			color = Image::getInterpolatedPixel(x, y, this->data, this->width, this->height, this->format);
		}
		return color;
	}

	bool Texture::fillRect(int x, int y, int w, int h, const Color& color)
	{
		if (!this->_isWritable())
		{
			hlog::warn(logTag, "Cannot write texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot write texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		return this->_rawFillRect(x, y, w, h, color);
	}

	bool Texture::_rawFillRect(int x, int y, int w, int h, const Color& color)
	{
		if (w == 1 && h == 1)
		{
			return this->_rawSetPixel(x, y, color);
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::fillRect(x, y, w, h, color, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::copyPixelData(unsigned char** output, Image::Format format)
	{
		if (!this->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			return false;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		return Image::convertToFormat(this->width, this->height, this->data, this->format, output, format, false);
	}

	Image* Texture::createImage(Image::Format format)
	{
		if (!this->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + this->_getInternalName());
			return NULL;
		}
		if (!this->ensureLoaded())
		{
			return NULL;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		unsigned char* data = NULL;
		Image* image = NULL;
		if (Image::convertToFormat(this->width, this->height, this->data, this->format, &data, format, false))
		{
			image = Image::create(this->width, this->height, NULL, format);
			image->data = data;
		}
		return image;
	}

	bool Texture::write(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		if (!this->_isWritable())
		{
			hlog::warn(logTag, "Cannot write texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot write texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		return this->_rawWrite(sx, sy, sw, sh, dx, dy, srcData, srcWidth, srcHeight, srcFormat);
	}

	bool Texture::_rawWrite(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::write(sx, sy, sw, sh, dx, dy, srcData, srcWidth, srcHeight, srcFormat, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::write(int sx, int sy, int sw, int sh, int dx, int dy, Texture* texture)
	{
		if (!this->_isWritable())
		{
			hlog::warn(logTag, "Cannot write texture: " + this->_getInternalName());
			return false;
		}
		if (texture == NULL)
		{
			hlog::warn(logTag, "Cannot read texture: NULL");
			return false;
		}
		if (!texture->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + texture->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot write texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		if (!texture->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot read texture '%s', not loaded!", texture->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&texture->asyncDataMutex);
		return this->write(sx, sy, sw, sh, dx, dy, texture->data, texture->width, texture->height, texture->format);
	}

	bool Texture::writeStretch(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		if (!this->_isWritable())
		{
			hlog::warn(logTag, "Cannot write texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot write texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::writeStretch(sx, sy, sw, sh, dx, dy, dw, dh, srcData, srcWidth, srcHeight, srcFormat, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::writeStretch(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, Texture* texture)
	{
		if (!this->_isWritable())
		{
			hlog::warn(logTag, "Cannot write texture: " + this->_getInternalName());
			return false;
		}
		if (texture == NULL)
		{
			hlog::warn(logTag, "Cannot read texture: NULL");
			return false;
		}
		if (!texture->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + texture->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot write texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		if (!texture->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot read texture '%s', not loaded!", texture->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&texture->asyncDataMutex);
		return this->writeStretch(sx, sy, sw, sh, dx, dy, dw, dh, texture->data, texture->width, texture->height, texture->format);
	}

	bool Texture::blit(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat, unsigned char alpha)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::blit(sx, sy, sw, sh, dx, dy, srcData, srcWidth, srcHeight, srcFormat, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::blit(int sx, int sy, int sw, int sh, int dx, int dy, Texture* texture, unsigned char alpha)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (texture == NULL)
		{
			hlog::warn(logTag, "Cannot read texture: NULL");
			return false;
		}
		if (!texture->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + texture->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		if (!texture->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot read texture '%s', not loaded!", texture->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&texture->asyncDataMutex);
		return this->blit(sx, sy, sw, sh, dx, dy, texture->data, texture->width, texture->height, texture->format);
	}

	bool Texture::blitStretch(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat, unsigned char alpha)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::blitStretch(sx, sy, sw, sh, dx, dy, dw, dh, srcData, srcWidth, srcHeight, srcFormat, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::blitStretch(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, Texture* texture, unsigned char alpha)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (texture == NULL)
		{
			hlog::warn(logTag, "Cannot read texture: NULL");
			return false;
		}
		if (!texture->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + texture->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		if (!texture->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot read texture '%s', not loaded!", texture->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&texture->asyncDataMutex);
		return this->blitStretch(sx, sy, sw, sh, dx, dy, dw, dh, texture->data, texture->width, texture->height, texture->format);
	}

	bool Texture::rotateHue(int x, int y, int w, int h, float degrees)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::rotateHue(x, y, w, h, degrees, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::saturate(int x, int y, int w, int h, float factor)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::saturate(x, y, w, h, factor, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::invert(int x, int y, int w, int h)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::invert(x, y, w, h, this->data, this->width, this->height, this->format);
		this->dirty |= result;
		return result;
	}

	bool Texture::insertAlphaMap(unsigned char* srcData, Image::Format srcFormat, unsigned char median, int ambiguity)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		
		hmutex::ScopeLock lock(&this->asyncDataMutex);
		bool result = Image::insertAlphaMap(this->width, this->height, srcData, srcFormat, this->data, this->format, median, ambiguity);
		this->dirty |= result;
		return result;
	}

	bool Texture::insertAlphaMap(Texture* texture, unsigned char median, int ambiguity)
	{
		if (!this->_isAlterable())
		{
			hlog::warn(logTag, "Cannot alter texture: " + this->_getInternalName());
			return false;
		}
		if (texture == NULL)
		{
			hlog::warn(logTag, "Cannot read texture: NULL");
			return false;
		}
		if (!texture->_isReadable())
		{
			hlog::warn(logTag, "Cannot read texture: " + texture->_getInternalName());
			return false;
		}
		if (!this->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot alter texture '%s', not loaded!", this->_getInternalName().cStr());
			return false;
		}
		if (!texture->ensureLoaded())
		{
			hlog::errorf(logTag, "Cannot read texture '%s', not loaded!", texture->_getInternalName().cStr());
			return false;
		}
		if (texture->width != this->width || texture->height != this->height)
		{
			hlog::errorf(logTag, "Cannot insert alpha map, texture sizes don't match: '%s'@%d,%d and '%s'@%d,%d",
				this->_getInternalName().cStr(), this->width, this->height, texture->_getInternalName().cStr(), texture->width, texture->height);
			return false;
		}
		hmutex::ScopeLock lock(&texture->asyncDataMutex);
		return this->insertAlphaMap(texture->data, texture->format, median, ambiguity);
	}

	// overloads

	Color Texture::getPixel(cgvec2 position)
	{
		return this->getPixel(HROUND_GVEC2(position));
	}

	bool Texture::setPixel(cgvec2 position, const Color& color)
	{
		return this->setPixel(HROUND_GVEC2(position), color);
	}

	Color Texture::getInterpolatedPixel(cgvec2 position)
	{
		return this->getInterpolatedPixel(position.x, position.y);
	}

	bool Texture::fillRect(cgrect rect, const Color& color)
	{
		return this->fillRect(HROUND_GRECT(rect), color);
	}

	bool Texture::copyPixelData(unsigned char** output)
	{
		return this->copyPixelData(output, this->format);
	}

	Image* Texture::createImage()
	{
		return this->createImage(this->format);
	}

	bool Texture::write(int sx, int sy, int sw, int sh, int dx, int dy, Image* image)
	{
		return this->write(sx, sy, sw, sh, dx, dy, image->data, image->w, image->h, image->format);
	}

	bool Texture::write(cgrect srcRect, cgvec2 destPosition, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		return this->write(HROUND_GRECT(srcRect), HROUND_GVEC2(destPosition), srcData, srcWidth, srcHeight, srcFormat);
	}

	bool Texture::write(cgrect srcRect, cgvec2 destPosition, Texture* texture)
	{
		return this->write(HROUND_GRECT(srcRect), HROUND_GVEC2(destPosition), texture);
	}

	bool Texture::write(cgrect srcRect, cgvec2 destPosition, Image* image)
	{
		return this->write(HROUND_GRECT(srcRect), HROUND_GVEC2(destPosition), image->data, image->w, image->h, image->format);
	}

	bool Texture::writeStretch(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, Image* image)
	{
		return this->writeStretch(sx, sy, sw, sh, dx, dy, dw, dh, image->data, image->w, image->h, image->format);
	}

	bool Texture::writeStretch(cgrect srcRect, cgrect destRect, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat)
	{
		return this->writeStretch(HROUND_GRECT(srcRect), HROUND_GRECT(destRect), srcData, srcWidth, srcHeight, srcFormat);
	}

	bool Texture::writeStretch(cgrect srcRect, cgrect destRect, Texture* texture)
	{
		return this->writeStretch(HROUND_GRECT(srcRect), HROUND_GRECT(destRect), texture);
	}

	bool Texture::writeStretch(cgrect srcRect, cgrect destRect, Image* image)
	{
		return this->writeStretch(HROUND_GRECT(srcRect), HROUND_GRECT(destRect), image->data, image->w, image->h, image->format);
	}

	bool Texture::blit(int sx, int sy, int sw, int sh, int dx, int dy, Image* image, unsigned char alpha)
	{
		return this->blit(sx, sy, sw, sh, dx, dy, image->data, image->w, image->h, image->format, alpha);
	}

	bool Texture::blit(cgrect srcRect, cgvec2 destPosition, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat, unsigned char alpha)
	{
		return this->blit(HROUND_GRECT(srcRect), HROUND_GVEC2(destPosition), srcData, srcWidth, srcHeight, srcFormat, alpha);
	}

	bool Texture::blit(cgrect srcRect, cgvec2 destPosition, Texture* texture, unsigned char alpha)
	{
		return this->blit(HROUND_GRECT(srcRect), HROUND_GVEC2(destPosition), texture, alpha);
	}

	bool Texture::blit(cgrect srcRect, cgvec2 destPosition, Image* image, unsigned char alpha)
	{
		return this->blit(HROUND_GRECT(srcRect), HROUND_GVEC2(destPosition), image->data, image->w, image->h, image->format, alpha);
	}

	bool Texture::blitStretch(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, Image* image, unsigned char alpha)
	{
		return this->blitStretch(sx, sy, sw, sh, dx, dy, dw, dh, image->data, image->w, image->h, image->format, alpha);
	}

	bool Texture::blitStretch(cgrect srcRect, cgrect destRect, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat, unsigned char alpha)
	{
		return this->blitStretch(HROUND_GRECT(srcRect), HROUND_GRECT(destRect), srcData, srcWidth, srcHeight, srcFormat, alpha);
	}

	bool Texture::blitStretch(cgrect srcRect, cgrect destRect, Texture* texture, unsigned char alpha)
	{
		return this->blitStretch(HROUND_GRECT(srcRect), HROUND_GRECT(destRect), texture, alpha);
	}

	bool Texture::blitStretch(cgrect srcRect, cgrect destRect, Image* image, unsigned char alpha)
	{
		return this->blitStretch(HROUND_GRECT(srcRect), HROUND_GRECT(destRect), image->data, image->w, image->h, image->format, alpha);
	}

	bool Texture::rotateHue(cgrect rect, float degrees)
	{
		return this->rotateHue(HROUND_GRECT(rect), degrees);
	}

	bool Texture::saturate(cgrect rect, float factor)
	{
		return this->saturate(HROUND_GRECT(rect), factor);
	}

	bool Texture::invert(cgrect rect)
	{
		return this->invert(HROUND_GRECT(rect));
	}

	bool Texture::insertAlphaMap(Image* image, unsigned char median, int ambiguity)
	{
		// safety checks are done in the other insertAlphaMap()
		if (image->w != this->width || image->h != this->height)
		{
			return false;
		}
		return this->insertAlphaMap(image->data, image->format, median, ambiguity);
	}

	void Texture::_setupPot(int& outWidth, int& outHeight)
	{
		outWidth = hpotCeil(this->width);
		outHeight = hpotCeil(this->height);
		if (this->width < outWidth || this->height < outHeight)
		{
			// software NPOT textures do not support anything other than AddressMode::Clamp
			this->addressMode = AddressMode::Clamp;
			// effective addressing area needs to be changed
			this->effectiveWidth = (float)this->width / outWidth;
			this->effectiveHeight = (float)this->height / outHeight;
		}
	}

	unsigned char* Texture::_createPotData(int& outWidth, int& outHeight, unsigned char* data)
	{
		this->_setupPot(outWidth, outHeight);
		unsigned char* newData = new unsigned char[outWidth * outHeight * this->format.getBpp()];
		Image::write(0, 0, this->width, this->height, 0, 0, data, this->width, this->height, this->format, newData, outWidth, outHeight, this->format);
		if (this->width < outWidth)
		{
			Image::writeStretch(this->width - 1, 0, 1, this->height, this->width, 0, outWidth - this->width, this->height, newData, outWidth, outHeight, this->format, newData, outWidth, outHeight, this->format);
		}
		if (this->height < outHeight)
		{
			Image::writeStretch(0, this->height - 1, outWidth, 1, 0, this->height, outWidth, outHeight - this->height, newData, outWidth, outHeight, this->format, newData, outWidth, outHeight, this->format);
		}
		return newData;
	}

	unsigned char* Texture::_createPotClearData(int& outWidth, int& outHeight)
	{
		this->_setupPot(outWidth, outHeight);
		int size = outWidth * outHeight * this->format.getBpp();
		unsigned char* newData = new unsigned char[size];
		memset(newData, 0, size);
		return newData;
	}

	Texture::Lock Texture::_tryLock(int x, int y, int w, int h)
	{
		Lock lock;
		// TODOx - remove
		/*
		if (this->data != NULL)
		{
			lock.activateLock(x, y, w, h, x, y, this->data, this->width, this->height, this->format);
		}
		else
		{
			this->loadAsync();
			this->ensureLoaded();
			lock = this->_tryLockSystem(x, y, w, h);
		}
		if (!lock.failed)
		{
			this->locked = true;
		}
		*/
		return lock;
	}

	Texture::Lock Texture::_tryLock()
	{
		return this->_tryLock(0, 0, this->width, this->height);
	}

	bool Texture::_unlock(Texture::Lock lock, bool update)
	{
		// TODOx - remove
		/*
		if (!this->_unlockSystem(lock, update) && !lock.failed && update)
		{
			this->dirty = true;
		}
		*/
		return update;
	}

	bool Texture::_uploadDataToGpu(int x, int y, int w, int h)
	{
		if ((!Image::needsConversion(this->format, april::rendersys->getNativeTextureFormat(this->format)) &&
			this->_uploadToGpu(x, y, w, h, x, y, this->data, this->width, this->height, this->format)) || this->dirty)
		{
			return true;
		}
		Lock lock = this->_tryLockSystem(x, y, w, h);
		if (lock.failed)
		{
			return false;
		}
		bool result = Image::write(x, y, w, h, lock.x, lock.y, this->data, this->width, this->height, this->format, lock.data, lock.dataWidth, lock.dataHeight, lock.format);
		this->_unlockSystem(lock, true);
		return result;
	}

}
