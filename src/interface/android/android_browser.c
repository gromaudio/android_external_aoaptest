/*****************************************************************************
 *   android_browser.c:  Android browser iterface.
 *                       Implements Node interface.
 *
 *   Copyright(C) 2013, X-mediatech
 *   All rights reserved.
 *
 *
 *    No. | Date        |   Author       | Description
 *   ========================================================================
 *      1 | 29 Jan 2014 |  Ivan Zaitsev  | First release.
 *
******************************************************************************/

#include <string.h>
#include "common.h"
#include "player_common.h"
#include "interface/debug/debug.h"
#include "interface/android/android_browser.h"
#include "interface/android/android_accessory.h"
#include "interface/android/android_commands.h"

// --------------------------------------------------------------------------------
//#define ABROWSER_TRACE(...) DEBUGOUT_F( "ABROWSER: " __VA_ARGS__ )
#define ABROWSER_TRACE(...)

// --------------------------------------------------------------------------------
typedef struct
{
  unsigned char      iGroupId;
  BROWSER_ITEM_TYPE  BrowserType;
  char              *pName;
}ANDROID_ROOT_ITEM;

// --------------------------------------------------------------------------------
const ANDROID_ROOT_ITEM aRootItems[] = {
  { GROUP_PLAYLISTS, NODE_PLAYLISTS, "Playlists" },
  { GROUP_ARTISTS,   NODE_ARTISTS,   "Artists" },
  { GROUP_ALBUMS,    NODE_ALBUMS,    "Albums" },
  { GROUP_GENRES,    NODE_GENRES,    "Genres" },
  { GROUP_ALL_FILES, NODE_ALL_FILES, "All songs" }
};
ANDROID_BROWSER_STATE *pAndroidBrowserState;

// --------------------------------------------------------------------------------
// Return pointer to browser interface
const DEVICE_BROWSER_INTERFACE* ABROWSER_GetInterface( void )
{
  return &android_browser;
}

// --------------------------------------------------------------------------------
// Init browser interface
void ABROWSER_Init( void )
{

}

// --------------------------------------------------------------------------------
// Activate browser interface
void ABROWSER_Activate( ANDROID_BROWSER_STATE *pState )
{
  pAndroidBrowserState = pState;
  pAndroidBrowserState->iCurrentNodeIndex      = NOT_SELECTED;
  pAndroidBrowserState->iCurrentNodeType       = NOT_SELECTED;
  pAndroidBrowserState->iCurrentNodeNodesCount = NELEMS( aRootItems );
  pAndroidBrowserState->iCurrentNodeItemsCount = 0;
}

// --------------------------------------------------------------------------------
// Return active group id
unsigned char ABROWSER_GetActiveGroupType( void )
{
  return aRootItems[ pAndroidBrowserState->iRootNodeIdx ].iGroupId;
}

// --------------------------------------------------------------------------------
// Return active group index
int ABROWSER_GetActiveGroupIndex( void )
{
  return pAndroidBrowserState->iCurrentNodeIndex;
}

// --------------------------------------------------------------------------------
// Return active item index
int ABROWSER_GetActiveItemIndex( void )
{
  return pAndroidBrowserState->iCurrentItemIndex;
}

// --------------------------------------------------------------------------------
// Browser interface details
static int browserGetCurrentLevel( void )
{
  // We do not have a way to tell where we are
  return ( pAndroidBrowserState->iCurrentNodeType  == NOT_SELECTED ) ? 0:
         ( pAndroidBrowserState->iCurrentNodeIndex == NOT_SELECTED ) ? 1: 2;
}


// --------------------------------------------------------------------------------
// Get counts for the current node
static DEVICE_BROWSER_RESULT browserGetCount( int *nodesCount,
                                              int *itemsCount )
{
  DEVICE_BROWSER_RESULT iRes = DEVICE_BROWSER_RESULT_OK;
  *nodesCount = pAndroidBrowserState->iCurrentNodeNodesCount;
  *itemsCount = pAndroidBrowserState->iCurrentNodeItemsCount;
  return iRes;
}

