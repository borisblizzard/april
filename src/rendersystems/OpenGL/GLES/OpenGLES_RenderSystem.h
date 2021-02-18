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
/// Defines a generic OpenGL render system.

#ifdef _OPENGLES
#ifndef APRIL_OPENGLES_RENDER_SYSTEM_H
#define APRIL_OPENGLES_RENDER_SYSTEM_H

#include "OpenGL_RenderSystem.h"

namespace april
{
	class OpenGLES_PixelShader;
	class OpenGLES_Texture;
	class OpenGLES_VertexShader;

	class OpenGLES_RenderSystem : public OpenGL_RenderSystem
	{
	public:
		friend class OpenGLES_Texture;

		class ShaderProgram
		{
		public:
			friend class OpenGLES_RenderSystem;

			ShaderProgram();
			~ShaderProgram();

			bool load(unsigned int pixelShaderId, unsigned int vertexShaderId);

		protected:
			unsigned int glShaderProgram;

		};

		OpenGLES_RenderSystem();

#ifdef __ANDROID__
		bool canUseLowLevelCalls() const override;
#endif

	protected:
		OpenGLES_VertexShader* vertexShaderPlain;
		OpenGLES_VertexShader* vertexShaderTextured;
		OpenGLES_VertexShader* vertexShaderExTextured;
		OpenGLES_VertexShader* vertexShaderColored;
		OpenGLES_VertexShader* vertexShaderColoredTextured;
		OpenGLES_VertexShader* vertexShaderColoredExTextured;
		OpenGLES_PixelShader* pixelShaderMultiply;
		OpenGLES_PixelShader* pixelShaderAlphaMap;
		OpenGLES_PixelShader* pixelShaderLerp;
		OpenGLES_PixelShader* pixelShaderDesaturate;
		OpenGLES_PixelShader* pixelShaderSepia;
		OpenGLES_PixelShader* pixelShaderTexturedMultiply;
		OpenGLES_PixelShader* pixelShaderTexturedAlphaMap;
		OpenGLES_PixelShader* pixelShaderTexturedLerp;
		OpenGLES_PixelShader* pixelShaderTexturedDesaturate;
		OpenGLES_PixelShader* pixelShaderTexturedSepia;
		OpenGLES_PixelShader* pixelShaderExTexturedMultiply;
		OpenGLES_PixelShader* pixelShaderExTexturedAlphaMap;
		OpenGLES_PixelShader* pixelShaderExTexturedLerp;
		OpenGLES_PixelShader* pixelShaderExTexturedDesaturate;
		OpenGLES_PixelShader* pixelShaderExTexturedSepia;
		OpenGLES_PixelShader* pixelShaderColoredMultiply;
		OpenGLES_PixelShader* pixelShaderColoredAlphaMap;
		OpenGLES_PixelShader* pixelShaderColoredLerp;
		OpenGLES_PixelShader* pixelShaderColoredDesaturate;
		OpenGLES_PixelShader* pixelShaderColoredSepia;
		OpenGLES_PixelShader* pixelShaderColoredTexturedMultiply;
		OpenGLES_PixelShader* pixelShaderColoredTexturedAlphaMap;
		OpenGLES_PixelShader* pixelShaderColoredTexturedLerp;
		OpenGLES_PixelShader* pixelShaderColoredTexturedDesaturate;
		OpenGLES_PixelShader* pixelShaderColoredTexturedSepia;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedMultiply;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedAlphaMap;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedLerp;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedDesaturate;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedSepia;
#ifdef __ANDROID__
		OpenGLES_PixelShader* pixelShaderTexturedMultiply_AlphaHack;
		OpenGLES_PixelShader* pixelShaderTexturedLerp_AlphaHack;
		OpenGLES_PixelShader* pixelShaderTexturedDesaturate_AlphaHack;
		OpenGLES_PixelShader* pixelShaderTexturedSepia_AlphaHack;
		OpenGLES_PixelShader* pixelShaderExTexturedMultiply_AlphaHack;
		OpenGLES_PixelShader* pixelShaderExTexturedLerp_AlphaHack;
		OpenGLES_PixelShader* pixelShaderExTexturedDesaturate_AlphaHack;
		OpenGLES_PixelShader* pixelShaderExTexturedSepia_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredTexturedMultiply_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredTexturedLerp_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredTexturedDesaturate_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredTexturedSepia_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedMultiply_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedLerp_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedDesaturate_AlphaHack;
		OpenGLES_PixelShader* pixelShaderColoredExTexturedSepia_AlphaHack;
#endif
		ShaderProgram* shaderMultiply;
		ShaderProgram* shaderAlphaMap;
		ShaderProgram* shaderLerp;
		ShaderProgram* shaderDesaturate;
		ShaderProgram* shaderSepia;
		ShaderProgram* shaderTexturedMultiply;
		ShaderProgram* shaderTexturedAlphaMap;
		ShaderProgram* shaderTexturedLerp;
		ShaderProgram* shaderTexturedDesaturate;
		ShaderProgram* shaderTexturedSepia;
		ShaderProgram* shaderExTexturedMultiply;
		ShaderProgram* shaderExTexturedAlphaMap;
		ShaderProgram* shaderExTexturedLerp;
		ShaderProgram* shaderExTexturedDesaturate;
		ShaderProgram* shaderExTexturedSepia;
		ShaderProgram* shaderColoredMultiply;
		ShaderProgram* shaderColoredAlphaMap;
		ShaderProgram* shaderColoredLerp;
		ShaderProgram* shaderColoredDesaturate;
		ShaderProgram* shaderColoredSepia;
		ShaderProgram* shaderColoredTexturedMultiply;
		ShaderProgram* shaderColoredTexturedAlphaMap;
		ShaderProgram* shaderColoredTexturedLerp;
		ShaderProgram* shaderColoredTexturedDesaturate;
		ShaderProgram* shaderColoredTexturedSepia;
		ShaderProgram* shaderColoredExTexturedMultiply;
		ShaderProgram* shaderColoredExTexturedAlphaMap;
		ShaderProgram* shaderColoredExTexturedLerp;
		ShaderProgram* shaderColoredExTexturedDesaturate;
		ShaderProgram* shaderColoredExTexturedSepia;
#ifdef __ANDROID__
		ShaderProgram* shaderTexturedMultiply_AlphaHack;
		ShaderProgram* shaderTexturedLerp_AlphaHack;
		ShaderProgram* shaderTexturedDesaturate_AlphaHack;
		ShaderProgram* shaderTexturedSepia_AlphaHack;
		ShaderProgram* shaderExTexturedMultiply_AlphaHack;
		ShaderProgram* shaderExTexturedLerp_AlphaHack;
		ShaderProgram* shaderExTexturedDesaturate_AlphaHack;
		ShaderProgram* shaderExTexturedSepia_AlphaHack;
		ShaderProgram* shaderColoredTexturedMultiply_AlphaHack;
		ShaderProgram* shaderColoredTexturedLerp_AlphaHack;
		ShaderProgram* shaderColoredTexturedDesaturate_AlphaHack;
		ShaderProgram* shaderColoredTexturedSepia_AlphaHack;
		ShaderProgram* shaderColoredExTexturedMultiply_AlphaHack;
		ShaderProgram* shaderColoredExTexturedLerp_AlphaHack;
		ShaderProgram* shaderColoredExTexturedDesaturate_AlphaHack;
		ShaderProgram* shaderColoredExTexturedSepia_AlphaHack;
		bool etc1Supported;
#endif

