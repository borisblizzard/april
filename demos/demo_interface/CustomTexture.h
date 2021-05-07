/// @file
/// @version 5.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifndef CUSTOM_TEXTURE_H
#define CUSTOM_TEXTURE_H

#include <april/Texture.h>

struct IDirect3DTexture9;
struct IDirect3DSurface9;

namespace april
{
	class CustomRenderSystem;

	class CustomTexture : public Texture
	{
	public:
		friend class CustomRenderSystem;

		CustomTexture(bool fromResource);

		void* getBackendId() const;
		
	protected:
		IDirect3DSurface9* d3dSurface;
		IDirect3DTexture9* d3dTexture;
		D3DFORMAT d3dFormat;
		D3DPOOL d3dPool;
		DWORD d3dUsage;

		bool _deviceCreateTexture(unsigned char* data, int size) override;
		bool _deviceDestroyTexture() override;
		void _assignFormat() override;

		IDirect3DSurface9* _getSurface();

		Lock _tryLockSystem(int x, int y, int w, int h) override;
		bool _unlockSystem(Lock& lock, bool update) override;
		bool _uploadToGpu(int sx, int sy, int sw, int sh, int dx, int dy, unsigned char* srcData, int srcWidth, int srcHeight, Image::Format srcFormat) override;

	};

}

#endif
