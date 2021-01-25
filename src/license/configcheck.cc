#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "storage.hh"
#include "licensemgr.hh"

#ifdef DEBUG
#include "debugbreak.h"
#define DEBUG_LOG(x) {fprintf(stderr, "%s\n", x);}
#define VERIFY(x) {if (!(x)) debug_break();}
#else
#define DEBUG_LOG(x)
#define VERIFY(x) {if (x) ;}
#endif

typedef struct
{
    std::string board;
    std::string address;
} MESA_DEVICE;

int main(int argc, char** argv)
{
	int retcode = 1;
	DEBUG_LOG("starting");

    MESA_DEVICE devices[] = { { "5i25", "" },
                              { "ECM1", "10.10.10.10" },
                              { "7i92", "10.10.10.10" } };

    for (unsigned int ix = 0; ix < NUM_ELEMENTS(devices); ++ix)
    {
        DEBUG_LOG(devices[ix].board.c_str());
    }

	// The problem this app is intended to solve is the fresh boot of a controller
	// that has no idea what it is attached to (no machine.json or pathpilot.json).
	// But the machine ECM1 or 7i92 or 5i25 or Torstep flash has been written
	// previously by the factory during final QC or by the 2.8.0 release
	// which started to do that, easing emergency recovery for customers where they
	// need to completely wipe the drive and boot fresh again.

    // So there is no ini file path.
    // If so, we start to gently probe trying to read the flash successfully
    // of the 5i25, then ECM1, then 7i92, then Torstep for success.

    std::string PATHPILOTJSON_FILEPATH(std::getenv("HOME"));
    PATHPILOTJSON_FILEPATH.append("/pathpilot.json");
    if (0 != access(PATHPILOTJSON_FILEPATH.c_str(), F_OK))
    {
        bool result = false;
        for (unsigned int ix; !result && (ix < NUM_ELEMENTS(devices)); ++ix)
        {
            Storage* pStorage = new Storage(devices[ix].board, devices[ix].address);
            if (0 != access(pStorage->MACHINEJSON_FILEPATH.c_str(), F_OK))
            {
                // Try to read the flash on this device.
                result = pStorage->read_flash();
                if (result)
                {
                    // Bingo!
                    // Write out machine.json, pathpilot.json from the flash block
                    retcode = 0;
                }
            }

            delete pStorage;
            pStorage = NULL;
        }
    }

	return retcode;
}