// --------------------------------------------------------------------------------
// Get item in the active node
static DEVICE_BROWSER_RESULT browserGetItem( int                  iIndex,
                                             DEVICE_BROWSER_ITEM *cItem )
{
  cItem->type      = NODE_TRACK;
  cItem->name[ 0 ] = 0;

  // Check if we have any items in this level
  if( iIndex < 0 ||
      iIndex >= pAndroidBrowserState->iCurrentNodeItemsCount )
    return DEVICE_BROWSER_PARAM_INVALID;

  if( cItem->nameSize > 0 )
  {
    ACOMMAND_GetTrackName( iIndex, cItem->name, cItem->nameSize );
    ABROWSER_TRACE( "GetItem %d: %s", iIndex, cItem->name );
    if( cItem->name[ 0 ] == 0 )
      return DEVICE_BROWSER_RESULT_FAILED;
    else
      return DEVICE_BROWSER_RESULT_OK;
  }
  return DEVICE_BROWSER_RESULT_OK;
}

// --------------------------------------------------------------------------------
// Get item in the active node
static DEVICE_BROWSER_RESULT browserSetItem( int iIndex )
{
  ABROWSER_TRACE( "SetItem %d", iIndex );

  if( iIndex < 0 ||
      iIndex >= pAndroidBrowserState->iCurrentNodeItemsCount )
    return DEVICE_BROWSER_PARAM_INVALID;

  pAndroidBrowserState->iCurrentItemIndex = iIndex;
  ACOMMAND_SelectCurrentTrack( iIndex );

  return DEVICE_BROWSER_RESULT_OK;
}

// --------------------------------------------------------------------------------
// Get item in the active node
static DEVICE_BROWSER_RESULT browserGetNode( int                  iIndex,
                                             DEVICE_BROWSER_ITEM *cItem )
{
  DEVICE_BROWSER_RESULT iRes = DEVICE_BROWSER_RESULT_OK;

  if( iIndex < 0 ||
      iIndex >= pAndroidBrowserState->iCurrentNodeNodesCount )
  {
    ABROWSER_TRACE( "GetNode invalid param: %d %d", iIndex, pAndroidBrowserState->iCurrentNodeNodesCount );
    return DEVICE_BROWSER_PARAM_INVALID;
  }

  if( pAndroidBrowserState->iCurrentNodeType == NOT_SELECTED )
  {
    // We are in the root
    ABROWSER_TRACE( "GetNode Lev_0(%d)", iIndex );
    cItem->type = aRootItems[ iIndex ].BrowserType;
    cItem->name = strncpy( cItem->name, aRootItems[ iIndex ].pName, cItem->nameSize );
    cItem->name[ cItem->nameSize - 1 ] = 0;
    ABROWSER_TRACE( "node(%d): %s", cItem->nameSize, cItem->name );
  }
  else
  if( pAndroidBrowserState->iCurrentNodeIndex == NOT_SELECTED )
  {
    // We're on level 1.
    ABROWSER_TRACE( "GetNode Lev_1(%d) 0x%08X %d", iIndex, cItem->name, cItem->nameSize );
    cItem->type = NODE_FOLDER;

    if( cItem->nameSize > 0 )
    {
      ACOMMAND_GetGroupItemName( pAndroidBrowserState->iCurrentNodeType,
                                 iIndex,
                                 cItem->name,
                                 cItem->nameSize );
      ABROWSER_TRACE( "node(%d): %s", cItem->nameSize, cItem->name );
    }
  }
  else
  {
    // There should be no nodes below level 2
    ABROWSER_TRACE( "GetNode below lev_2 error %d", pAndroidBrowserState->iCurrentNodeIndex );
    iRes= DEVICE_BROWSER_PARAM_INVALID;
  }
  return iRes;
}

