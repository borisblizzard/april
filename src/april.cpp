/// @file
/// @version 4.4
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include <stdio.h>
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#define __HL_INCLUDE_PLATFORM_HEADERS
#include <hltypes/harray.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hlog.h>
#include <hltypes/hplatform.h>
#include <hltypes/hresource.h>
#include <hltypes/hstring.h>
#include <hltypes/hversion.h>

#include "april.h"
#include "Platform.h"
#include "RenderSystem.h"
#ifdef _DIRECTX9
#include "DirectX9_RenderSystem.h"
#endif
#ifdef _DIRECTX11
#include "DirectX11_RenderSystem.h"
#endif
#ifdef _DIRECTX12
#include "DirectX12_RenderSystem.h"
#endif
#ifdef _OPENGL1
#include "OpenGL1_RenderSystem.h"
#endif
#ifdef _OPENGLES1
#include "OpenGLES1_RenderSystem.h"
#endif
#ifdef _OPENGLES2
#include "OpenGLES2_RenderSystem.h"
#endif
#include "Window.h"
#ifdef _ANDROID
#include "AndroidJNI_Window.h"
#endif
#ifdef _IOS
#include "iOS_Window.h"
#endif
#ifdef _SDL_WINDOW
#include "SDL_Window.h"
#endif
#ifdef _COCOA_WINDOW
#include "Mac_Window.h"
#endif
#ifdef _WIN32_WINDOW
#include "Win32_Window.h"
#endif
#ifdef _WINRT_WINDOW
#include "WinRT_Window.h"
#endif
#ifdef _WINUWP_WINDOW
#include "WinUWP_Window.h"
#endif
#ifdef _EGL
#include "egl.h"
#endif

#ifdef _WIN32
	#ifdef _DIRECTX9
		#define RS_INTERNAL_DEFAULT RenderSystemType::DirectX9
	#elif defined(_DIRECTX11)
		#define RS_INTERNAL_DEFAULT RenderSystemType::DirectX11
	#elif defined(_DIRECTX12)
		#define RS_INTERNAL_DEFAULT RenderSystemType::DirectX12
	#elif defined(_OPENGL1)
		#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGL1
	#elif defined(_OPENGLES1)
		#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGLES1
	#elif defined(_OPENGLES2)
		#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGLES2
	#endif
	#ifndef _WINRT
		#define WS_INTERNAL_DEFAULT WindowType::Win32
	#elif defined(_WINUWP)
		#define WS_INTERNAL_DEFAULT WindowType::WinUWP
	#else
		#define WS_INTERNAL_DEFAULT WindowType::WinRT
	#endif
#elif defined(__APPLE__)
	#ifdef _IOS
		#ifdef _OPENGLES2
			#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGLES2
		#elif defined(_OPENGLES1)
			#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGLES1
		#endif
		#define WS_INTERNAL_DEFAULT WindowType::iOS
	#else
		#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGL1
		#ifdef _SDL_WINDOW
			#define WS_INTERNAL_DEFAULT WindowType::SDL
		#else
			#define WS_INTERNAL_DEFAULT WindowType::Mac
		#endif
	#endif
#elif defined(_UNIX)
	#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGL1
	#define WS_INTERNAL_DEFAULT WindowType::SDL
#elif defined(_ANDROID)
	#ifdef _OPENGLES1
		#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGLES1
	#elif defined(_OPENGLES2)
		#define RS_INTERNAL_DEFAULT RenderSystemType::OpenGLES2
	#endif
	#ifdef _ANDROIDJNI_WINDOW
		#define WS_INTERNAL_DEFAULT WindowType::AndroidJNI
	#endif
#endif

#ifndef RS_INTERNAL_DEFAULT
	#define RS_INTERNAL_DEFAULT RenderSystemType::Default
#endif
#ifndef WS_INTERNAL_DEFAULT
	#define WS_INTERNAL_DEFAULT WindowType::Default
#endif

