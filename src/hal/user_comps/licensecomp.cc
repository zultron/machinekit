//
// This is a userspace HAL driver for Tormach licensing control
// which can run prior to hostmot2 drivers being initialized.
//
// Copyright 2020 Tormach, LLC
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//


#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string.h>

#include "hal.h"
#include "licensemgr.hh"

// misdirection for user comp name
char modname[] = {"hwmon\0"};

int hal_comp_id;


static void exit_handler(int sig)
{
    fprintf(stderr, "%s: exiting\n", modname);

    // "halcmd list comp" over and over until hostmot2 not listed

    // might be able to reliably put the nv flash write stuff here
    fprintf(stderr, "%s: simulating nv flash writing taking awhile\n", modname);
    sleep(10);
    fprintf(stderr, "%s: nv flash write done - now exiting\n", modname);

    exit(0);
}


static void call_hal_exit(void)
{
    hal_exit(hal_comp_id);
}


int main(int argc, char *argv[])
{
	int retcode = 1;
    fprintf(stderr, "%s: starting\n", modname);
	if (argc == 2)
	{
		hal_comp_id = hal_init(modname);
		if (hal_comp_id >= 1)
		{
		    signal(SIGINT, exit_handler);
		    signal(SIGTERM, exit_handler);
		    atexit(call_hal_exit);

		    fprintf(stderr, "%s:         LicMgr init\n", modname);
		    std::string ini_file_path(argv[1]);
		    LicenseMgr licmgr(ini_file_path);
		    licmgr.perform_boot_maintenance();

		    // Read of hal script is holding off for us to be ready - esp. hostmot2 so do not
		    // signal ready until no more mesaflash interaction.
		    hal_ready(hal_comp_id);

		    // Just kill time until exit when we might need to write nv flash
		    while (1)
		    {
		        sleep(60);
		    }
		}
		else
	    {
	        fprintf(stderr, "%s: hal_init failed\n", modname);
	    }
	}
	else
	{
		fprintf(stderr, "%s: required ini file path argument missing\n", modname);
	}

	exit(retcode);
}
