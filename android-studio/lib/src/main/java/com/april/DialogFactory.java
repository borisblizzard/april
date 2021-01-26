package com.april;

/// @version 5.2

import android.app.Dialog;
import android.app.AlertDialog;

import com.april.DialogListener.Cancel;
import com.april.DialogListener.Ok;
import com.april.DialogListener.No;
import com.april.DialogListener.Yes;

public class DialogFactory
{
	protected static AlertDialog.Builder dialogBuilder = null;

	public static void create(String title, String text, String ok, String yes, String no, String cancel, int iconId)
	{
		DialogFactory.dialogBuilder = new AlertDialog.Builder(NativeInterface.activity);
		DialogFactory.dialogBuilder.setTitle(title != null ? title : "");
		DialogFactory.dialogBuilder.setMessage(text != null ? text : "");
		if (ok != null)
		{
			DialogFactory.dialogBuilder.setPositiveButton(ok, new Ok());
		}
		else
		{
			if (yes != null)
			{
				DialogFactory.dialogBuilder.setPositiveButton(yes, new Yes());
			}
			if (no != null)
			{
				DialogFactory.dialogBuilder.setNegativeButton(no, new No());
			}
		}
		if (cancel != null)
		{
			DialogFactory.dialogBuilder.setNeutralButton(cancel, new Cancel());
		}
		switch (iconId)
		{
		case 1:
			DialogFactory.dialogBuilder.setIcon(android.R.drawable.ic_dialog_info);
			break;
		case 2:
			DialogFactory.dialogBuilder.setIcon(android.R.drawable.ic_dialog_alert);
			break;
		default:
			break;
		}
		try
		{
			DialogFragment dialogFragment = new DialogFragment();
			dialogFragment.show(NativeInterface.activity.getSupportFragmentManager(), "april-dialog");
		}
		catch (java.lang.Throwable e)
		{
			android.util.Log.w("april", "This Android OS version does not support DialogFragment, defaulting to legacy Activity.showDialog().");
		}
	}
	
	public static Dialog show()
	{
		Dialog dialog = DialogFactory.dialogBuilder.create();
		dialog.setCancelable(true);
		dialog.setCanceledOnTouchOutside(true);
		return dialog;
	}
	
}
