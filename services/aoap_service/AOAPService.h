/*
 * AOAPService.h
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#ifndef AOAPSERVICE_H_
#define AOAPSERVICE_H_

#include "IAOAPService.h"
#include "AOAPMediaControl.h"

namespace android
{

//------------------------------------------------------------------------------------
class AOAPService : public BnAOAPService, IAOAPMediaControlCb
{
public:
	AOAPService();
	virtual ~AOAPService();

public:
	bool init(void);
	void unInit(void);
	bool tick(void);

private: //IAOAPService
	virtual bool		   	  enable(void);
	virtual bool 		   	  disable(void);
	virtual int 			  onEvent(int origin, int event);
	virtual sp<IMediaControl> getControl(void);
	//virtual sp<IBinder>		getDB(void);

private: //IAOAPMediaControlCb
	void onCommand(int ev_command);

private:
	bool registerService(void);
	bool initBinder(void);

private:
	sp<AOAPMediaControl> mMediaControl;
};

};

#endif /* AOAPSERVICE_H_ */
