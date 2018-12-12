/*
 * IAOAPService.h
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#ifndef IAOAPSERVICE_H_
#define IAOAPSERVICE_H_

#include <binder/IInterface.h>
#include "IMediaControl.h"

#ifdef TARGET_ANDROID_8
  #define  dataWriteStrongBinder(cb)  data.writeStrongBinder(IInterface::asBinder(cb))
  #define  platAsBinder(cb) IInterface::asBinder(cb)
#else
  #define  dataWriteStrongBinder(cb)  data.writeStrongBinder((cb)->asBinder())
  #define  platAsBinder(cb) (cb)->asBinder()
#endif

namespace android {

enum {
  ENABLE = IBinder::FIRST_CALL_TRANSACTION,
  DISABLE,
  ON_EVENT,
  GET_CONTROL,
  GET_DB,
};

//------------------------------------------------------------------------------------
class IAOAPService : public IInterface
{
public:
  DECLARE_META_INTERFACE( AOAPService );

  virtual bool		   		enable(void) = 0;
  virtual bool 		   		disable(void) = 0;
  virtual int 				onEvent(int origin, int event) = 0;
  virtual sp<IMediaControl> getControl(void) = 0;
  //virtual sp<IBinder>		getDB(void) = 0;

};

//------------------------------------------------------------------------------------
class BnAOAPService : public BnInterface<IAOAPService>
{
public:
  virtual status_t onTransact( uint32_t       code,
                               const Parcel&  data,
                               Parcel*        reply,
                               uint32_t       flags = 0 );
};

}; //namespace android

#endif /* IAOAPSERVICE_H_ */
