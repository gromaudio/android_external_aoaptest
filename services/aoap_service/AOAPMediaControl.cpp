/*
 * AOAPMediaControl.cpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#define LOG_TAG "AOAPMediaControl"

#include <unistd.h>
#include <log/log.h>
#include "AOAPMediaControl.h"

using namespace android;

//------------------------------------------------------------------------------------
AOAPMediaControl::AOAPMediaControl() : mCallback(0), mAOAPCallback(0)
{
}

//------------------------------------------------------------------------------------
void AOAPMediaControl::addCallback(const sp<IMediaControlCallback> &callback)
{
	ALOGD("addCallback();\n");
	mCallback = callback;
}
//------------------------------------------------------------------------------------
void AOAPMediaControl::removeCallback(const sp<IMediaControlCallback> &callback)
{
	ALOGD("removeCallback();\n");
	mCallback = NULL;
}
//------------------------------------------------------------------------------------
int AOAPMediaControl::onEvent(int origin, int event)
{
	ALOGD("onEvent(origin=%d, event=%d);\n", origin, event);
	switch (event) {
	case MEDIA_PLAYBACK_STOP:
		onCommand(EV_STOP);
		break;
	case MEDIA_PLAYBACK_PLAY:
	case MEDIA_PLAYBACK_PAUSE:
		onCommand(EV_PLAYPAUSE);
		break;
	}
	return 0;
}
//------------------------------------------------------------------------------------
int AOAPMediaControl::onEvent(int origin, int event, int arg1)
{
	ALOGD("onEvent(origin=%d, event=%d, arg1=%d);\n", origin, event, arg1);
	return 0;
}
//------------------------------------------------------------------------------------
int AOAPMediaControl::onEvent(int origin, int event, int rootCategory,
		  	  	  	  int itemPath, int arg2, int arg3)
{
	ALOGD("onEvent(origin=%d, event=%d, rootCategory=%d, itemPath=%d, arg2=%d, arg3=%d);\n",
			origin, event, rootCategory, itemPath, arg2, arg3);
	return 0;
}
//------------------------------------------------------------------------------------
int AOAPMediaControl::getMediaState(void)
{
	ALOGD("getMediaState();\n");
	return 0;
}

//------------------------------------------------------------------------------------
void AOAPMediaControl::onCommand(int ev_command)
{
	if (mAOAPCallback) {
		mAOAPCallback->onCommand(ev_command);
	}
}
