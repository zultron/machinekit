//
// This is a userspace HAL driver for Tormach licensing control
// which can run prior to hostmot2 drivers being initialized.
//
// Copyright 2020 Tormach, LLC
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
#include <wait.h>
#include <poll.h>

#include <string.h>

#include "hal.h"
#include "hal_priv.h"
#include "licensemgr.hh"

// TODO
#define DEBUG

#ifdef DEBUG
#include "debugbreak.h"
#define DEBUG_LOG(x) {fprintf(stderr, "%s: %s\n", modname, x);}
#define VERIFY(x) {if (!(x)) debug_break();}
#else
#define DEBUG_LOG(x)
#define VERIFY(x) x
#endif


// misdirection for user comp name
char modname[] = {"hwmon\0"};

int hal_comp_id;

LicenseMgr* pLicMgr = NULL;

static void exit_handler(int sig)
{
	int total_wait_sec = 0;
	while ((total_wait_sec < 15) &&
	       (NULL != halpr_find_comp_by_name("hostmot2")))
	{
		DEBUG_LOG("waiting up to 15 seconds for hostmot2 to disappear...");
		sleep(1);
		++total_wait_sec;
	}

	if (NULL == halpr_find_comp_by_name("hostmot2"))
	{
	    if (pLicMgr)
	    {
			DEBUG_LOG("hostmot2 is gone!");
		    pLicMgr->perform_shutdown_maintenance();
		}
	}

    if (pLicMgr)
    {
	    delete pLicMgr;
	    pLicMgr = NULL;
	}

	DEBUG_LOG("exiting");

    exit(0);
}


static void call_hal_exit(void)
{
    hal_exit(hal_comp_id);
}


void trim_key_bookends(std::string& public_key)
{
    const char header[] = {"-----BEGIN PUBLIC KEY-----\n"};
    const char footer[] = {"-----END PUBLIC KEY-----\n"};
    size_t pos = public_key.find(header);
    if (std::string::npos != pos)
    {
        public_key.erase(pos, strlen(header));
    }

    pos = public_key.find(footer);
    if (std::string::npos != pos)
    {
        public_key.erase(pos, strlen(footer));
    }
}


bool read_entire_file(const char* path, std::string& contents)
{
    bool result = false;
    int fd = open(path, O_RDONLY);
    if (fd >= 0)
    {
        struct stat sb;
        if (0 == fstat(fd, &sb))
        {
            char* buffer = new char[sb.st_size + 1];
            if (sb.st_size == read(fd, buffer, sb.st_size))
            {
                buffer[sb.st_size] = '\0';
                contents.assign(buffer);
                result = true;
            }
            delete[] buffer;
            buffer = NULL;
        }

        close(fd);
        fd = -1;
    }

    return result;
}


bool refresh_public_key_file(const char* filename)
{
	DEBUG_LOG("refreshing public key file...");

	bool result = false;
	char scriptfilename[] = { "/tmp/XXXXXX" };
	char tmpfilename[] = { "/tmp/XXXXXX" };
    int fd = mkstemp(tmpfilename);
    if (-1 != fd)
	{
		close(fd);
		fd = mkstemp(scriptfilename);
		if (-1 != fd)
		{
		    pid_t pid = fork();
		    if (pid == 0)
		    {
		        // Child
                // We don't want the output of the exec program to be visible in the
                // normal inherited stdout and stderr logs as it reveals a lot of detail.

                #ifndef DEBUG
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                #endif

		        int ev = 1;
  	        	char cmdline[1024];
		        sprintf(cmdline, "/usr/bin/openssl s_client -servername hub.pathpilot.com -connect hub.pathpilot.com:443 < /dev/null 2> /dev/null | /usr/bin/openssl x509 -pubkey -noout > %s\n", tmpfilename);
		        ssize_t cb = (ssize_t) strlen(cmdline);
				if (write(fd, cmdline, cb) == cb)
				{
					close(fd);
					fd = -1;
			        ev = execlp("/bin/bash",
			                    "/bin/bash",
			                    scriptfilename,
			                    NULL);
				}
				else
				{
					close(fd);
					fd = -1;
				}
		        exit(ev);
		    }
		    else
		    {
		    	// Parent
		    	close(fd);
		    	fd = -1;
		        int status = 0;
		        if (pid == waitpid(pid, &status, 0))
		        {
		            if (WIFEXITED(status))
		            {
		            	if (0 == WEXITSTATUS(status))
		            	{
					        // Trim the bookends from the public key file
					        std::string pubkey;
					        result = read_entire_file(tmpfilename, pubkey);
					        if (result)
					        {
					        	trim_key_bookends(pubkey);
					        	fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
					        	if (-1 != fd)
					        	{
					        		if (write(fd, pubkey.c_str(), pubkey.length()) == (ssize_t) pubkey.length())
					        		{
										DEBUG_LOG("success!");
					        			result = true;
					        		}

					        		close(fd);
					        		fd = -1;
					        	}
					        }
		            	}
		            }
		        }
		    }

		    unlink(scriptfilename);
		}

		unlink(tmpfilename);
	}

    return result;
}