// --------------------------------------------------------------------------------
// Set active node
static DEVICE_BROWSER_RESULT browserSetNode( int iIndex )
{
  // If iIndex contains command execute it first
  if( iIndex== BROWSER_NODE_ROOT )
  {
    pAndroidBrowserState->iCurrentNodeIndex      = NOT_SELECTED;
    pAndroidBrowserState->iCurrentNodeType       = NOT_SELECTED;
    pAndroidBrowserState->iCurrentNodeNodesCount = NELEMS( aRootItems );
    pAndroidBrowserState->iCurrentNodeItemsCount = 0;
    return DEVICE_BROWSER_RESULT_OK;
  }
  else
  if( iIndex== BROWSER_NODE_LEVEL_UP )
  {
    if( ( pAndroidBrowserState->iCurrentNodeType != NOT_SELECTED ) &&
        ( pAndroidBrowserState->iCurrentNodeIndex != NOT_SELECTED ) )
    {
      // We're on level 2. Go to level 1
      iIndex = pAndroidBrowserState->iRootNodeIdx;
      pAndroidBrowserState->iCurrentNodeIndex      = NOT_SELECTED;
      pAndroidBrowserState->iCurrentNodeType       = NOT_SELECTED;
      pAndroidBrowserState->iCurrentNodeNodesCount = NELEMS( aRootItems );
      pAndroidBrowserState->iCurrentNodeItemsCount = 0;
      ABROWSER_TRACE( "SetNode Lev 2: LEV_UP %d", iIndex );
    }
    else
    if( ( pAndroidBrowserState->iCurrentNodeType != NOT_SELECTED ) &&
        ( pAndroidBrowserState->iCurrentNodeIndex == NOT_SELECTED ) )
    {
      ABROWSER_TRACE( "SetNode Lev 1: LEV_UP" );
      // We're on level 1. Go to level 0 ( root )
      pAndroidBrowserState->iCurrentNodeIndex      = NOT_SELECTED;
      pAndroidBrowserState->iCurrentNodeType       = NOT_SELECTED;
      pAndroidBrowserState->iCurrentNodeNodesCount = NELEMS( aRootItems );
      pAndroidBrowserState->iCurrentNodeItemsCount = 0;
      return DEVICE_BROWSER_RESULT_OK;
    }
    else
    {
      // We're on level 0. No way to go up.
      return DEVICE_BROWSER_RESULT_FAILED;
    }
  }

  // Now iIndex has node index we have to select.
  // Check it for overflow first and select specified node.
    if( iIndex < 0 ||
        iIndex >= pAndroidBrowserState->iCurrentNodeNodesCount )
      return DEVICE_BROWSER_PARAM_INVALID;

  if( pAndroidBrowserState->iCurrentNodeType == NOT_SELECTED )
  {
    // We're in the level 0 ( root ). Select GROUP.
    pAndroidBrowserState->iRootNodeIdx           = iIndex;
    pAndroidBrowserState->iCurrentNodeIndex      = NOT_SELECTED;
    pAndroidBrowserState->iCurrentNodeType       = aRootItems[ iIndex ].iGroupId;
    if( aRootItems[ iIndex ].iGroupId != GROUP_ALL_FILES )
    {
      pAndroidBrowserState->iCurrentNodeNodesCount = ACOMMAND_SelectGroup( aRootItems[ iIndex ].iGroupId );
      pAndroidBrowserState->iCurrentNodeItemsCount = 0;
    }
    else
    {
      pAndroidBrowserState->iCurrentNodeNodesCount = 0;
      pAndroidBrowserState->iCurrentNodeItemsCount = ACOMMAND_SelectGroup( aRootItems[ iIndex ].iGroupId );
    }

    ABROWSER_TRACE( "SetNode Lev 0, Grp %d(%d): %d", iIndex, pAndroidBrowserState->iCurrentNodeType, pAndroidBrowserState->iCurrentNodeNodesCount );

  }
  else
  if( pAndroidBrowserState->iCurrentNodeIndex == NOT_SELECTED )
  {
    // We're in the level 1. Select GROUP Item.
    pAndroidBrowserState->iCurrentNodeIndex      = iIndex;
    pAndroidBrowserState->iCurrentNodeNodesCount = 0;
    pAndroidBrowserState->iCurrentNodeItemsCount = ACOMMAND_SelectGroupItem( pAndroidBrowserState->iCurrentNodeType,
                                                                             iIndex );
    ABROWSER_TRACE( "SetNode Lev 1, Grp %d: %d", pAndroidBrowserState->iCurrentNodeType, pAndroidBrowserState->iCurrentNodeItemsCount );
  }
  else
  {
    // We're on level 2. No nodes to select on this level.
    return DEVICE_BROWSER_RESULT_FAILED;
  }

  return DEVICE_BROWSER_RESULT_OK;
}

// --------------------------------------------------------------------------------
// Get URL for item
static DEVICE_BROWSER_RESULT browserGetURL( int iIndex, char* pBuff, int iBuffSize )
{
  strncpy( pBuff, "Not supported", iBuffSize );
  pBuff[ iBuffSize - 1 ] = 0;
  return DEVICE_BROWSER_RESULT_OK;
}


// --------------------------------------------------------------------------------
const DEVICE_BROWSER_INTERFACE android_browser=
{
  browserGetCurrentLevel,
  browserGetCount,
  browserGetItem,
  browserSetItem,
  browserGetNode,
  browserSetNode,
  browserGetURL
};



