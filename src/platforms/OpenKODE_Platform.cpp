/// @file
/// @version 4.4
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _OPENKODE
#include <KD/kd.h>
#include <stdio.h>

#define __HL_INCLUDE_PLATFORM_HEADERS
#include <gtypes/Vector2.h>
#include <hltypes/hlog.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hmap.h>
#include <hltypes/hplatform.h>
#include <hltypes/hstring.h>

#include "april.h"

#ifdef _EGL
	#include "egl.h"
#endif
#include "Platform.h"
#include "RenderSystem.h"
#include "Window.h"

#ifdef __APPLE__
	#include <TargetConditionals.h>
	#include <sys/sysctl.h>
	#ifdef _IOS
		#import <UIKit/UIKit.h>
		void getStaticiOSInfo(chstr name, april::SystemInfo& info);
	#else
		#import <AppKit/AppKit.h>
	#endif
#endif

#ifdef _ANDROID
	#include <jni.h>
	#define __NATIVE_INTERFACE_CLASS "com/april/NativeInterface"
	#include "androidUtilJNI.h"
	#include <unistd.h>
#endif

#ifdef _ANDROID
	#define debug_log(s) hlog::write(logTag, s)
#else
	#define debug_log(s) 
#endif

namespace april
{
	void _setupSystemInfo_platform(SystemInfo& info)
	{
		if (info.locale == "")
		{
			info.name = "KD";
			const char* envName = kdGetenv("COMPUTERNAME");
			if (envName != NULL)
			{
				info.deviceName = envName;
			}
			else
			{
				info.deviceName = "Unknown";
			}
			debug_log("Fetching OpenKODE system info");
			// number of CPU cores
			info.cpuCores = 1;
			// display resolution
			int width = 0;
			int height = 0;
			debug_log("getting screen info");
			kdQueryAttribi(KD_ATTRIB_WIDTH, (KDint*)&width);
			kdQueryAttribi(KD_ATTRIB_HEIGHT, (KDint*)&height);
			info.displayResolution.set((float)hmax(width, height), (float)hmin(width, height));
			// display DPI
			int dpi = 0;
			kdQueryAttribi(KD_ATTRIB_DPI, (KDint*)&dpi);
			info.displayDpi = (float)dpi;
			// name
#ifdef _IOS // On iOS, april prefers to use hardcoded device info than OpenKODE's info, it's more accurate
			size_t size = 255;
			char cname[256] = { '\0' };
			sysctlbyname("hw.machine", cname, &size, NULL, 0);
			info.name = cname; // defaults for unknown devices
#elif defined(__APPLE__) && defined(_PC_INPUT) // mac
			info.name = "mac";
#elif defined(_WINRT)
			info.name = "winrt";
#elif defined(_ANDROID)
			info.name = "android";
#elif defined(_WIN32)
			info.name = "Windows";
#endif
			// misc
#ifdef _IOS // On iOS, april prefers to use hardcoded device info than OpenKODE's info, it's more accurate
			hstr model = kdQueryAttribcv(KD_ATTRIB_PLATFORM);
			if (model.contains("(") && model.contains(")"))
			{
				hstr a, b;
				model.split("(", a, b);
				b.split(")", model, a);
				getStaticiOSInfo(model, info);
			}
#elif defined(__APPLE__) && defined(_PC_INPUT) // mac
			info.cpuCores = (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(_WINRT)
			SYSTEM_INFO w32info;
			GetNativeSystemInfo(&w32info);
			info.cpuCores = w32info.dwNumberOfProcessors;
			info.osVersion.set(8, 1);
#elif defined(_ANDROID)
		// CPU cores
			debug_log("getting cpu cores");
			info.cpuCores = sysconf(_SC_NPROCESSORS_CONF);
			// OS version
			debug_log("getting java stuff");
			APRIL_GET_NATIVE_INTERFACE_CLASS(classNativeInterface);
			debug_log("getting os version");
			jmethodID methodGetOsVersion = env->GetStaticMethodID(classNativeInterface, "getOsVersion", _JARGS(_JSTR, ));
			info.osVersion.set(_JSTR_TO_HSTR((jstring)env->CallStaticObjectMethod(classNativeInterface, methodGetOsVersion)));
#endif
			// RAM size
#if TARGET_IPHONE_SIMULATOR
			info.ram = 1024;
#elif defined(__APPLE__) && defined(_PC_INPUT) // mac
			int mib[] = { CTL_HW, HW_MEMSIZE };
			int64_t value = 0;
			size_t length = sizeof(value);
			if (sysctl(mib, 2, &value, &length, NULL, 0) != -1)
			{
				info.ram = (int)(value / (1024 * 1024));
			}
			else
			{
				info.ram = 2048;
			}
#else
			int pageSize;
			int pageCount;
			kdQueryAttribi(KD_ATTRIB_PAGESIZE, (KDint*)&pageSize);
			kdQueryAttribi(KD_ATTRIB_NUMPAGES, (KDint*)&pageCount);
#ifdef _WINRT
#ifndef _WINP8
			info.ram = (int)((int64)pageSize * pageCount / (1024 * 1024)); // in MB
#else
			// WinP8 reports 75% of the RAM's size as available
			info.ram = (int)(Windows::System::MemoryManager::AppMemoryUsageLimit / (1024 * 1024) * 4 / 3); // in MB
#endif
#else
			info.ram = (int)((int64_t)pageSize * pageCount / (1024 * 1024)); // in MB
#endif
#endif
		// other
			debug_log("getting locale");
			info.locale = "en"; // default is "en"
			hstr fullLocale = hstr(kdGetLocale());
			if (fullLocale.contains("-"))
			{
				fullLocale.split("-", info.locale, info.localeVariant);
			}
			else if (fullLocale.contains("_"))
			{
				fullLocale.split("_", info.locale, info.localeVariant);
			}
			else
			{
				info.locale = fullLocale;
			}
			info.locale = info.locale.lowered();
			info.localeVariant = info.localeVariant.uppered();
#ifdef _ANDROID
			env->PopLocalFrame(NULL);
#endif
		}
	}

	hstr _getPackageName_platform()
	{
#ifndef _WINRT
		return hstr(kdGetenv("KD_APP_ID"));
#else
		return _HL_PSTR_TO_HSTR(Windows::ApplicationModel::Package::Current->Id->FamilyName);
#endif
	}

	hstr _getUserDataPath_platform()
	{
		return "data";
	}
	
	int64_t _getRamConsumption_platform()
	{
		// TODOa
		return 0LL;
	}	
	
	bool _openUrl_platform(chstr url)
	{
#ifdef __APPLE__
#ifdef _IOS
		if ([[UIApplication sharedApplication] canOpenURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.cStr()]]])
		{
			[[UIApplication sharedApplication] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.cStr()]]];
		}
		else
		{
			hlog::write(logTag, "Failed to open URL");
			return false;
		}
