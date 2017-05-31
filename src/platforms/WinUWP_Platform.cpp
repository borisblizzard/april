/// @file
/// @version 4.3
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#if defined(_WINUWP) && !defined(_OPENKODE)
#define __HL_INCLUDE_PLATFORM_HEADERS
#include <gtypes/Vector2.h>
#include <hltypes/hdir.h>
#include <hltypes/hlog.h>
#include <hltypes/hltypesUtil.h>
#include <hltypes/hmap.h>
#include <hltypes/hmutex.h>
#include <hltypes/hplatform.h>
#include <hltypes/hstring.h>

#include "april.h"
#include "Platform.h"
#include "RenderSystem.h"
#include "Window.h"
#include "WinUWP.h"
#include "WinUWP_Window.h"

using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::UI::Popups;
using namespace Windows::UI::ViewManagement;

namespace april
{
	void _setupSystemInfo_platform(SystemInfo& info)
	{
		if (info.locale == "")
		{
			info.name = "winrt";
			info.deviceName = "unnamedWinUWPDevice";
#ifdef _ARM
			info.architecture = "ARM";
#else
			info.architecture = "x86";
#endif
			// number of CPU cores
			SYSTEM_INFO w32info;
			GetNativeSystemInfo(&w32info);
			info.cpuCores = w32info.dwNumberOfProcessors;
			// RAM size
#ifndef _WINP8
			// pure WinUWP can't retrieve this information so some arbitrary value is used
#ifndef _ARM
			info.ram = 2048;
#else
			info.ram = 1536;
#endif
#else
			// WinP8 reports 75% of the RAM's size as available
			info.ram = (int)(Windows::System::MemoryManager::AppMemoryUsageLimit / (1024 * 1024) * 4 / 3); // in MB
#endif
			// other
			info.locale = "en"; // default is "en"
			IIterator<Platform::String^>^ it = Windows::Globalization::ApplicationLanguages::Languages->First();
			if (it->HasCurrent)
			{
				hstr fullLocale = _HL_PSTR_TO_HSTR(it->Current);
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
			}
			info.osVersion.set(6, 3);
			//info.osVersion.set(10, 0); // sooooooon...
		}
		DisplayInformation^ displayInfo = DisplayInformation::GetForCurrentView();
		// display DPI
		info.displayDpi = displayInfo->RawDpiY;
		if (info.displayDpi < 0.01f)
		{
			static bool displayDpiLogged = false;
			if (!displayDpiLogged)
			{
				hlog::warn(logTag, "Cannot get raw display DPI, trying logical DPI.");
			}
			info.displayDpi = displayInfo->LogicalDpi;
			if (info.displayDpi < 0.01f)
			{
				if (!displayDpiLogged)
				{
					hlog::warn(logTag, "Cannot get logical display DPI, defaulting to 96.");
				}
				info.displayDpi = 96.0f;
			}
			displayDpiLogged = true;
		}
#ifdef _WINUWP_WINDOW
		// display resolution
		float dpiRatio = WinUWP::getDpiRatio();
		CoreWindow^ window = CoreWindow::GetForCurrentThread();
		int width = hround(window->Bounds.Width * dpiRatio);
		int height = hround(window->Bounds.Height * dpiRatio);
		// these orientations are not supported in APRIL, but Windows allows them anyway even if the manifest says that they aren't supported
		if (displayInfo->CurrentOrientation == DisplayOrientations::Portrait ||
			displayInfo->CurrentOrientation == DisplayOrientations::PortraitFlipped)
		{
			hswap(width, height);
		}
		if (info.displayResolution.y == 0.0f)
		{
			info.displayResolution.set((float)width, (float)height);
		}
		else
		{
			info.displayResolution.x = hmax((float)width, info.displayResolution.x);
		}
#endif
	}

	hstr _getPackageName_platform()
	{
		return _HL_PSTR_TO_HSTR(Windows::ApplicationModel::Package::Current->Id->FamilyName);
	}

	hstr _getUserDataPath_platform()
	{
		return hdir::systemize(_HL_PSTR_TO_HSTR(ApplicationData::Current->RoamingFolder->Path));
	}
	
	int64_t _getRamConsumption_platform()
	{
		// TODOa
		hlog::warn(logTag, "Cannot use getRamConsumption() on this platform.");
		return 0LL;
	}	
	
	bool _openUrl_platform(chstr url)
	{
		hlog::write(logTag, "Opening URL: " + url);
		Windows::System::Launcher::LaunchUriAsync(ref new Windows::Foundation::Uri(_HL_HSTR_TO_PSTR(url)));
		return true;
	}