#ifdef _WIN32
	#ifndef _WINRT
		#define APRIL_PLATFORM_NAME "Win32"
	#elif defined(_WINUWP)
		#define APRIL_PLATFORM_NAME "WinUWP"
	#elif !defined(_WINP8)
		#define APRIL_PLATFORM_NAME "WinRT"
	#else
		#define APRIL_PLATFORM_NAME "WinP8"
	#endif
#elif defined(_ANDROID)
	#define APRIL_PLATFORM_NAME "Android"
#elif defined(__APPLE__)
	#ifdef _IOS
		#define APRIL_PLATFORM_NAME "iOS"
	#else
		#define APRIL_PLATFORM_NAME "Mac OS X"
		#ifdef __LP64__
			#define APRIL_PLATFORM_ARCHITECTURE "x64"
		#endif
	#endif
#elif defined(_UNIX)
	#define APRIL_PLATFORM_NAME "Unix"
#endif
#ifndef APRIL_PLATFORM_NAME
#define APRIL_PLATFORM_NAME "Unknown"
#endif

#ifdef _ARM
	#define APRIL_PLATFORM_ARCHITECTURE "ARM"
#endif
#ifndef APRIL_PLATFORM_ARCHITECTURE
	#define APRIL_PLATFORM_ARCHITECTURE "x86"
#endif

#if defined(_IOS) || defined(_MAC)
	#ifdef __LP64__
		#define APRIL_PLATFORM_ARCHITECTURE_BITS 64
	#else
		#define APRIL_PLATFORM_ARCHITECTURE_BITS 32
	#endif
#else
	#define APRIL_PLATFORM_ARCHITECTURE_BITS 32
#endif

namespace april
{
	hstr logTag = "april";

	static hversion version(4, 4, 0);

	static harray<hstr> extensions;
	static int maxAsyncTextureUploadsPerFrame = 0;
#if defined(_ANDROID) || defined(_IOS) || defined(_WINRT) && defined(_WINP8)
	static int maxWaitingAsyncTextures = 8; // to limit RAM consumption
#else
	static int maxWaitingAsyncTextures = 0;
#endif
	hmap<hstr, april::Color> symbolicColors;

	HL_ENUM_CLASS_DEFINE(RenderSystemType,
	(
		HL_ENUM_DEFINE(RenderSystemType, Default);
		HL_ENUM_DEFINE(RenderSystemType, DirectX9);
		HL_ENUM_DEFINE(RenderSystemType, DirectX11);
		HL_ENUM_DEFINE(RenderSystemType, DirectX12);
		HL_ENUM_DEFINE(RenderSystemType, OpenGL1);
		HL_ENUM_DEFINE(RenderSystemType, OpenGLES1);
		HL_ENUM_DEFINE(RenderSystemType, OpenGLES2);
	));

	HL_ENUM_CLASS_DEFINE(WindowType,
	(
		HL_ENUM_DEFINE(WindowType, Default);
		HL_ENUM_DEFINE(WindowType, Win32);
		HL_ENUM_DEFINE(WindowType, WinRT);
		HL_ENUM_DEFINE(WindowType, WinUWP);
		HL_ENUM_DEFINE(WindowType, SDL);
		HL_ENUM_DEFINE(WindowType, Mac);
		HL_ENUM_DEFINE(WindowType, iOS);
		HL_ENUM_DEFINE(WindowType, AndroidJNI);
		HL_ENUM_DEFINE(WindowType, OpenKODE);
	));

