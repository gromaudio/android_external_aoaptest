/*
 * AOAPMediaControl.h
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#ifndef AOAPMEDIACONTROL_H_
#define AOAPMEDIACONTROL_H_

#include "IMediaControl.h"

namespace android
{

//------------------------------------------------------------------------------------
enum ev_command {
	EV_STOP,
	EV_PLAYPAUSE,
	EV_NEXT,
	EV_PREV
};

//------------------------------------------------------------------------------------
class IAOAPMediaControlCb
{
public:
	virtual ~IAOAPMediaControlCb(){};
	virtual void onCommand(int ev_command) = 0;
};

//------------------------------------------------------------------------------------
class AOAPMediaControl : public BnMediaControl
{
public:
	AOAPMediaControl();
	void setCallback(IAOAPMediaControlCb *cb) {mAOAPCallback = cb;}

private: //IMediaControl
	  virtual void addCallback(const sp<IMediaControlCallback> &callback);
	  virtual void removeCallback(const sp<IMediaControlCallback> &callback);

	  virtual int onEvent(int origin, int event);
	  virtual int onEvent(int origin, int event, int arg1);
	  virtual int onEvent(int origin, int event, int rootCategory,
			  	  	  	  int itemPath, int arg2, int arg3);
	  virtual int getMediaState(void);

private:
	  void onCommand(int ev_command);

private:
	  sp<IMediaControlCallback> mCallback;
	  IAOAPMediaControlCb *mAOAPCallback;
};

};


#endif /* AOAPMEDIACONTROL_H_ */
