/*
* Android Auto Daemon.
*
* This program can be used and distributed without restrictions.
*
* Authors: Ivan Zaitsev
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "AndroidAuto.h"

//-------------------------------------------------------------------------------------------------
bool gQuit = false;

//-------------------------------------------------------------------------------------------------
static void got_signal( int )
{
  gQuit = true;
}

//-------------------------------------------------------------------------------------------------
int main(int argc, char ** argv)
{
  struct sigaction sa;

  memset( &sa, 0, sizeof( sa ) );
  sa.sa_handler = got_signal;
  sigfillset( &sa.sa_mask );
  sigaction( SIGINT, &sa, NULL );

  if( OK != AUTO_init() )
    exit(EXIT_SUCCESS);

  while( !gQuit )
    if( OK != AUTO_tick() )
      break;

  AUTO_exit();

  exit(EXIT_SUCCESS);
}