	void _startInit()
	{
		hlog::write(logTag, "Initializing APRIL: " + version.toString());
		hlog::writef(logTag, "Platform: %s %s, %d bit", APRIL_PLATFORM_NAME, APRIL_PLATFORM_ARCHITECTURE, APRIL_PLATFORM_ARCHITECTURE_BITS);
		// symbolic colors
		symbolicColors["white"] = april::Color::White;
		symbolicColors["black"] = april::Color::Black;
		symbolicColors["grey"] = april::Color::Grey;
		symbolicColors["red"] = april::Color::Red;
		symbolicColors["green"] = april::Color::Green;
		symbolicColors["blue"] = april::Color::Blue;
		symbolicColors["yellow"] = april::Color::Yellow;
		symbolicColors["magenta"] = april::Color::Magenta;
		symbolicColors["cyan"] = april::Color::Cyan;
		symbolicColors["orange"] = april::Color::Orange;
		symbolicColors["pink"] = april::Color::Pink;
		symbolicColors["teal"] = april::Color::Teal;
		symbolicColors["neon"] = april::Color::Neon;
		symbolicColors["purple"] = april::Color::Purple;
		symbolicColors["aqua"] = april::Color::Aqua;
		symbolicColors["light_grey"] = april::Color::LightGrey;
		symbolicColors["light_red"] = april::Color::LightRed;
		symbolicColors["light_green"] = april::Color::LightGreen;
		symbolicColors["light_blue"] = april::Color::LightBlue;
		symbolicColors["light_yellow"] = april::Color::LightYellow;
		symbolicColors["light_magenta"] = april::Color::LightMagenta;
		symbolicColors["light_cyan"] = april::Color::LightCyan;
		symbolicColors["light_orange"] = april::Color::LightOrange;
		symbolicColors["light_pink"] = april::Color::LightPink;
		symbolicColors["light_teal"] = april::Color::LightTeal;
		symbolicColors["light_neon"] = april::Color::LightNeon;
		symbolicColors["light_purple"] = april::Color::LightPurple;
		symbolicColors["light_aqua"] = april::Color::LightAqua;
		symbolicColors["dark_grey"] = april::Color::DarkGrey;
		symbolicColors["dark_red"] = april::Color::DarkRed;
		symbolicColors["dark_green"] = april::Color::DarkGreen;
		symbolicColors["dark_blue"] = april::Color::DarkBlue;
		symbolicColors["dark_yellow"] = april::Color::DarkYellow;
		symbolicColors["dark_magenta"] = april::Color::DarkMagenta;
		symbolicColors["dark_cyan"] = april::Color::DarkCyan;
		symbolicColors["dark_orange"] = april::Color::DarkOrange;
		symbolicColors["dark_pink"] = april::Color::DarkPink;
		symbolicColors["dark_teal"] = april::Color::DarkTeal;
		symbolicColors["dark_neon"] = april::Color::DarkNeon;
		symbolicColors["dark_purple"] = april::Color::DarkPurple;
		symbolicColors["dark_aqua"] = april::Color::DarkAqua;
        symbolicColors["clear"] = april::Color::Clear;
        symbolicColors["blank"] = april::Color::Blank;
        
		// extensions
		if (extensions.size() == 0)
		{
			extensions += ".jpt";
			extensions += ".png";
			extensions += ".jpg";
#ifdef _IOS
			extensions += ".pvrz";
			extensions += ".pvr";
#endif
#ifdef _ANDROID
			extensions += ".etcx";
#endif
		}
#ifdef _EGL
		if (april::egl == NULL)
		{
			april::egl = new EglData();
		}
#endif
	}

	void _finishInit()
	{
		SystemInfo info = april::getSystemInfo(); // calling getSystemInfo() is required here so it's initialized on certain platforms
		hlog::writef(logTag, "OS Version: %s", info.osVersion.toString().cStr());
		hlog::writef(logTag, "Using: %s, %s", april::rendersys->getName().cStr(), april::window->getName().cStr());
	}

