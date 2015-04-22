#ifndef __ANDROID_BROWSER_H__
#define __ANDROID_BROWSER_H__

#include "interface/browser_interface.h"

#define NOT_SELECTED  -1

typedef struct
{
  unsigned char iRootNodeIdx;
  signed char   iCurrentNodeType;
  int           iCurrentNodeIndex,
                iCurrentItemIndex,
                iCurrentNodeNodesCount,
                iCurrentNodeItemsCount;
}ANDROID_BROWSER_STATE;

extern const DEVICE_BROWSER_INTERFACE android_browser;

void                            ABROWSER_Init( void );
void                            ABROWSER_Activate( ANDROID_BROWSER_STATE *pState );
const DEVICE_BROWSER_INTERFACE* ABROWSER_GetInterface( void );
unsigned char                   ABROWSER_GetActiveGroupType( void );
int                             ABROWSER_GetActiveGroupIndex( void );
int                             ABROWSER_GetActiveItemIndex( void );

#endif /* __ANDROID_BROWSER_H__ */
