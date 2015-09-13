#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>
#include "AndroidAuto.h"
#include "AndroidAutoTouch.h"

#include <utils/StrongPointer.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <media/ICrypto.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/NativeWindowWrapper.h>
#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/Surface.h>
#include <ui/DisplayInfo.h>
#include <android/native_window.h>
#include "Utils.h"
#include <binder/IServiceManager.h>
#include "IVBaseEventService.h"

extern "C" {
#include "android_auto/hu.h"
}

using namespace android;

// ------------------------------------------------------------------------------------------------

typedef struct {
  size_t   mIndex;
  size_t   mOffset;
  size_t   mSize;
  int64_t  mPresentationTimeUs;
  uint32_t mFlags;
}BufferInfo;

unsigned char cmd_buf[256];
unsigned char res_buf[65536 * 16];

pthread_t                 gDisplayThread;
sp<SurfaceComposerClient> gComposerClient;
sp<SurfaceControl>        gControl;
sp<Surface>               gSurface;
sp<NativeWindowWrapper>   gNativeWindowWrapper;
struct ANativeWindow     *gNativeWindow;
sp<MediaCodec>            gCodec;
sp<ALooper>               gLooper;
Vector<sp<ABuffer> >      gInBuffers;
Vector<sp<ABuffer> >      gOutBuffers;
sp<IVBaseEventService>    gVBasedEventService;

// ------------------------------------------------------------------------------------------------
static status_t initOutputSurface( void )
{
  gComposerClient = new SurfaceComposerClient;
  if( OK == gComposerClient->initCheck() )
  {
    DisplayInfo info;
    sp<IBinder> display( SurfaceComposerClient::getBuiltInDisplay(
                         ISurfaceComposer::eDisplayIdMain ) );

    SurfaceComposerClient::getDisplayInfo( display, &info );
    size_t displayWidth  = info.w;
    size_t displayHeight = info.h;

    ALOGE( "display is %d x %d", displayWidth, displayHeight );

    gControl = gComposerClient->createSurface( String8( "A Surface" ),
                                               displayWidth,
                                               displayHeight,
                                               PIXEL_FORMAT_RGB_565,
                                               0 );

    if( ( gControl != NULL ) && ( gControl->isValid() ) )
    {
      SurfaceComposerClient::openGlobalTransaction();
      if( ( OK == gControl->setLayer( INT_MAX ) ) &&
          ( OK == gControl->show() ) )
      {
        SurfaceComposerClient::closeGlobalTransaction();
        gSurface = gControl->getSurface();
      }
    }
  }

  if( gSurface == NULL )
  {
    ALOGE( "Screen surface create error" );
    return UNKNOWN_ERROR;
  }
  gNativeWindowWrapper = new NativeWindowWrapper( gSurface );
  gNativeWindow = gSurface.get();

  native_window_set_buffers_format( gNativeWindow, HAL_PIXEL_FORMAT_YV12 );
  native_window_set_buffers_dimensions( gNativeWindow, 800, 480 );
  native_window_set_scaling_mode( gNativeWindow, NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW );

  ALOGD( "Screen surface created" );
  ALOGD( "Screen surface %s", ( gSurface->isValid( gSurface ) ? "valid" : "invalid" ) );

  return OK;
}

//-------------------------------------------------------------------------------------------------
uint8_t AvccExtraData[] = {
  0x01, 0x42, 0x80, 0x1F, 0xFF, 0xE1, 0x00, 0x0D, 0x67, 0x42, 0x80, 0x1F, 0xDA, 0x03, 0x20, 0xF6,
  0x80, 0x6D, 0x0A, 0x13, 0x50, 0x01, 0x00, 0x04, 0x68, 0xCE, 0x06, 0xE2
};

//-------------------------------------------------------------------------------------------------
static status_t initHwCodec( uint8_t *configData, size_t configDataSize )
{
  sp<AMessage> format;
  sp<MetaData> meta = new MetaData;
  status_t     res;

  gLooper = new ALooper;
  gLooper->start();

  gCodec = MediaCodec::CreateByType( gLooper, "video/avc", false );
  if( gCodec != NULL )
  {
    ALOGD( "Codec created" );
  }

  meta->clear();
  meta->setCString( kKeyMIMEType,         "video/avc" );
  meta->setInt32(   kKeyTrackID,          1 );
  meta->setInt32(   kKeyWidth,            800 );
  meta->setInt32(   kKeyHeight,           480 );
  meta->setInt32(   kKeyDisplayWidth,     800 );
  meta->setInt32(   kKeyDisplayHeight,    480 );
  meta->setData(    kKeyAVCC, kTypeAVCC,  configData, configDataSize );
  meta->dumpToLog();
  convertMetaDataToMessage( meta, &format );

  res = gCodec->configure( format, gNativeWindowWrapper->getSurfaceTextureClient(), NULL, 0 );
  if( res != OK )
  {
    ALOGE( "Codec configure error: %d", res );
    return res;
  }

  res = gCodec->start();
  if( res != OK )
  {
    ALOGE( "Codec start error: %d", res );
    return res;
  }

  res = gCodec->getInputBuffers( &gInBuffers );
  if( res != OK )
  {
    ALOGE( "Codec get input buffers error: %d", res );
    return res;
  }

  res = gCodec->getOutputBuffers( &gOutBuffers );
  if( res != OK )
  {
    ALOGE( "Codec get output buffers error: %d", res );
    return res;
  }

  sp<ABuffer> srcBuffer;
  size_t j = 0;
  while( format->findBuffer( StringPrintf( "csd-%d", j ).c_str(), &srcBuffer ) )
  {
    size_t index;
    res = gCodec->dequeueInputBuffer( &index, -1ll );
    if( res != OK )
    {
      ALOGE( "Dequeue error" );
    }

    const sp<ABuffer> &dstBuffer = gInBuffers.itemAt( index );

    dstBuffer->setRange( 0, srcBuffer->size() );
    memcpy( dstBuffer->data(), srcBuffer->data(), srcBuffer->size() );

    ALOGD( "CSD data size: %d", srcBuffer->size() );
    for( size_t i = 0; i < srcBuffer->size(); i++ )
    {
      ALOGD(  "0x%02X ", srcBuffer->data()[ i ] );
      if( !( ( i + 1 ) % 8) )
        ALOGD( "\n" );
    }
    ALOGE( "\n" );

    res = gCodec->queueInputBuffer( index,
                                    0,
                                    dstBuffer->size(),
                                    0ll,
                                    MediaCodec::BUFFER_FLAG_CODECCONFIG );
    if( res != OK )
    {
      ALOGE( "Queue error: %d", res );
      return res;
    }

    j++;
  }

  ALOGD( "Codec init OK" );
  return OK;
}