	void _createRenderSystem(RenderSystemType renderSystemType)
	{
		// creating the rendersystem
		RenderSystemType renderSystem = renderSystemType;
		if (renderSystem == RenderSystemType::Default)
		{
			renderSystem = RS_INTERNAL_DEFAULT;
		}
#ifdef _DIRECTX9
		if (april::rendersys == NULL && renderSystem == RenderSystemType::DirectX9)
		{
			april::rendersys = new DirectX9_RenderSystem();
		}
#endif
#ifdef _DIRECTX11
		if (april::rendersys == NULL && renderSystem == RenderSystemType::DirectX11)
		{
			april::rendersys = new DirectX11_RenderSystem();
		}
#endif
#ifdef _DIRECTX12
		if (april::rendersys == NULL && renderSystem == RenderSystemType::DirectX12)
		{
			april::rendersys = new DirectX12_RenderSystem();
		}
#endif
#ifdef _OPENGL1
		if (april::rendersys == NULL && renderSystem == RenderSystemType::OpenGL1)
		{
			april::rendersys = new OpenGL1_RenderSystem();
		}
#endif
#ifdef _OPENGLES1
		if (april::rendersys == NULL && renderSystem == RenderSystemType::OpenGLES1)
		{
			april::rendersys = new OpenGLES1_RenderSystem();
		}
#endif
#ifdef _OPENGLES2
		if (april::rendersys == NULL && renderSystem == RenderSystemType::OpenGLES2)
		{
			april::rendersys = new OpenGLES2_RenderSystem();
		}
#endif
		if (april::rendersys == NULL)
		{
			throw Exception("Could not create given rendersystem!");
		}
		april::rendersys->init();
	}

	void _createWindowSystem(WindowType windowType)
	{
		// creating the windowsystem
		WindowType window = windowType;
		if (window == WindowType::Default)
		{
			window = WS_INTERNAL_DEFAULT;
		}
#ifdef _WIN32_WINDOW
		if (april::window == NULL && window == WindowType::Win32)
		{
			april::window = new Win32_Window();
		}
#endif
#ifdef _WINRT_WINDOW
		if (april::window == NULL && window == WindowType::WinRT)
		{
			april::window = new WinRT_Window();
		}
#endif
#ifdef _WINUWP_WINDOW
		if (april::window == NULL && window == WindowType::WinUWP)
		{
			april::window = new WinUWP_Window();
		}
#endif
#ifdef _SDL_WINDOW
		if (april::window == NULL && window == WindowType::SDL)
		{
			april::window = new SDL_Window();
		}
#endif
#ifdef _COCOA_WINDOW
		if (april::window == NULL && window == WindowType::Mac)
		{
			april::window = new Mac_Window();
		}
#endif
#ifdef _IOS
		if (april::window == NULL && window == WindowType::iOS)
		{
			april::window = new iOS_Window();
		}
#endif
#ifdef _ANDROIDJNI_WINDOW
		if (april::window == NULL && window == WindowType::AndroidJNI)
		{
			april::window = new AndroidJNI_Window();
		}
#endif
		if (april::window == NULL)
		{
			throw Exception("Could not create given windowsystem!");
		}
	}

	void init(RenderSystemType renderSystemType, WindowType windowType)
	{
		_startInit();
		_createRenderSystem(renderSystemType);
		_createWindowSystem(windowType);
		_finishInit();
	}
	
	void init(RenderSystem* customRenderSystem, WindowType windowType)
	{
		_startInit();
		april::rendersys = customRenderSystem;
		_createWindowSystem(windowType);
		_finishInit();
	}
	
	void init(RenderSystemType renderSystemType, Window* customWindow)
	{
		_startInit();
		_createRenderSystem(renderSystemType);
		april::window = customWindow;
		_finishInit();
	}
	
	void init(RenderSystem* customRenderSystem, Window* customWindow)
	{
		_startInit();
		april::rendersys = customRenderSystem;
		april::window = customWindow;
		_finishInit();
	}
	
	void init(RenderSystemType renderSystemType, WindowType windowType, RenderSystem::Options renderSystemOptions,
		int w, int h, bool fullscreen, chstr title, Window::Options windowOptions)
	{
		init(renderSystemType, windowType);
		createRenderSystem(renderSystemOptions);
		createWindow(w, h, fullscreen, title, windowOptions);
	}
	
