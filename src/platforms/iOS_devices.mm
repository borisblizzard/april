/// @file
/// @version 5.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#include "Platform.h"
#import <UIKit/UIKit.h>

void getStaticiOSInfo(chstr deviceName, april::SystemInfo& info)
{
	int w = info.displayResolution.x;
	int h = info.displayResolution.y;
	// for future reference, look here: https://everymac.com/systems/apple/iphone/index-iphone-specs.html
	// and here: https://everymac.com/systems/apple/ipad/index-ipad-specs.html
	// and here: http://www.paintcodeapp.com/news/ultimate-guide-to-iphone-resolutions

	if (deviceName.startsWith("iPad"))
	{
		if (deviceName.startsWith("iPad1,"))
		{
			info.deviceName = "iPad 1";
			info.displayDpi = 132;
		}
		else if (deviceName.startsWith("iPad2,"))
		{
			if (deviceName == "iPad2,5" || deviceName == "iPad2,6" || deviceName == "iPad2,7") // iPad Mini
			{
				info.deviceName = "iPad Mini";
				info.displayDpi = 163;
			}
			else
			{
				info.deviceName = "iPad 2";
				info.displayDpi = 132;
			}
		}
		else if (deviceName.startsWith("iPad3,"))
		{
			if (deviceName == "iPad3,4" || deviceName == "iPad3,5" || deviceName == "iPad3,6") // iPad 4
			{
				info.deviceName = "iPad 4";
			}
			else
			{
				info.deviceName = "iPad 3";
			}
			info.displayDpi = 264;
		}
		else if (deviceName.startsWith("iPad4,"))
		{
			if (deviceName == "iPad4,4" || deviceName == "iPad4,5" || deviceName == "iPad4,6") // iPad Mini 2
			{
				info.deviceName = "iPad Mini 2";
				info.displayDpi = 326;
			}
			else if (deviceName == "iPad4,7" || deviceName == "iPad4,8" || deviceName == "iPad4,9") // iPad Mini 3
			{
				info.deviceName = "iPad Mini 3";
				info.displayDpi = 326;
			}
			else
			{
				info.deviceName = "iPad Air";
				info.displayDpi = 264;
			}
		}
		else if (deviceName.startsWith("iPad5,"))
		{
			info.deviceName = "iPad Air 2";
			info.displayDpi = 264;
			if (deviceName == "iPad5,1" || deviceName == "iPad5,2") // iPad Mini 4
			{
				info.deviceName = "iPad Mini 4";
				info.displayDpi = 326;
			}
		}
		else if (deviceName.startsWith("iPad6,"))
		{
			info.deviceName = "iPad Pro";
			info.displayDpi = 264;
			if (deviceName == "iPad6,11" || deviceName == "iPad6,12") // 9.7"
			{
				info.deviceName = "iPad 5";
			}
		}
		else if (deviceName.startsWith("iPad7,"))
		{
			info.deviceName = "iPad Pro";
			info.displayDpi = 264;
			if (deviceName == "iPad7,5" || deviceName == "iPad7,6")
			{
				info.deviceName = "iPad 6";
			}
			else if (deviceName == "iPad7,11" || deviceName == "iPad7,12")
			{
				info.deviceName = "iPad 7";
			}
		}
		else if (deviceName.startsWith("iPad8,"))
		{
			info.deviceName = "iPad Pro";
			info.displayDpi = 264;
		}
		else if (deviceName.startsWith("iPad11,"))
		{
			info.deviceName = "iPad Air 3";
			info.displayDpi = 264;
			if (deviceName == "iPad11,1" || deviceName == "iPad11,2") // iPad Mini 5
			{
				info.deviceName = "iPad Mini 5";
				info.displayDpi = 326;
			}
		}
		else
		{
			info.deviceName = "iPad ??? (" + deviceName + ")";
			info.displayDpi = 264;
		}
	}
	else if (deviceName.startsWith("iPhone"))
	{
		if (deviceName == "iPhone1,1")
		{
			info.deviceName = "iPhone 2G";
			info.displayDpi = 163;
		}
		else if (deviceName == "iPhone1,2")
		{
			info.deviceName = "iPhone 3G";
			info.displayDpi = 163;
		}
		else if (deviceName == "iPhone2,1")
		{
			info.deviceName = "iPhone 3GS";
			info.displayDpi = 163;
		}
		else if (deviceName.startsWith("iPhone3,"))
		{
			info.deviceName = "iPhone 4";
			info.displayDpi = 326;
		}
		else if (deviceName.startsWith("iPhone4,"))
		{
			info.deviceName = "iPhone 4S";
			info.displayDpi = 326;
		}
		else if (deviceName.startsWith("iPhone5,"))
		{
			info.displayDpi = 326;
			if (deviceName == "iPhone5,1" || deviceName == "iPhone5,2")
			{
				info.deviceName = "iPhone 5";
			}
			else
			{
				info.deviceName = "iPhone 5C";
			}
		}
		else if (deviceName.startsWith("iPhone6,"))
		{
			info.deviceName = "iPhone 5S";
			info.displayDpi = 326;
		}
		else if (deviceName.startsWith("iPhone7,"))
		{
			if (deviceName.startsWith("iPhone7,2"))
			{
				info.deviceName = "iPhone 6";
				info.displayDpi = 326;
				if ([[UIScreen mainScreen] nativeScale] != 2)
				{
					info.deviceName += " Zoomed_4inch";
					info.displayDpi = 316;
				}
			}
			else
			{
				// iPhone 6+ has a resolution of 2208x1242 but is downscaled to 1920x1080
				// The physical DPI is 401, but because of this, it is better to use the upscaled equivalent DPI of 461
				// we also need to account for possible zoomed mode. there can be 2 zoomed modes, one for compatibility upscaling iPhone 5 resolution
				// and one as a feature, upscaling iPhone 6 resolution
				info.deviceName = "iPhone 6+";
				info.displayDpi = 461;
				if (w == 1704)
				{
					info.deviceName += " Zoomed_4inch";
					info.displayDpi = 355;
				}
				else if (w == 2001)
				{
					info.deviceName += " Zoomed_4.7inch";
					info.displayDpi = 417;
				}
			}
		}
		else if (deviceName.startsWith("iPhone8,"))
		{
			if (deviceName.startsWith("iPhone8,1"))
			{
				info.deviceName = "iPhone 6S";
				info.displayDpi = 326;
				if ([[UIScreen mainScreen] nativeScale] != 2)
				{
					info.deviceName += " Zoomed_4inch";
					info.displayDpi = 316;
				}
			}
			else if (deviceName.startsWith("iPhone8,2"))
			{
				info.deviceName = "iPhone 6S+";
				info.displayDpi = 461;
				if (w == 1704)
				{
					info.deviceName += " Zoomed_4inch";
					info.displayDpi = 355;
				}
				else if (w == 2001)
				{
					info.deviceName += " Zoomed_4.7inch";
					info.displayDpi = 417;
				}
			}
			else
			{
				info.deviceName = "iPhone SE";
				info.displayDpi = 326;
			}
		}
		else if (deviceName.startsWith("iPhone9,"))
		{
			if (deviceName == "iPhone9,1" || deviceName == "iPhone9,3")
			{
				info.deviceName = "iPhone 7";
				info.displayDpi = 326;
				if ([[UIScreen mainScreen] nativeScale] != 2)
				{
					info.deviceName += " Zoomed_4inch";
					info.displayDpi = 316;
				}
			}
			else
			{
				info.deviceName = "iPhone 7+";
				info.displayDpi = 461;
				if (w == 1704)
				{
					info.deviceName += " Zoomed_4inch";
					info.displayDpi = 355;
				}
				else if (w == 2001)
				{
					info.deviceName += " Zoomed_4.7inch";
					info.displayDpi = 417;
				}
			}
		}
		else if (deviceName.startsWith("iPhone10,"))
		{
			if (deviceName == "iPhone10,1" || deviceName == "iPhone10,4")
			{
				info.deviceName = "iPhone 8";
				info.displayDpi = 326;
			}
			else if (deviceName == "iPhone10,2" || deviceName == "iPhone10,5")
			{
				info.deviceName = "iPhone 8+";
				info.displayDpi = 461;
			}
			else
			{
				info.deviceName = "iPhone X";
				info.displayDpi = 458;
			}
		}
		else if (deviceName.startsWith("iPhone11,"))
		{
			if (deviceName == "iPhone11,8")
			{
				info.deviceName = "iPhone XR";
				info.displayDpi = 326;
			}
			else if (deviceName == "iPhone11,2")
			{
				info.deviceName = "iPhone Xs";
				info.displayDpi = 458;
			}
			else
			{
				info.deviceName = "iPhone Xs Max";
				info.displayDpi = 458;
			}
		}
		else if (deviceName.startsWith("iPhone12,"))
		{
			if (deviceName == "iPhone12,5")
			{
				info.deviceName = "iPhone 11 Pro Max";
				info.displayDpi = 458;
			}
			else if (deviceName == "iPhone12,3")
			{
				info.deviceName = "iPhone 11 Pro";
				info.displayDpi = 458;
			}
			else
			{
				info.deviceName = "iPhone 11";
				info.displayDpi = 458;
			}
		}
		else
		{
			info.deviceName = "iPhone ??? (" + deviceName + ")";
			info.displayDpi = 326;
		}
	}
	else if (deviceName.startsWith("iPod"))
	{
		if (deviceName == "iPod1,1")
		{
			info.deviceName = "iPod Touch";
			info.displayDpi = 163;
		}
		else if (deviceName == "iPod2,1")
		{
			info.deviceName = "iPod Touch 2";
			info.displayDpi = 163;
		}
		else if (deviceName == "iPod3,1")
		{
			info.deviceName = "iPod Touch 3";
			info.displayDpi = 163;
		}
		else if (deviceName == "iPod4,1")
		{
			info.deviceName = "iPod Touch 4";
			info.displayDpi = 326;
		}
		else if (deviceName == "iPod5,1")
		{
			info.deviceName = "iPod Touch 5";
			info.displayDpi = 326;
		}
		else if (deviceName == "iPod7,1")
		{
			info.deviceName = "iPod Touch 6";
			info.displayDpi = 326;
		}
		else if (deviceName == "iPod9,1")
		{
			info.deviceName = "iPod Touch 7";
			info.displayDpi = 326;
		}
		else
		{
			info.deviceName = "iPod ??? (" + deviceName + ")";
			info.displayDpi = 326;
		}
	}
	else if (deviceName.startsWith("x86")) // iPhone Simulator
	{
		if ((float)w / h >= 1.5f) // iPhone
		{
			if (w == 480)
			{
				info.deviceName = "iPhone 3GS";
				info.displayDpi = 163;
			}
			else if (w == 960)
			{
				info.deviceName = "iPhone 4";
				info.displayDpi = 326;
			}
			else if (w == 1136)
			{
				if ([[UIScreen mainScreen] nativeScale] != 2)
				{
					info.deviceName = "iPhone 6 Zoomed_4inch";
					info.displayDpi = 316;
				}
				else
				{
					info.deviceName = "iPhone 5";
					info.displayDpi = 326;
				}
			}
			else if (w == 1334)
			{
				info.deviceName = "iPhone 6";
				info.displayDpi = 326;
			}
			else if (w == 2208)
			{
				info.deviceName = "iPhone 6 Plus";
				info.displayDpi = 461;
			}
			else if (w == 1704)
			{
				info.deviceName = "iPhone 6+ Zoomed_4inch";
				info.displayDpi = 355;
			}
			else if (w == 2001)
			{
				info.deviceName = "iPhone 6+ Zoomed_4.7inch";
				info.displayDpi = 417;
			}
			else if (w == 2436)
			{
				info.deviceName = "iPhone X";
				info.displayDpi = 458;
			}
			else if (w == 1792)
			{
				info.deviceName = "iPhone 11";
				info.displayDpi = 458;
			}
			else if (w == 2436)
			{
				info.deviceName = "iPhone 11 Pro";
				info.displayDpi = 458;
			}
			else if (w == 2688)
			{
				info.deviceName = "iPhone 11 Pro Max";
				info.displayDpi = 458;
			}
		}
		else
		{
			if (h == 768)
			{
				info.deviceName = "iPad 2";
				info.displayDpi = 132;
			}
			else if (h == 1536)
			{
				info.deviceName = "iPad 3";
				info.displayDpi = 264;
			}
			else
			{
				info.deviceName = "iPad Pro";
				info.displayDpi = 264;
			}
		}
	}
	//else: i386 (iphone simulator) and possible future device types
}