int main(int argc, char *argv[])
{
	int retcode = 1;
	DEBUG_LOG("starting");
	if (argc == 2)
	{
		hal_comp_id = hal_init(modname);
		if (hal_comp_id >= 1)
		{
		    signal(SIGINT, exit_handler);
		    signal(SIGTERM, exit_handler);
		    atexit(call_hal_exit);

		    std::string ini_file_path(argv[1]);
		    pLicMgr = new LicenseMgr(ini_file_path);
		    pLicMgr->perform_boot_maintenance();

		    // Read of hal script is holding off for us to be ready - esp. hostmot2 so do not
		    // signal ready until no more mesaflash interaction.
		    hal_ready(hal_comp_id);

		    // Use linux inotify kernel system to efficiently get notified right away
		    // if anything in the directory we store the ssd files in changes.
		    // The PP UI HUB code will acquire new machine and license jwts and write them
		    // there.  Then we wake up and refresh the cached SSL public keys from the cert
		    // as needed.  This way the public key code isn't anywhere near the code that
		    // updates the jwt dat files and is hidden in compiled C++ as well.

		    int eventq_fd = inotify_init();
		    if (-1 != eventq_fd)
		    {
		    	// IN_MODIFY does not work so well.  In practice, you get 2 notification events
		    	// for each file for some reason the way the crt libs work.  But using IN_CLOSE_WRITE
		    	// is perfect.  Notifies us when a file that was opened for writing is closed.

			    int datadir_fd = inotify_add_watch(eventq_fd, "/boot/home/.d", IN_CLOSE_WRITE);
			    if (-1 != datadir_fd)
			    {
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

					    DEBUG_LOG("into ppoll");
					    int ready = ppoll(&pfd, 1, &timeout, NULL);  // Efficiently block for 60 seconds
					    if (ready > 0)
					    {
						    DEBUG_LOG("ppoll indicates ready event");

					    	// read an event
					    	ssize_t cb = read(eventq_fd, event, event_cb);
					    	if (cb >= (ssize_t)sizeof(struct inotify_event))
					    	{
					    		// can at least trust the struct members
					    		if (datadir_fd == event->wd)
					    		{
					    			#ifdef DEBUG
					    			fprintf(stderr, "%s:  %s changed   mask=%x\n", modname, event->name, event->mask);
					    			#endif

					    			if (0 == strcmp(event->name, ".m"))
					    			{
							    		refresh_public_key_file("/boot/home/.d/.0");
							    	}
							    	else if (0 == strcmp(event->name, ".l"))
							    	{
							    		refresh_public_key_file("/boot/home/.d/.1");
							    	}
						    	}
					    	}
					    }
					}

					delete[] event;
					event = NULL;

				    VERIFY(inotify_rm_watch(eventq_fd, datadir_fd) == 0);
				    VERIFY(close(datadir_fd) == 0);
				}

			    VERIFY(close(eventq_fd) == 0);
			}
		}
		else
	    {
	       	DEBUG_LOG("hal_init failed");
	    }
	}
	else
	{
		fprintf(stderr, "%s: required ini file path argument missing\n", modname);
	}

	exit(retcode);
}
