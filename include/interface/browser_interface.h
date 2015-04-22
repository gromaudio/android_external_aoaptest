#ifndef BROWSER_INTERFACE_H
#define BROWSER_INTERFACE_H

// ----------------------------
#define BROWSER_NODE_LEVEL_UP    -1
#define BROWSER_NODE_ROOT        -2
// ----------------------------
typedef enum
{
  NODE_OPEN_FOLDER = 0,
  NODE_PLAYLISTS,
  NODE_ARTISTS,
  NODE_ALBUMS,
  NODE_GENRES,
  NODE_PODCASTS,
  NODE_FOLDER,
  NODE_TRACK,
  NODE_ALL_FILES,
} BROWSER_ITEM_TYPE;

// ----------------------------
typedef struct
{
  char          *name;
  unsigned short nameSize;

  BROWSER_ITEM_TYPE type;
} DEVICE_BROWSER_ITEM;

// ----------------------------
typedef enum
{
  DEVICE_BROWSER_RESULT_OK =    0,
  DEVICE_BROWSER_RESULT_FAILED= 20,
  DEVICE_BROWSER_RESULT_NOINIT,
  DEVICE_BROWSER_PARAM_INVALID,
  DEVICE_BROWSER_ABORTED,
} DEVICE_BROWSER_RESULT;

// ----------------------------
typedef struct
{
  int            (*getCurrentLevel)( void );
  DEVICE_BROWSER_RESULT (*getCount)( int                 *nodesCount,
                                     int                 *itemsCount );
  DEVICE_BROWSER_RESULT (*getItem)(  int                  index,
                                     DEVICE_BROWSER_ITEM *item );
  DEVICE_BROWSER_RESULT (*setItem)(  int                  index );
  DEVICE_BROWSER_RESULT (*getNode)(  int                  index,
                                     DEVICE_BROWSER_ITEM *node );
  DEVICE_BROWSER_RESULT (*setNode)(  int                  index );

  DEVICE_BROWSER_RESULT (*getUrl) ( int _index, char *_buff, int _buff_size);
} DEVICE_BROWSER_INTERFACE;

#endif
