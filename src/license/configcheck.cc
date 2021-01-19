#include <stdio.h>
#include <string.h>

#include "licensemgr.hh"

// TODO
#define DEBUG

#ifdef DEBUG
#include "debugbreak.h"
#define DEBUG_LOG(x) {fprintf(stderr, "%s\n", x);}
#define VERIFY(x) {if (!(x)) debug_break();}
#else
#define DEBUG_LOG(x)
#define VERIFY(x) {if (x) ;}
#endif


int main(int argc, char** argv)
{
	int retcode = 1;
	DEBUG_LOG("starting");

	// The problem this app is intended to solve is the fresh boot of a controller
	// that has no idea what it is attached to (no machine.json or pathpilot.json).
	// But the machine ECM1 or 7i92 or 5i25 or Torstep flash has been written
	// previously by the factory during final QC or by the 2.8.0 release
	// which started to do that, easing emergency recovery for customers where they
	// need to completely wipe the drive and boot fresh again.

	// So there is no ini file path.
	// Instead we look to see if pathpilot.json and machine.json are both missing.
	// If so, we start to gently probe trying to read the flash successfully
	// of the 5i25 and then the 7i25, ECM1, and Torstep for success.

    //pLicMgr = new LicenseMgr(ini_file_path);
    //pLicMgr->perform_boot_maintenance();

	exit(retcode);
}
