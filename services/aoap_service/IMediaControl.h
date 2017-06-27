/*
 * IMediaControl.h
 *
 *  Created on: Jun 23, 2017
 *      Author: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
 */

#ifndef IMEDIACONTROL_H_
#define IMEDIACONTROL_H_

#include <binder/IInterface.h>

namespace android {


// Set of events that can come to player
enum MEDIA_CONTROL_EVENT {
    MEDIA_CONTROL_PLAY = 0,
    MEDIA_CONTROL_STOP,
    MEDIA_CONTROL_PAUSE,
    MEDIA_CONTROL_SEEK,
    MEDIA_CONTROL_SET_TRACK,
    MEDIA_CONTROL_SET_TRACK_PLAYING_NOW,
    MEDIA_CONTROL_NEXT_TRACK,
    MEDIA_CONTROL_PREV_TRACK,
    MEDIA_CONTROL_SET_REPEAT,
    MEDIA_CONTROL_SET_RANDOM,
    MEDIA_CONTROL_NEXT_CATEGORY_ITEM,
    MEDIA_CONTROL_PREV_CATEGORY_ITEM,
};

// Set of states the player can with in
enum MEDIA_PLAYBACK_STATE {
    MEDIA_PLAYBACK_STOP = 0,
    MEDIA_PLAYBACK_PLAY,
    MEDIA_PLAYBACK_PAUSE,
    MEDIA_PLAYBACK_FAST_FORWARD,
    MEDIA_PLAYBACK_FAST_REWIND
};

enum MEDIA_CONTROL_ORIGIN {
    MEDIA_CONTROL_ORIGIN_GUI = 0,
    MEDIA_CONTROL_ORIGIN_HW
};

// Set of error codes player can return
enum PLAYER_CONTROL_ERROR {
    PLAYER_CONTROL_ERROR_NO_ERROR = 0,
    PLAYER_CONTROL_ERROR_CATEGORY_TYPE_NOT_FOUND,
    PLAYER_CONTROL_ERROR_CATEGORY_NOT_FOUND,
    PLAYER_CONTROL_ERROR_CATEGORY_INSTANCE_NOT_FOUND,
    PLAYER_CONTROL_ERROR_NO_TRACK_IN_CATEGORY,
    PLAYER_CONTROL_ERROR_NO_SERVICE,
    PLAYER_CONTROL_ERROR_NO_STREAM_PLAYER,
    PLAYER_CONTROL_ERROR_NO_MEDIA_DB,
    PLAYER_CONTROL_ERROR_NO_CACHE_DATA,
};

// Set of states the player can with in
enum PLAYER_MODE_RANDOM {
    PLAYER_MODE_RANDOM_OFF = 0,
    PLAYER_MODE_RANDOM_ALL,
    PLAYER_MODE_RANDOM_SELECTION
};

enum PLAYER_MODE_REPEAT {
    PLAYER_MODE_REPEAT_OFF = 0,
    PLAYER_MODE_REPEAT_SINGLE,
    PLAYER_MODE_REPEAT_SELECTION
};

//-----------------------------------------------------------------
// IMediaControlCallback
//-----------------------------------------------------------------
enum {
  ON_MEDIA_STATE_CHANGED = IBinder::FIRST_CALL_TRANSACTION,
};
//------------------------------------------------------------------------------------
class IMediaControlCallback : public IInterface
{
public:
  DECLARE_META_INTERFACE( MediaControlCallback );
  virtual bool onMediaStateChanged(int origin, int event, int state) = 0;
};
//------------------------------------------------------------------------------------
class BnMediaControlCallback : public BnInterface<IMediaControlCallback>
{
public:
  virtual status_t onTransact( uint32_t       code,
                               const Parcel&  data,
                               Parcel*        reply,
                               uint32_t       flags = 0 );
};

//-----------------------------------------------------------------
// IMediaControl
//-----------------------------------------------------------------
enum {
  ADD_CALLBACK = IBinder::FIRST_CALL_TRANSACTION,
  REMOVE_CALLBACK,
  ON_EVENT1,
  ON_EVENT2,
  ON_EVENT3,
  GET_MEDIA_STATE_,
};

//------------------------------------------------------------------------------------
class IMediaControl : public IInterface
{
public:
  DECLARE_META_INTERFACE( MediaControl );

  virtual void addCallback(const sp<IMediaControlCallback> &callback) = 0;
  virtual void removeCallback(const sp<IMediaControlCallback> &callback) = 0;

  virtual int onEvent(int origin, int event) = 0;
  virtual int onEvent(int origin, int event, int arg1) = 0;
  virtual int onEvent(int origin, int event, int rootCategory, int itemPath, int arg2, int arg3) = 0;

  virtual int getMediaState(void) = 0;
};

//------------------------------------------------------------------------------------
class BnMediaControl : public BnInterface<IMediaControl>
{
public:
  virtual status_t onTransact( uint32_t       code,
                               const Parcel&  data,
                               Parcel*        reply,
                               uint32_t       flags = 0 );
};




}; //namespace android



#endif /* IMEDIACONTROL_H_ */
