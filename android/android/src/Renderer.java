package com.april;

/// @version 3.7

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewTreeObserver;

public class Renderer implements android.opengl.GLSurfaceView.Renderer
{
	public void onSurfaceCreated(GL10 gl, EGLConfig config)
	{
		Thread.currentThread().setPriority(Thread.MIN_PRIORITY);
		NativeInterface.onSurfaceCreated();
		if (!NativeInterface.running)
		{
			NativeInterface.setVariables(NativeInterface.dataPath, NativeInterface.archivePath);
			String args[] = {NativeInterface.apkPath}; // adding argv[0]
			NativeInterface.init(args);
			NativeInterface.running = true;
			// needed for keyboard height
			NativeInterface.activity.getWindow().getDecorView().getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener()
			{
				@Override
				public void onGlobalLayout()
				{
					View view = NativeInterface.aprilActivity.getView();
					Rect r = new Rect();
					view.getWindowVisibleDisplayFrame(r);
					float heightRatio = 1.0f - (float)(r.bottom - r.top) / view.getRootView().getHeight();
					NativeInterface.onVirtualKeyboardChanged((heightRatio > 0.15f), heightRatio);
				}
			});
		}
	}
	
	public void onSurfaceChanged(GL10 gl, int w, int h)
	{
	}
	
	public void onDrawFrame(GL10 gl)
	{
		if (!NativeInterface.render())
		{
			NativeInterface.activity.finish();
		}
	}
	
	public void swapBuffers()
	{
		EGL10 egl = (EGL10)EGLContext.getEGL();
		EGLDisplay display = egl.eglGetCurrentDisplay();
		EGLSurface surface = egl.eglGetCurrentSurface(EGL10.EGL_READ);
		egl.eglSwapBuffers(display, surface);
	}
	
}