	void init(RenderSystem* customRenderSystem, WindowType windowType, RenderSystem::Options renderSystemOptions,
		int w, int h, bool fullscreen, chstr title, Window::Options windowOptions)
	{
		init(customRenderSystem, windowType);
		createRenderSystem(renderSystemOptions);
		createWindow(w, h, fullscreen, title, windowOptions);
	}
	
	void init(RenderSystemType renderSystemType, Window* customWindow, RenderSystem::Options renderSystemOptions,
		int w, int h, bool fullscreen, chstr title, Window::Options windowOptions)
	{
		init(renderSystemType, customWindow);
		createRenderSystem(renderSystemOptions);
		createWindow(w, h, fullscreen, title, windowOptions);
	}
	
	void init(RenderSystem* customRenderSystem, Window* customWindow, RenderSystem::Options renderSystemOptions,
		int w, int h, bool fullscreen, chstr title, Window::Options windowOptions)
	{
		init(customRenderSystem, customWindow);
		createRenderSystem(renderSystemOptions);
		createWindow(w, h, fullscreen, title, windowOptions);
	}
	
	void createRenderSystem(RenderSystem::Options options)
	{
		april::rendersys->create(options);
	}
	
	void createWindow(int w, int h, bool fullscreen, chstr title, Window::Options options)
	{
		april::window->create(w, h, fullscreen, title, options);
		april::rendersys->assignWindow(april::window);
		// one special hack for one special backend
#ifdef _WINUWP
		april::rendersys->setViewport(grect(0.0f, 0.0f, (float)w, (float)h));
#endif
		april::rendersys->getCaps(); // calling getCaps() is required here so it's initialized on certain platforms
		april::rendersys->clear(); // initial clear backbuffer
	}

	void destroy()
	{
		if (april::rendersys != NULL || april::window != NULL)
		{
			hlog::write(logTag, "Destroying APRIL.");
		}
		if (april::window != NULL)
		{
			april::window->unassign();
		}
		if (april::window != NULL && april::rendersys != NULL)
		{
			april::rendersys->destroy();
			april::window->destroy();
		}
		if (april::window != NULL)
		{
			delete april::window;
			april::window = NULL;
		}
		if (april::rendersys != NULL)
		{
			delete april::rendersys;
			april::rendersys = NULL;
		}
#ifdef _EGL
		if (april::egl != NULL)
		{
			delete april::egl;
			april::egl = NULL;
		}
#endif
	}
	
	void addTextureExtension(chstr extension)
	{
		april::extensions += extension;
	}
	
	harray<hstr> getTextureExtensions()
	{
		return april::extensions;
	}
	
	void setTextureExtensions(const harray<hstr>& extensions)
	{
		april::extensions = extensions;
	}

	int getMaxAsyncTextureUploadsPerFrame()
	{
		return maxAsyncTextureUploadsPerFrame;
	}
	
	void setMaxAsyncTextureUploadsPerFrame(int value)
	{
		maxAsyncTextureUploadsPerFrame = value;
	}

	int getMaxWaitingAsyncTextures()
	{
		return maxWaitingAsyncTextures;
	}

	void setMaxWaitingAsyncTextures(int value)
	{
		maxWaitingAsyncTextures = value;
	}

	void addSymbolicColor(chstr symbolicName, const april::Color& color)
	{
		symbolicColors[symbolicName] = color;
	}

	bool findSymbolicColor(chstr symbolicName, april::Color& color)
	{
		if (symbolicColors.hasKey(symbolicName))
		{
			color = symbolicColors[symbolicName];
			return true;
		}
		return false;
	}

	bool findSymbolicColor(chstr symbolicName, hstr& hexColor)
	{
		if (symbolicColors.hasKey(symbolicName))
		{
			hexColor = symbolicColors[symbolicName].hex();
			return true;
		}
		return false;
	}

}
