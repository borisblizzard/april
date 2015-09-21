package com.april;

/// @version 3.6

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;

public class DialogFragment extends android.app.DialogFragment
{
	public DialogFragment()
	{
		super();
	}
	
	@Override
	public Dialog onCreateDialog(Bundle savedInstanceState)
	{
		return DialogFactory.show();
	}
	
	@Override
	public void onCancel(DialogInterface dialog)
	{
		dialog.cancel();
		NativeInterface.aprilActivity.glView.queueEvent(new Runnable()
		{
			public void run()
			{
				NativeInterface.onDialogCancel();
			}
		});
	}

}
