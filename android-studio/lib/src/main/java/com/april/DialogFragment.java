package com.april;

/// @version 5.2

import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Bundle;

public class DialogFragment extends androidx.fragment.app.DialogFragment
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
		NativeInterface.aprilActivity.glView.queueEvent(() -> NativeInterface.onDialogCancel());
	}

}
