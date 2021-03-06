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
/// Defines a DirectX11 specific texture.

#ifdef _DIRECTX11
#ifndef APRIL_DIRECTX11_TEXTURE_H
#define APRIL_DIRECTX11_TEXTURE_H

#include <hltypes/hplatform.h>
#include <hltypes/hstring.h>

#include "RenderSystem.h"
#include "DirectX_Texture.h"

using namespace Microsoft::WRL;

namespace april
{
	class DirectX11_RenderSystem;

	class DirectX11_Texture : public DirectX_Texture
	{
	public:
		friend class DirectX11_RenderSystem;
		
		void* getBackendId() const;

	protected:
		DirectX11_Texture(bool fromResource);
		
		ComPtr<ID3D11Texture2D> d3dTexture;
		ComPtr<ID3D11ShaderResourceView> d3dView;
		ComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
		DXGI_FORMAT dxgiFormat;

		bool _deviceCreateTexture(unsigned char* data, int size);
		bool _deviceDestroyTexture();
		void _assignFormat();

		Lock _tryLockSystem(int x, int y, int w, int h);
		bool _unlockSystem(Lock& lock, bool update);

	};

}

#endif
#endif