//-------------------------------------------------------------------------------------------------
static void decodeHwFrame( uint8_t *inBuff, size_t inBuffSize )
{
  status_t res;
  size_t   index;

  for(;;)
  {
    res = gCodec->dequeueInputBuffer( &index, 1ll );
    if( res == OK )
    {
      const sp<ABuffer> &buffer = gInBuffers.itemAt( index );

      buffer->setRange( 0, inBuffSize );
      memcpy( (uint8_t*)buffer->data(), inBuff, inBuffSize );

      gCodec->queueInputBuffer( index,
                                buffer->offset(),
                                buffer->size(),
                                0ll,
                                0 );
      break;
    }
  }
}

//-------------------------------------------------------------------------------------------------
static void* displayThread( void* params )
{
  status_t        res;
  BufferInfo      info;

  for(;;)
  {
    res = gCodec->dequeueOutputBuffer( &info.mIndex,
                                       &info.mOffset,
                                       &info.mSize,
                                       &info.mPresentationTimeUs,
                                       &info.mFlags );
    if( res == OK )
    {
      gCodec->renderOutputBufferAndRelease( info.mIndex );
    }
  }
}

//-------------------------------------------------------------------------------------------------
static size_t saveFrame( uint8_t *frameData, size_t frameDataSize )
{
  static int  file_counter  = 0;
  size_t      bytes_written = 0;
  char        file_name[ 100 ];
  FILE        *file;

  sprintf( file_name, "/boot/auto/AnnexB/auto_%03d.raw", file_counter );
  file = fopen( file_name, "wb" );
  if(file != NULL)
  {
    bytes_written = fwrite( frameData, sizeof(uint8_t), frameDataSize, file );
    fclose( file );
    file_counter++;
  }
  else
  {
    fprintf( stderr, "Error opening file %s: %d, %s\n", file_name, errno, strerror( errno ) );
  }
  return bytes_written;
}

//-------------------------------------------------------------------------------------------------
status_t AUTO_init( void )
{
  status_t                res;
  sp<IBinder>             binder = NULL;

  binder = defaultServiceManager()->getService( String16( "vbased.events" ) );
  gVBasedEventService = interface_cast<IVBaseEventService>( binder );
  if( gVBasedEventService == NULL )
  {
    ALOGE( "Cannot get VBaseEventService interface" );
    return UNKNOWN_ERROR;
  }

  cmd_buf[ 0 ]   = 121;
  cmd_buf[ 1 ]   = 0x81;
  cmd_buf[ 2 ]   = 0x02;
  jni_aa_cmd( 3, (char*)cmd_buf, 0, NULL );

  TOUCH_init( gVBasedEventService->getNativeDevPath() );
  ProcessState::self()->startThreadPool();
  DataSource::RegisterDefaultSniffers();
  res = initOutputSurface();
  if( res != OK )
    return res;

  res = initHwCodec( AvccExtraData, sizeof( AvccExtraData ) );
  if( res != OK )
    return res;

  if( pthread_create( &gDisplayThread,
                      NULL,
                      displayThread,
                      NULL ) )
  {
    ALOGE( "Cannot create display thread" );
    return UNKNOWN_ERROR;
  }
  return OK;
}

//-------------------------------------------------------------------------------------------------
status_t AUTO_tick( void )
{
  int             bytes_received,
                  bytes_written;
  size_t          cmd_len;

  cmd_len = TOUCH_getAction( cmd_buf, sizeof( cmd_buf ) );
  bytes_received = jni_aa_cmd( cmd_len, (char*)cmd_buf, sizeof( res_buf ), (char*)res_buf );

  if( bytes_received < 0 )
  {
    ALOGE( "Android auto error: %s", strerror( bytes_received ) );
    return UNKNOWN_ERROR;
  }

  if( bytes_received > 0 )
    decodeHwFrame( res_buf, bytes_received );

  gVBasedEventService->stopAndroidEvents();

  return OK;
}