		bool deviceState_matrixChanged;
		bool deviceState_systemColorChanged;
		bool deviceState_colorModeFactorChanged;
		unsigned int framebufferId;
		unsigned int renderbufferId;
		// TODOa - these need to be refactored
		ShaderProgram* deviceState_shader;

		void _deviceInit() override;
		bool _deviceCreate(Options options) override;
		bool _deviceDestroy() override;
		void _deviceAssignWindow(Window* window) override;
		void _deviceSuspend() override;
		void _deviceSetupCaps() override;
		void _deviceSetup() override;

		void _createShaders();
		void _destroyShaders();

		void _updateDeviceState(RenderState* state, bool forceUpdate = false, bool ignoreRenderTarget = false) override;

		void _setDeviceModelviewMatrix(const gmat4& matrix) override;
		void _setDeviceProjectionMatrix(const gmat4& matrix) override;
		void _setDeviceBlendMode(const BlendMode& blendMode) override;
		void _setDeviceTexture(Texture* texture) override;
		void _setDeviceColorMode(const ColorMode& colorMode, float colorModeFactor, bool useTexture, bool useColor, const Color& systemColor) override;
		void _setDeviceRenderTarget(Texture* texture) override;
		void _updateShader(bool forceUpdate);

		void _devicePresentFrame(bool systemEnabled) override;
		void _deviceCopyRenderTargetData(Texture* source, Texture* destination) override;
		void _deviceTakeScreenshot(Image::Format format, bool backBufferOnly) override;

		void _setGlTextureEnabled(bool enabled) override;
		void _setGlColorEnabled(bool enabled) override;
		void _setGlVertexPointer(int stride, const void* pointer) override;
		void _setGlTexturePointer(int stride, const void* pointer) override;
		void _setGlColorPointer(int stride, const void* pointer) override;

	};
	
}
#endif
#endif
