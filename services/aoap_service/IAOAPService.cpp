/*
 * IAOAPService.cpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include "IAOAPService.h"

namespace android
{

class BpAOAPService : public BpInterface<IAOAPService>
{
public:
	BpAOAPService( const sp<IBinder>& impl )
			: BpInterface<IAOAPService>( impl ) {}

    virtual bool enable(void)
    {
      Parcel data, reply;
      data.writeInterfaceToken( IAOAPService::getInterfaceDescriptor() );
      remote()->transact( ENABLE, data, &reply );
      return reply.readInt32();
    }

    virtual bool disable(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IAOAPService::getInterfaceDescriptor() );
        remote()->transact( DISABLE, data, &reply );
        return reply.readInt32();
    }

    virtual int onEvent(int origin, int event)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IAOAPService::getInterfaceDescriptor() );
        data.writeInt32(origin);
        data.writeInt32(event);
        remote()->transact( ON_EVENT, data, &reply );
        return reply.readInt32();
    }

    virtual sp<IMediaControl> getControl(void)
    {
      Parcel data, reply;
      data.writeInterfaceToken( IAOAPService::getInterfaceDescriptor() );
      remote()->transact( GET_CONTROL, data, &reply );
      return interface_cast<IMediaControl>(reply.readStrongBinder());
    }

    /*
    virtual sp<IBinder> getDB(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IAOAPService::getInterfaceDescriptor() );
        remote()->transact( GET_DB, data, &reply );
        return reply.readStrongBinder();
    }
    */
};

IMPLEMENT_META_INTERFACE( AOAPService, "com.gromaudio.vlineservice.aoap.IAOAPService" );

status_t BnAOAPService::onTransact( uint32_t code, const Parcel& data,
                                    Parcel* reply, uint32_t flags)
{
  switch( code )
  {
    case ENABLE:
    {
      CHECK_INTERFACE( IAOAPService, data, reply );
      reply->writeInt32( enable() );
      return NO_ERROR;
    }break;

    case DISABLE:
    {
      CHECK_INTERFACE( IAOAPService, data, reply );
      reply->writeInt32( disable() );
      return NO_ERROR;
    }break;

    case ON_EVENT:
    {
      CHECK_INTERFACE( IAOAPService, data, reply );
      int origin = data.readInt32();
      int event = data.readInt32();
      reply->writeInt32( onEvent(origin, event) );
      return NO_ERROR;
    }break;

    case GET_CONTROL:
    {
      CHECK_INTERFACE( IAOAPService, data, reply );
      reply->writeStrongBinder( getControl()->asBinder() );
      return NO_ERROR;
    }break;

    /*
    case GET_DB:
    {
      CHECK_INTERFACE( IAOAPService, data, reply );
      reply->writeStrongBinder( getDB() );
      return NO_ERROR;
    }break;
    */

  }
  return BBinder::onTransact( code, data, reply, flags );
};

// ----------------------------------------------------------------------------

}; // namespace android



