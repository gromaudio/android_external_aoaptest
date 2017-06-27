/*
 * AOAPService.cpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */
#define LOG_TAG "AOAPService"

#include <unistd.h>
#include <log/log.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include "AOAPService.h"

extern "C" {
#include "intercom/usbhost_hid.h"
}

namespace android
{


//------------------------------------------------------------------------------------
AOAPService::AOAPService()
{
	mMediaControl = new AOAPMediaControl();
	mMediaControl->setCallback(this);
	ALOGD("Service created.\n");
}
//------------------------------------------------------------------------------------
AOAPService::~AOAPService()
{
	mMediaControl->setCallback(0);
	mMediaControl = NULL;
	ALOGD("Service destroyed.\n");
}

//------------------------------------------------------------------------------------
bool AOAPService::init(void)
{
	bool bRes = true;
	bRes &= registerService();
	bRes &= initBinder();
	bRes &= !hid_init();
	ALOGD("Service initialization %s.\n", bRes ? "OK" : "Error");
	return bRes;
}

//------------------------------------------------------------------------------------
void AOAPService::unInit(void)
{
	ALOGD("Service exit.\n");
}

//------------------------------------------------------------------------------------
bool AOAPService::tick(void)
{
	usleep(250*1000);
	return true;
}

//------------------------------------------------------------------------------------
bool AOAPService::enable(void)
{
	ALOGD("Enable AOAP.\n");
	hid_send(HID_CMD_TOGGLEPLAY);
	/*
	if (enable_iso_driver()) {
		ALOGE("enable_iso_driver() failed;\n");
		return false;
	}
	*/
	return true;
}

//------------------------------------------------------------------------------------
bool AOAPService::disable(void)
{
	ALOGD("Disable AOAP.\n");
	hid_send(HID_CMD_STOP);
	/*
	if (disable_iso_driver()) {
		ALOGE("disable_iso_driver() failed;\n");
		return false;
	}
	*/
	return true;
}

//------------------------------------------------------------------------------------
int AOAPService::onEvent(int origin, int event)
{
	ALOGD("onEvent(origin=%d, event=%d);\n", origin, event);
	switch (event) {
	case MEDIA_CONTROL_STOP:
		onCommand(EV_STOP);
		break;
	case MEDIA_CONTROL_PLAY:
	case MEDIA_CONTROL_PAUSE:
		onCommand(EV_PLAYPAUSE);
		break;
	case MEDIA_CONTROL_NEXT_TRACK:
		onCommand(EV_NEXT);
		break;
	case MEDIA_CONTROL_PREV_TRACK:
		onCommand(EV_PREV);
		break;
	}
	return 0;
}

//------------------------------------------------------------------------------------
sp<IMediaControl> AOAPService::getControl(void)
{
	ALOGD("Get IMediaControl interface.\n");
	return mMediaControl;
}

//------------------------------------------------------------------------------------
/*
sp<IBinder>		getDB(void)
{
	return NULL;
}
*/

//------------------------------------------------------------------------------------
void AOAPService::onCommand(int ev_command)
{
	ALOGD("onCommand(%d);\n", ev_command);
	switch (ev_command) {
	case EV_STOP:
		hid_send(HID_CMD_STOP);
		break;
	case EV_PLAYPAUSE:
		hid_send(HID_CMD_TOGGLEPLAY);
		break;
	case EV_NEXT:
		hid_send(HID_CMD_NEXT);
		break;
	case EV_PREV:
		hid_send(HID_CMD_PREV);
		break;
	}
}

//------------------------------------------------------------------------------------
bool AOAPService::registerService(void)
{
	defaultServiceManager()->addService( String16( "vline.aoap" ), this );
	ALOGD("Service 'vline.aoap' is registered.\n");
	return true;
}
//-------------------------------------------------------------------------------------------------
bool AOAPService::initBinder(void)
{
	ProcessState::self()->startThreadPool();
}

}; //namespace android