#else
		[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url.cStr()]]];
#endif
#elif defined(_ANDROID)
		APRIL_GET_NATIVE_INTERFACE_METHOD(classNativeInterface, methodOpenUrl, "openUrl", _JARGS(_JVOID, _JSTR));
		env->CallStaticObjectMethod(classNativeInterface, methodOpenUrl, env->NewStringUTF(url.cStr()));
		env->PopLocalFrame(NULL);
#elif defined(_WIN32) && !defined(_WINRT)
		ShellExecuteW(NULL, L"open", url.wStr().c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
		kdSystem((KDchar*)url.cStr());
#endif
		return true;
	}
	
	void _showMessageBox_platform(chstr title, chstr text, MessageBoxButton buttons, MessageBoxStyle style,
		hmap<MessageBoxButton, hstr> customButtonTitles, void (*callback)(MessageBoxButton), bool modal)
	{
		hstr ok;
		hstr yes;
		hstr no;
		hstr cancel;
		_makeButtonLabels(&ok, &yes, &no, &cancel, buttons, customButtonTitles);
		const char* kdButtons[4] = {"", NULL, NULL, NULL};
		MessageBoxButton resultButtons[4] = { MessageBoxButton::Ok, MessageBoxButton::Ok, MessageBoxButton::Ok, MessageBoxButton::Ok };
		int indexCancel = -1;
		if (buttons == MessageBoxButton::OkCancel)
		{
			// order is reversed because libKD prefers the colored button to be at place [1], at least on iOS
			// if this is going to be changed for a new platform, ifdef the button order for iOS
			kdButtons[1] = ok.cStr();
			kdButtons[0] = cancel.cStr();
			resultButtons[1] = MessageBoxButton::Ok;
			resultButtons[0] = MessageBoxButton::Cancel;
			indexCancel = 0;
		}
		else if (buttons == MessageBoxButton::YesNoCancel)
		{
			kdButtons[1] = yes.cStr();
			kdButtons[0] = no.cStr();
			kdButtons[2] = cancel.cStr();
			resultButtons[1] = MessageBoxButton::Yes;
			resultButtons[0] = MessageBoxButton::No;
			resultButtons[2] = MessageBoxButton::Cancel;
			indexCancel = 2;
		}
		else if (buttons == MessageBoxButton::Ok)
		{
			kdButtons[0] = ok.cStr();
			resultButtons[0] = MessageBoxButton::Ok;
			indexCancel = 0;
		}
		else if (buttons == MessageBoxButton::YesNo)
		{
			kdButtons[1] = yes.cStr();
			kdButtons[0] = no.cStr();
			resultButtons[1] = MessageBoxButton::Yes;
			resultButtons[0] = MessageBoxButton::No;
			indexCancel = 1;
		}
		int index = kdShowMessage(title.cStr(), text.cStr(), kdButtons);
		if (index == -1)
		{
			index = indexCancel;
		}
		if (callback != NULL && index >= 0)
		{
			(*callback)(resultButtons[index]);
		}
	}

}
#endif
