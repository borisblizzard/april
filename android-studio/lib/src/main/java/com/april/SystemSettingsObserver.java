package com.april;

/// @version 5.2

import android.content.pm.ActivityInfo;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings.System;

public class SystemSettingsObserver extends ContentObserver
{
	public SystemSettingsObserver()
	{
		super(new Handler(Looper.getMainLooper()));
	}
	
	@Override
	public boolean deliverSelfNotifications()
	{
		return true;
	}

	@Override
	public void onChange(boolean selfChange, Uri uri)
	{
		// enable auto-rotation
		if (System.getInt(NativeInterface.activity.getContentResolver(), System.ACCELEROMETER_ROTATION, 0) != 0)
		{
			if (NativeInterface.activity.getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE)
			{
				NativeInterface.activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
			}
			else if (NativeInterface.activity.getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT)
			{
				NativeInterface.activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
			}
		}
		else
		{
			if (NativeInterface.activity.getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE)
			{
				NativeInterface.activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
			}
			else if (NativeInterface.activity.getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT)
			{
				NativeInterface.activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
			}
		}
	}

	@Override
	public void onChange(boolean selfChange)
	{
		this.onChange(selfChange, null);
	}
	
};
