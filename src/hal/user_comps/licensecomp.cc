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
#include <sys/inotify.h>
#include <poll.h>

#include <string.h>

#include "hal.h"
#include "licensemgr.hh"

#include "debugbreak.h"

// misdirection for user comp name
char modname[] = {"hwmon\0"};

int hal_comp_id;

LicenseMgr* pLicMgr = NULL;


static void exit_handler(int sig)
{
    fprintf(stderr, "%s: performing shutdown maintenance\n", modname);

    // TODO
    // "halcmd list comp" over and over until hostmot2 not listed

    if (pLicMgr)
    {
	    pLicMgr->perform_shutdown_maintenance();
	    delete pLicMgr;
	    pLicMgr = NULL;
	}

    fprintf(stderr, "%s: exiting\n", modname);

    exit(0);
}


static void call_hal_exit(void)
{
    hal_exit(hal_comp_id);
}

// TODO
#define DEBUG
#ifdef DEBUG
#define VERIFY(x) {if (!(x)) debug_break();}
#else
#define VERIFY(x) x
#endif

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

		    // TODO
		    fprintf(stderr, "%s:         LicMgr init\n", modname);
		    std::string ini_file_path(argv[1]);

		    pLicMgr = new LicenseMgr(ini_file_path);
		    pLicMgr->perform_boot_maintenance();

		    // Read of hal script is holding off for us to be ready - esp. hostmot2 so do not
		    // signal ready until no more mesaflash interaction.
		    hal_ready(hal_comp_id);

		    int eventq_fd = inotify_init();
		    int machinedat_fd = inotify_add_watch(eventq_fd, "/boot/home/.d/.m", IN_CREATE | IN_MODIFY);
		    int licensedat_fd = inotify_add_watch(eventq_fd, "/boot/home/.d/.l", IN_CREATE | IN_MODIFY);

			ssize_t event_cb = sizeof(struct inotify_event) + NAME_MAX + 1;
		    struct inotify_event* event = (struct inotify_event*) new char[event_cb];

		    while (1)
		    {
			    // efficiently block on eventq_fd or timeout poll
			    struct pollfd pfd;
			    pfd.fd = eventq_fd;
			    pfd.events = POLLIN;
			    pfd.revents = 0;

			    struct timespec timeout;
			    timeout.tv_sec = 60;
			    timeout.tv_nsec = 0;

			    fprintf(stderr, "%s: into ppoll\n", modname);
			    int ready = ppoll(&pfd, 1, &timeout, NULL);  // Efficiently block for 60 seconds
			    if (ready > 0)
			    {
				    fprintf(stderr, "%s: out of ppoll\n", modname);

			    	// read an event
			    	ssize_t cb = read(eventq_fd, event, event_cb);
			    	if (cb >= (ssize_t)sizeof(struct inotify_event))
			    	{
			    		// can at least trust the struct members
			    		if (machinedat_fd == event->wd)
			    		{
						    fprintf(stderr, "%s: machine.dat changed\n", modname);
			    			// update the machine jwt public key
			    		}
			    		else if (licensedat_fd == event->wd)
			    		{
						    fprintf(stderr, "%s: license.dat changed\n", modname);
			    			// update the machine jwt public key
			    		}
			    	}
			    }
			}

			delete[] event;
			event = NULL;

		    VERIFY(inotify_rm_watch(eventq_fd, machinedat_fd) == 0);
		    VERIFY(inotify_rm_watch(eventq_fd, licensedat_fd) == 0);

		    VERIFY(close(machinedat_fd) == 0);
		    VERIFY(close(licensedat_fd) == 0);
		    VERIFY(close(eventq_fd) == 0);

	    	// slight chance that we're 'late' and operator stabs the cycle start
	    	// before we can update the files?
			// /bin/bash -c "openssl s_client -servername hub.pathpilot.com -connect hub.pathpilot.com:443 < /dev/null 2> /dev/null | openssl x509 -pubkey -noout > public.pem"
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
