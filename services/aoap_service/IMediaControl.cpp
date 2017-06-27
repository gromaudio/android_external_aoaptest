/*
 * IMediaControl.cpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include "IMediaControl.h"

namespace android
{

//-----------------------------------------------------------------
// IMediaControlCallback
//-----------------------------------------------------------------
class BpMediaControlCallback : public BpInterface<IMediaControlCallback>
{
public:
	BpMediaControlCallback( const sp<IBinder>& impl )
			: BpInterface<IMediaControlCallback>( impl ) {}

    virtual bool onMediaStateChanged(int origin, int event, int state)
    {
      Parcel data, reply;
      data.writeInterfaceToken( IMediaControlCallback::getInterfaceDescriptor() );
      data.writeInt32(origin);
      data.writeInt32(event);
      data.writeInt32(state);
      remote()->transact( ON_MEDIA_STATE_CHANGED, data, &reply );
      return reply.readInt32();
    }
};
//-----------------------------------------------------------------
IMPLEMENT_META_INTERFACE( MediaControlCallback, "com.gromaudio.vlineservice.aoap.IMediaControlCallback" );
status_t BnMediaControlCallback::onTransact( uint32_t code, const Parcel& data,
                                    Parcel* reply, uint32_t flags)
{
  switch( code )
  {
    case ON_MEDIA_STATE_CHANGED:
    {
      CHECK_INTERFACE( IMediaControlCallback, data, reply );
      int origin = data.readInt32();
      int event = data.readInt32();
      int state = data.readInt32();
      reply->writeInt32( onMediaStateChanged(origin, event, state) );
      return NO_ERROR;
    }break;
  }
  return BBinder::onTransact( code, data, reply, flags );
};


//-----------------------------------------------------------------
// IMediaControl
//-----------------------------------------------------------------
class BpMediaControl : public BpInterface<IMediaControl>
{
public:
	BpMediaControl( const sp<IBinder>& impl )
			: BpInterface<IMediaControl>( impl ) {}

    virtual void addCallback(const sp<IMediaControlCallback> &callback)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IMediaControl::getInterfaceDescriptor() );
        data.writeStrongBinder(callback->asBinder());
        remote()->transact( ADD_CALLBACK, data, &reply );
    }

    virtual void removeCallback(const sp<IMediaControlCallback> &callback)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IMediaControl::getInterfaceDescriptor() );
        data.writeStrongBinder(callback->asBinder());
        remote()->transact( REMOVE_CALLBACK, data, &reply );
    }

    virtual int onEvent(int origin, int event)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IMediaControl::getInterfaceDescriptor() );
        data.writeInt32(origin);
        data.writeInt32(event);
        remote()->transact( ON_EVENT1, data, &reply );
        return reply.readInt32();
    }

    virtual int onEvent(int origin, int event, int arg1)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IMediaControl::getInterfaceDescriptor() );
        data.writeInt32(origin);
        data.writeInt32(event);
        data.writeInt32(arg1);
        remote()->transact( ON_EVENT2, data, &reply );
        return reply.readInt32();
    }

    virtual int onEvent(int origin, int event, int rootCategory, int itemPath, int arg2, int arg3)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IMediaControl::getInterfaceDescriptor() );
        data.writeInt32(origin);
        data.writeInt32(event);
        data.writeInt32(rootCategory);
        data.writeInt32(itemPath);
        data.writeInt32(arg2);
        data.writeInt32(arg3);
        remote()->transact( ON_EVENT3, data, &reply );
        return reply.readInt32();
    }

    virtual int getMediaState(void)
    {
        Parcel data, reply;
        data.writeInterfaceToken( IMediaControl::getInterfaceDescriptor() );
        remote()->transact( GET_MEDIA_STATE_, data, &reply );
        return reply.readInt32();
    }
};
//-----------------------------------------------------------------
IMPLEMENT_META_INTERFACE( MediaControl, "com.gromaudio.vlineservice.aoap.IMediaControl" );
status_t BnMediaControl::onTransact( uint32_t code, const Parcel& data,
                                    Parcel* reply, uint32_t flags)
{
  switch( code )
  {
    case ADD_CALLBACK:
    {
      CHECK_INTERFACE( IMediaControl, data, reply );
      sp<IMediaControlCallback> callback = interface_cast<IMediaControlCallback>
      	  	  	  	  	  	  	  	  	  	  	  	  	  (data.readStrongBinder());
      reply->writeNoException();
      addCallback(callback);
      return NO_ERROR;
    }break;

    case REMOVE_CALLBACK:
    {
      CHECK_INTERFACE( IMediaControl, data, reply );
      sp<IMediaControlCallback> callback = interface_cast<IMediaControlCallback>
      	  	  	  	  	  	  	  	  	  	  	  	  	  (data.readStrongBinder());
      reply->writeNoException();
      removeCallback(callback);
      return NO_ERROR;
    }break;

    case ON_EVENT1:
    {
      CHECK_INTERFACE( IMediaControl, data, reply );
      int origin = data.readInt32();
      int event = data.readInt32();
      reply->writeInt32( onEvent(origin, event) );
      return NO_ERROR;
    }break;

    case ON_EVENT2:
    {
      CHECK_INTERFACE( IMediaControl, data, reply );
      int origin = data.readInt32();
      int event = data.readInt32();
      int arg1 = data.readInt32();
      reply->writeInt32( onEvent(origin, event, arg1) );
      return NO_ERROR;
    }break;

    case ON_EVENT3:
    {
      CHECK_INTERFACE( IMediaControl, data, reply );
      int origin = data.readInt32();
      int event = data.readInt32();
      int rootCategory = data.readInt32();
      int itemPath = data.readInt32();
      int arg2 = data.readInt32();
      int arg3 = data.readInt32();
      reply->writeInt32( onEvent(origin, event, rootCategory, itemPath, arg2, arg3) );
      return NO_ERROR;
    }break;

    case GET_MEDIA_STATE_:
    {
      CHECK_INTERFACE( IMediaControl, data, reply );
      reply->writeInt32( getMediaState() );
      return NO_ERROR;
    }break;

  }

  return BBinder::onTransact( code, data, reply, flags );
};



// ----------------------------------------------------------------------------

}; // namespace android



