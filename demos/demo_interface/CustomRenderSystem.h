/// @file
/// @version 5.2
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifndef CUSTOM_RENDER_SYSTEM_H
#define CUSTOM_RENDER_SYSTEM_H

#include <d3d9.h>
#include <d3d9types.h>

#define __HL_INCLUDE_PLATFORM_HEADERS
#include <hltypes/hplatform.h>

#include <april/RenderSystem.h>

namespace april
{
	class CustomTexture;
	class Image;
	class Window;

	class CustomRenderSystem : public RenderSystem
	{
	public:
		friend class CustomTexture;

		CustomRenderSystem();
		~CustomRenderSystem();

		int getVRam() const;

		Image::Format getNativeTextureFormat(Image::Format format) const override;
		unsigned int getNativeColorUInt(const Color& color) const override;

	protected:
		IDirect3D9* d3d;
		IDirect3DDevice9* d3dDevice;
		_D3DPRESENT_PARAMETERS_* d3dpp;
		IDirect3DSurface9* backBuffer;
		HWND childHWnd;

		void _deviceInit() override;
		bool _deviceCreate(Options options) override;
		bool _deviceDestroy() override;
		void _deviceAssignWindow(Window* window) override;
		void _deviceReset() override;
		void _deviceSetupCaps() override;
		void _deviceSetup() override;
		void _deviceSetupDisplayModes() override;

		void _tryAssignChildWindow();
		void _tryUnassignChildWindow();

		Texture* _deviceCreateTexture(bool fromResource) override;

		void _deviceChangeResolution(int width, int height, bool fullscreen) override;

		void _setDeviceViewport(cgrecti rect) override;
		void _setDeviceModelviewMatrix(const gmat4& matrix) override;
		void _setDeviceProjectionMatrix(const gmat4& matrix) override;
		void _setDeviceDepthBuffer(bool enabled, bool writeEnabled) override;
		void _setDeviceRenderMode(bool useTexture, bool useColor) override;
		void _setDeviceTexture(Texture* texture) override;
		void _setDeviceTextureFilter(const Texture::Filter& textureFilter) override;
		void _setDeviceTextureAddressMode(const Texture::AddressMode& textureAddressMode) override;
		void _setDeviceBlendMode(const BlendMode& blendMode) override;
		void _setDeviceColorMode(const ColorMode& colorMode, float colorModeFactor, bool useTexture, bool useColor, const Color& systemColor) override;
		void _setDeviceRenderTarget(Texture* texture) override;

		void _deviceClear(bool depth) override;
		void _deviceClear(const Color& color, bool depth) override;
		void _deviceClearDepth() override;
		void _deviceRender(const RenderOperation& renderOperation, const PlainVertex* vertices, int count) override;
		void _deviceRender(const RenderOperation& renderOperation, const TexturedVertex* vertices, int count) override;
		void _deviceRender(const RenderOperation& renderOperation, const ColoredVertex* vertices, int count) override;
		void _deviceRender(const RenderOperation& renderOperation, const ColoredTexturedVertex* vertices, int count) override;
		void _devicePresentFrame(bool systemEnabled) override;
		void _deviceCopyRenderTargetData(Texture* source, Texture* destination) override;
		void _deviceTakeScreenshot(Image::Format format, bool backBufferOnly) override;

		static D3DPRIMITIVETYPE _dx9RenderOperations[];

	private:
		bool _supportsA8Surface; // this does not seem to be detectable via any type of device caps

	};

}
#endif
