/// @file
/// @version 4.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifndef __ANDROID__
	#ifndef _UWP
		#define RESOURCE_PATH "../../demos/media/"
	#else
		#define RESOURCE_PATH "media/"
	#endif
#elif defined(__APPLE__)
	#define RESOURCE_PATH "media/"
#else
	#define RESOURCE_PATH "./"
#endif

#include <stdlib.h>

#include <april/april.h>
#include <april/Cursor.h>
#include <april/KeyDelegate.h>
#include <april/main.h>
#include <april/MouseDelegate.h>
#include <april/Platform.h>
#include <april/RenderSystem.h>
#include <april/SystemDelegate.h>
#include <april/UpdateDelegate.h>
#include <april/Window.h>
#include <gtypes/Rectangle.h>
#include <hltypes/hlog.h>
#include <hltypes/hstring.h>

#define LOG_TAG "demo_helloworld"

april::Cursor* cursor = NULL;
april::TexturedVertex v[4];

#if !defined(__ANDROID__) && !defined(_IOS) && !defined(_WINP8)
grectf drawRect(0.0f, 0.0f, 800.0f, 600.0f);
#else
grectf drawRect(0.0f, 0.0f, 480.0f, 320.0f);
#endif
grectf backgroundRect(50.0f, 50.0f, drawRect.w - 100.0f, drawRect.h - 100.0f);
gvec2f size = drawRect.getSize() * 5 / 16;

class Ball
{
public:
	april::Texture* texture;

	Ball(april::Texture* texture)
	{
		this->texture = texture;
		this->position.set((float)hrand((int)drawRect.w - size), (float)hrand((int)drawRect.h - size));
		this->velocity.set((float)speed, (float)speed);
	}

	void update(float timeDelta)
	{
		this->position += this->velocity * timeDelta;

		if (this->position.x < 0 || this->position.x > drawRect.w - size)
		{
			this->position -= this->velocity * timeDelta;
			this->velocity.x = -this->velocity.x;
		}

		if (this->position.y < 0 || this->position.y > drawRect.h - size)
		{
			this->position -= this->velocity * timeDelta;
			this->velocity.y = -this->velocity.y;
		}
	}

	void render()
	{
		float x1 = this->position.x;
		float x2 = this->position.x + size;
		float y1 = this->position.y;
		float y2 = this->position.y + size;
		april::rendersys->setTexture(this->texture);
		v[0].x = x1; v[0].y = y1; v[0].z = 0; v[0].u = 0; v[0].v = 0;
		v[1].x = x2; v[1].y = y1; v[1].z = 0; v[1].u = 1; v[1].v = 0;
		v[2].x = x1; v[2].y = y2; v[2].z = 0; v[2].u = 0; v[2].v = 1;
		v[3].x = x2; v[3].y = y2; v[3].z = 0; v[3].u = 1; v[3].v = 1;
		april::rendersys->render(april::RenderOperation::TriangleStrip, v, 4);
	}

protected:
	gvec2f position;
	gvec2f velocity;

	static const int size = 96;
	static const int speed = 256;

};

harray<Ball> balls;

class UpdateDelegate : public april::UpdateDelegate
{
	bool onUpdate(float timeDelta) override
	{	
		april::rendersys->clear();
		april::rendersys->setOrthoProjection(drawRect);
		april::rendersys->drawFilledRect(drawRect, april::Color::Grey);
		april::rendersys->drawFilledRect(backgroundRect, april::Color::DarkGreen);
		foreach (Ball, it, balls)
		{
			it->update(timeDelta);
			it->render();
		}
		return true;
	}
};

class SystemDelegate : public april::SystemDelegate
{
public:
	SystemDelegate() : april::SystemDelegate()
	{
	}

	void onWindowSizeChanged(int width, int height, bool fullScreen) override
	{
		hlog::writef(LOG_TAG, "window size changed: %dx%d", width, height);
		april::rendersys->setViewport(drawRect);
	}

};

static UpdateDelegate* updateDelegate = NULL;
static SystemDelegate* systemDelegate = NULL;

void __aprilApplicationInit()
{
#ifdef __APPLE__
	// On MacOSX, the current working directory is not set by
	// the Finder, since you are expected to use Core Foundation
	// or ObjC APIs to find files. 
	// So, when porting you probably want to set the current working
	// directory to something sane (e.g. .../Resources/ in the app
	// bundle).
	// In this case, we set it to parent of the .app bundle.
	{	// curly braces in order to localize variables 

		CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());
		CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		// let's hope chdir() will be happy with utf8 encoding
		const char* cpath = CFStringGetCStringPtr(path, kCFStringEncodingUTF8);
		char* cpath_alloc = NULL;
		if (cpath == NULL)
		{
			// CFStringGetCStringPtr is allowed to return NULL. bummer.
			// we need to use CFStringGetCString instead.
			cpath_alloc = (char*)malloc(CFStringGetLength(path) + 1);
			CFStringGetCString(path, cpath_alloc, CFStringGetLength(path) + 1, kCFStringEncodingUTF8);
		}
		else
		{
			// even though it didn't return NULL, we still want to slice off bundle name.
			cpath_alloc = (char*)malloc(CFStringGetLength(path) + 1);
			strcpy(cpath_alloc, cpath);
		}
		// just in case / is appended to .app path for some reason
		if (cpath_alloc[CFStringGetLength(path) - 1] == '/')
		{
			cpath_alloc[CFStringGetLength(path) - 1] = 0;
		}
		// replace pre-.app / with a null character, thus
		// cutting off .app's name and getting parent of .app.
		strrchr(cpath_alloc, '/')[0] = 0;
		// change current dir using posix api
		chdir(cpath_alloc);
		free(cpath_alloc); // even if null, still ok
		CFRelease(path);
		CFRelease(url);
	}
#endif
	srand((unsigned int)htime());
	updateDelegate = new UpdateDelegate();
	systemDelegate = new SystemDelegate();	
#if defined(__ANDROID__) || defined(_IOS)
	drawRect.setSize(april::getSystemInfo().displayResolution);
#endif
	april::init(april::RenderSystemType::Default, april::WindowType::Default);
	april::createRenderSystem();
	april::Window::Options windowOptions;
	windowOptions.resizable = true;
	april::createWindow((int)drawRect.w, (int)drawRect.h, false, "APRIL: Hello World Demo", windowOptions);
#ifdef _UWP
	april::window->setParam("cursor_mappings", "101 " RESOURCE_PATH "cursor\n102 " RESOURCE_PATH "simple");
#endif
	april::window->setUpdateDelegate(updateDelegate);
	april::window->setSystemDelegate(systemDelegate);
	cursor = april::window->createCursorFromResource(RESOURCE_PATH "cursor");
	april::window->setCursor(cursor);
	april::Texture* texture = april::rendersys->createTextureFromResource(RESOURCE_PATH "logo");
	balls.add(Ball(texture));
	texture = april::rendersys->createTextureFromResource(RESOURCE_PATH "x");
	balls.add(Ball(texture));
}

void __aprilApplicationDestroy()
{
	april::window->setCursor(NULL);
	april::window->destroyCursor(cursor);
	cursor = NULL;
	foreach (Ball, it, balls)
	{
		april::rendersys->destroyTexture((*it).texture);
	}
	balls.clear();
	april::destroy();
	delete systemDelegate;
	systemDelegate = NULL;
	delete updateDelegate;
	updateDelegate = NULL;
}
