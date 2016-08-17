package com.april;

/// @version 4.0

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.ResultReceiver;
import android.view.inputmethod.InputMethodManager;
import android.view.View;
import android.util.DisplayMetrics;

import com.april.DialogFragment;

import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.lang.Math;
import java.lang.Runtime;
import java.util.Locale;

public class NativeInterface
{
	public static android.app.Activity activity = null;
	public static Activity aprilActivity = null;
	public static boolean running = false;
	public static boolean keyboardVisible = false;
	public static String archivePath = "";
	public static String dataPath = ".";
	public static String packageName = "";
	public static String versionCode = "0";
	public static String versionName = "";
	public static String apkPath = "";
	
	private static boolean htcKeyboardHack = false;
	private static class KeyboardResultReceiver extends ResultReceiver
	{
		public KeyboardResultReceiver()
		{
			super(new Handler());
		}
		
		@Override
		protected void onReceiveResult(int resultCode, Bundle resultData)
		{
			boolean keyboardVisible = true;
			if (resultCode == InputMethodManager.RESULT_UNCHANGED_HIDDEN || resultCode == InputMethodManager.RESULT_HIDDEN)
			{
				keyboardVisible = false;
			}
			if (keyboardVisible && NativeInterface.htcKeyboardHack)
			{
				NativeInterface.htcKeyboardHack = false;
				InputMethodManager inputMethodManager = NativeInterface._getInputMethodManager();
				View view = NativeInterface.aprilActivity.getView();
				inputMethodManager.hideSoftInputFromWindow(view.getWindowToken(), 0, NativeInterface.keyboardResultReceiver);
				inputMethodManager.showSoftInput(view, 0, NativeInterface.keyboardResultReceiver);
			}
		}
		
	};
	private static KeyboardResultReceiver keyboardResultReceiver = new KeyboardResultReceiver();
	
	public static native void setVariables(String dataPath, String forcedArchivePath);
	public static native void init(String[] args);
	public static native boolean render();
	public static native void destroy();
	public static native void onKeyDown(int keyCode, int charCode);
	public static native void onKeyUp(int keyCode);
	public static native void onTouch(int type, float x, float y, int index);
	public static native void onButtonDown(int controllerIndex, int buttonCode);
	public static native void onButtonUp(int controllerIndex, int buttonCode);
	public static native void onControllerAxisChange(int controllerIndex, int buttonCode, float axisValue);
	public static native void onWindowFocusChanged(boolean focused);
	public static native void onVirtualKeyboardChanged(boolean visible, float heightRation);
	public static native void onLowMemory();
	public static native void onSurfaceCreated();
	
	public static native void activityOnCreate();
	public static native void activityOnStart();
	public static native void activityOnResume();
	public static native void activityOnPause();
	public static native void activityOnStop();
	public static native void activityOnDestroy();
	public static native void activityOnRestart();
	
	public static native void onDialogOk();
	public static native void onDialogYes();
	public static native void onDialogNo();
	public static native void onDialogCancel();
	
	public static Object getDisplayResolution()
	{
		DisplayMetrics metrics = new DisplayMetrics();
		NativeInterface.activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
		int width = metrics.widthPixels;
		int height = metrics.heightPixels;
		if (NativeInterface.aprilActivity.isEnabledNavigationBarHiding())
		{
			// get the DecorView's size is status bar is being hidden
			android.graphics.Rect visibleFrame = new android.graphics.Rect();
			NativeInterface.activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(visibleFrame);
			width = visibleFrame.right;
			height = visibleFrame.bottom;
		}
		if (height > width)
		{
			int temp = height;
			height = width;
			width = temp;
		}
		// fixes problem with bottom 20 pixels being covered by Kindle Fire's menu
		if (Build.MANUFACTURER.equals("Amazon") && Build.MODEL.equals("Kindle Fire"))
		{
			height -= 20;
		}
		int[] result = {width, height};
		return result;
	}
	
	public static float getDisplayDpi()
	{
		// hardcoded exceptions for known devices that return wrong DPI
		if (Build.MANUFACTURER.equals("HTC") && Build.MODEL.equals("HTC One X"))
		{
			return 312.0f;
		}
		DisplayMetrics metrics = new DisplayMetrics();
		NativeInterface.activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
		return (float) Math.sqrt((metrics.xdpi * metrics.xdpi + metrics.ydpi * metrics.ydpi) / 2.0);
	}
	
	public static String getOsVersion()
	{
		return Build.VERSION.RELEASE;
	}
	
	public static String getLocale()
	{
		return Locale.getDefault().getLanguage();
	}
	
	public static String getLocaleVariant()
	{
		return Locale.getDefault().getCountry();
	}
	
	public static String getUserDataPath()
	{
		return NativeInterface.activity.getFilesDir().getAbsolutePath();
	}
	
	public static long getRamConsumption()
	{
		android.os.Debug.MemoryInfo info = new android.os.Debug.MemoryInfo();
		android.os.Debug.getMemoryInfo(info);
		return ((long)info.getTotalPrivateDirty() * 1024L); // because getTotalPrivateDirty() is in kB
	}
	
	public static void openUrl(String url)
	{
		NativeInterface.activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
	}
	
	public static void showVirtualKeyboard()
	{
		NativeInterface.activity.runOnUiThread(new Runnable()
		{
			public void run()
			{
				View view = NativeInterface.aprilActivity.getView();
				NativeInterface._getInputMethodManager().showSoftInput(view, 0, NativeInterface.keyboardResultReceiver);
			}
		});
	}
	
	public static void hideVirtualKeyboard()
	{
		NativeInterface.activity.runOnUiThread(new Runnable()
		{
			public void run()
			{
				View view = NativeInterface.aprilActivity.getView();
				NativeInterface._getInputMethodManager().hideSoftInputFromWindow(view.getWindowToken(), 0,
					NativeInterface.keyboardResultReceiver);
			}
		});
	}
	
	public static void updateKeyboard()
	{
		// TODO - detect broken versions of com.htc.android.htcime
		if (Build.BOARD.equals("mecha") ||		// Thunderbolt
			Build.BOARD.equals("marvel") ||		// Wildfire S
			Build.BOARD.equals("marvelc"))		// Wildfire S
		{
			htcKeyboardHack = true;
		}
		else if (Build.VERSION.SDK_INT < 10 &&
			Build.BOARD.equals("shooteru") ||	// EVO 3D
			Build.BOARD.equals("supersonic"))	// EVO 4G
		{
			htcKeyboardHack = true;
		}
		else if (Build.VERSION.SDK_INT >= 10 &&
			Build.BOARD.equals("inc"))			// Droid Incredible
		{
			htcKeyboardHack = true;
		}
	}
	
	private static InputMethodManager _getInputMethodManager()
	{
		return (InputMethodManager)NativeInterface.activity.getSystemService(Context.INPUT_METHOD_SERVICE);
	}
	
	public static void showMessageBox(String title, String text, String ok, String yes, String no, String cancel, int iconId)
	{
		DialogFactory.create(title, text, ok, yes, no, cancel, iconId);
	}
	
	public static void swapBuffers()
	{
		NativeInterface.aprilActivity.glView.swapBuffers();
	}
	
	public static void reset()
	{
		NativeInterface.activity = null;
		NativeInterface.aprilActivity = null;
		NativeInterface.running = false;
		NativeInterface.archivePath = "";
		NativeInterface.dataPath = ".";
		NativeInterface.packageName = "";
		NativeInterface.versionCode = "0";
		NativeInterface.apkPath = "";
	}
	
}