	static void(*currentCallback)(MessageBoxButton) = NULL;

	void _showMessageBoxResult(int button)
	{
		switch (button)
		{
		case IDOK:
			if (currentCallback != NULL)
			{
				(*currentCallback)(MessageBoxButton::Ok);
			}
			break;
		case IDYES:
			if (currentCallback != NULL)
			{
				(*currentCallback)(MessageBoxButton::Yes);
			}
			break;
		case IDNO:
			if (currentCallback != NULL)
			{
				(*currentCallback)(MessageBoxButton::No);
			}
			break;
		case IDCANCEL:
			if (currentCallback != NULL)
			{
				(*currentCallback)(MessageBoxButton::Cancel);
			}
			break;
		default:
			hlog::error(logTag, "Unknown message box callback: " + hstr(button));
			break;
		}
	}

	static harray<DispatchedHandler^> messageBoxQueue;
	static hmutex messageBoxQueueMutex;

	void _showMessageBox_platform(chstr title, chstr text, MessageBoxButton buttons, MessageBoxStyle style,
		hmap<MessageBoxButton, hstr> customButtonTitles, void (*callback)(MessageBoxButton), bool modal)
	{
		DispatchedHandler^ handler = ref new DispatchedHandler(
			[title, text, buttons, style, customButtonTitles, callback]()
		{
			currentCallback = callback;
			_HL_HSTR_TO_PSTR_DEF(text);
			_HL_HSTR_TO_PSTR_DEF(title);
			MessageDialog^ dialog = ref new MessageDialog(ptext, ptitle);
			UICommandInvokedHandler^ commandHandler = ref new UICommandInvokedHandler(
				[](IUICommand^ command)
			{
				_showMessageBoxResult((int)command->Id);
				DispatchedHandler^ handler = nullptr;
				messageBoxQueueMutex.lock();
				if (messageBoxQueue.size() > 0)
				{
					handler = messageBoxQueue.removeFirst();
				}
				messageBoxQueueMutex.unlock();
				if (handler != nullptr)
				{
					CoreWindow::GetForCurrentThread()->Dispatcher->RunAsync(CoreDispatcherPriority::Normal, handler);
				}
			});
			hstr ok;
			hstr yes;
			hstr no;
			hstr cancel;
			_makeButtonLabels(&ok, &yes, &no, &cancel, buttons, customButtonTitles);
			_HL_HSTR_TO_PSTR_DEF(ok);
			_HL_HSTR_TO_PSTR_DEF(yes);
			_HL_HSTR_TO_PSTR_DEF(no);
			_HL_HSTR_TO_PSTR_DEF(cancel);

			if (buttons == MessageBoxButton::OkCancel)
			{
				dialog->Commands->Append(ref new UICommand(pok, commandHandler, IDOK));
				dialog->Commands->Append(ref new UICommand(pcancel, commandHandler, IDCANCEL));
				dialog->DefaultCommandIndex = 0;
				dialog->CancelCommandIndex = 1;
			}
			else if (buttons == MessageBoxButton::YesNoCancel)
			{
				dialog->Commands->Append(ref new UICommand(pyes, commandHandler, IDYES));
				dialog->Commands->Append(ref new UICommand(pno, commandHandler, IDNO));
				dialog->Commands->Append(ref new UICommand(pcancel, commandHandler, IDCANCEL));
				dialog->DefaultCommandIndex = 0;
				dialog->CancelCommandIndex = 2;
			}
			else if (buttons == MessageBoxButton::Ok)
			{
				dialog->Commands->Append(ref new UICommand(pok, commandHandler, IDOK));
				dialog->DefaultCommandIndex = 0;
				dialog->CancelCommandIndex = 0;
			}
			else if (buttons == MessageBoxButton::YesNo)
			{
				dialog->Commands->Append(ref new UICommand(pyes, commandHandler, IDYES));
				dialog->Commands->Append(ref new UICommand(pno, commandHandler, IDNO));
				dialog->DefaultCommandIndex = 0;
				dialog->CancelCommandIndex = 1;
			}
			dialog->ShowAsync();
		});
		try
		{
			handler->Invoke();
		}
		catch (Platform::AccessDeniedException^ e)
		{
			hlog::warn(logTag, "messagebox() on WinUWP called \"recursively\"! Queueing to UI thread now...");
			messageBoxQueueMutex.lock();
			messageBoxQueue += handler;
			messageBoxQueueMutex.unlock();
		}
	}

}
#endif