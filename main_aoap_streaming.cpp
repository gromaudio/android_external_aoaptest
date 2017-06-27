/*
* AOAP Streaming daemon
*
* This program can be used and distributed without restrictions.
*
* Authors: Vitaly Kuznetsov <v.kuznetsov.work@gmail.com>
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

#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include "services/aoap_service/AOAPService.h"

using namespace android;

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

	sp<AOAPService> gAOAPService = new AOAPService();

	if( !gAOAPService->init() ) {
		exit(EXIT_SUCCESS);
	}

	while( !gQuit ) {
		if( !gAOAPService->tick() ) {
			break;
		}
	}

	gAOAPService->unInit();

	exit(EXIT_SUCCESS);
}
