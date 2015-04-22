#ifndef __ACCESSORY_H__
#define __ACCESSORY_H__

#define AALINQ_PROTOCOL_VER            3

//----------------------- MESSAGE TYPES -------------------------------
#define MESSAGE_TYPE_CAPABILITIES      0x10
#define MESSAGE_TYPE_CONTROL           0xC0
#define MESSAGE_TYPE_MEDIA             0x80
#define MESSAGE_TYPE_BULK              0x40

//----------------------- CONTROL COMMANDS ----------------------------
#define CONTROL_ACK                    0x00
#define CONTROL_PLAY                   0x01
#define CONTROL_PAUSE                  0x03
#define CONTROL_STOP                   0x05
#define CONTROL_NEXT_TRK               0x07
#define CONTROL_PREV_TRK               0x09
#define CONTROL_FFW                    0x0D
#define CONTROL_FRW                    0x0F
#define CONTROL_SET_RND                0x11
#define CONTROL_GET_RND                0x13
#define CONTROL_RND                    0x14
#define CONTROL_SET_RPT                0x15
#define CONTROL_GET_RPT                0x17
#define CONTROL_RPT                    0x18
#define CONTROL_TRK_CHANGED            0x30
#define CONTROL_TRK_STATE_CHANGED      0x32

//----------------------- MEDIA COMMANDS -------------------------------
#define MEDIA_GET_GROUP_ITEMS_COUNT    0x01    // <GROUP_NUM> Request for list of groups under category
#define MEDIA_GROUP_ITEMS_COUNT        0x02    // <GROUP_NUM> Response list of categories
#define MEDIA_GET_GROUP_ITEMS          0x03    // <GROUP_ID> <FROM> <TO> Request list of group values
#define MEDIA_GROUP_ITEMS              0x04    // <ZT_STRING>* Respond with list of values
#define MEDIA_SET_ACTIVE_GROUP_ITEM    0x05    // <GROUP_ID> <ITEM_NUM> 
#define MEDIA_GET_TRK_COUNT            0x07    // Ask total number of tracks in the current item
#define MEDIA_TRK_COUNT                0x08    // <INT> Total number of tracks in the groupped value
#define MEDIA_GET_TRK_INFO             0x09    // <TRK_NUM> Ask track information by track number (-1 if currently playing)
#define MEDIA_TRK_INFO                 0x0A    // <TRK_INFO> Response with track information
#define MEDIA_GET_TRK_ID3_INFO         0x0B    // <TRK_NUM> Ask track information by track number (-1 if currently playing)
#define MEDIA_TRK_ID3_INFO             0x0C    // <TRK_ID3> Response with track information
#define MEDIA_SET_CUR_TRK              0x0D    // <TRK_NUM> Set current track number
#define MEDIA_GET_CUR_TRK              0x0F    // <TRK_NUM> Ask current track number
#define MEDIA_CUR_TRK                  0x10    // <TRK_NUM> Respond with current track number
#define MEDIA_GET_STATUS               0x11    // Request media status information ( group value, position in group etc )
#define MEDIA_SET_STATUS               0x21    // Update media status information ( group value, position in group etc )
#define MEDIA_STATUS                   0x12    // <TIME_ELAPSED_MS> Response with track status information 
#define MEDIA_SET_TRK_TIME             0x13    // <TIME_ELAPSED_MS> Set status for the AL player to updat
#define MEDIA_GET_TRACK_NAME           0x15
#define MEDIA_TRACK_NAME               0x16

#define GROUP_ALL_FILES                0x00
#define GROUP_PLAYLISTS                0x01
#define GROUP_ARTISTS                  0x02
#define GROUP_ALBUMS                   0x03
#define GROUP_GENRES                   0x04
#define GROUP_FOLDERS                  0x05
#define GROUP_FAVORITS                 0x06

//----------------------- PLAYBACK STATUS-------------------------------
#define STATUS_STOP                    0x00
#define STATUS_PLAY                    0x01
#define STATUS_PAUSED                  0x02
#define STATUS_FF                      0x04
#define STATUS_FR                      0x08
#define STATUS_POS_CHANGED             0x10
#define STATUS_FILE_END                0xFF

//----------------------- PLAYBACK FLAGS -------------------------------
#define PL_FLAG_MIX                    ( 1 << 0 )
#define PL_FLAG_RPT                    ( 1 << 1 )
#define PL_FLAG_SCAN                   ( 1 << 2 )

//----------------------- BULK COMMANDS --------------------------------
#define BULK_READ_STREAM_DATA          0x01
#define BULK_STREAM_DATA               0x02
#define BULK_PICTURE_DATA              0x04

//----------------------- CAPABILITIES COMMANDS ------------------------
#define CAP_ACK                        0x00
#define CAP_CODECS_SUPPORTED           0x01
#define CAP_SCREEN_RESOLUTION          0x03
#define CAP_DOCK_MODE                  0x05
#define CAP_VERSION                    0x07
#define CAP_GET_DEVICE_NAME            0x09
#define CAP_DEVICE_NAME                0x0A
#define CAP_SET_DIGITAL_AUDIO_MODE     0x0B
#define CAP_GET_GROM_OPTIONS           0x0D
#define CAP_GROM_OPTIONS               0x0E

//----------------------- DIGITAL AUDIO MODES --------------------------
#define DIGITAL_AUDIO_AOA_V1           0x00
#define DIGITAL_AUDIO_AOA_V2           0x01

//----------------------- TRK CHANGE FLAGS -----------------------------
#define TRK_CHG_NO_FLAGS               0x00
#define TRK_CHG_NEW_OTG                ( 1 << 0 )

//----------------------- RESULT CODES -----------------------------
#define RESULT_OK                      0x00
#define RESULT_WRONG_PARAMETER         0x01
#define RESULT_UNKNOWN_COMMAND         0x02

//----------------------------------------------------------------------

#endif /* __ACCESSORY_H__ */
