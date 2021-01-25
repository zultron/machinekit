#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

#include "storage.hh"
#include "licensemgr.hh"

using namespace std;

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
    string board;
    string address;
} MESA_DEVICE;

int main(int argc, char** argv)
{
	int retcode = 1;

    MESA_DEVICE devices[] = { { "5i25", "" },
                              { "ECM1", "10.10.10.10" },
                              { "7i92", "10.10.10.10" } };

	// The problem this app is intended to solve is the fresh boot of a controller
	// that has no idea what it is attached to (no machine.json or pathpilot.json).
	// But the machine ECM1 or 7i92 or 5i25 or Torstep flash has been written
	// previously by the factory during final QC or by the 2.8.0 release
	// which started to do that, easing emergency recovery for customers where they
	// need to completely wipe the drive and boot fresh again.

    // So there is no ini file path.
    // If so, we start to gently probe trying to read the flash successfully
    // of the 5i25, then ECM1, then 7i92, then Torstep for success.

    string PATHPILOTJSON_FILEPATH(getenv("HOME"));
    PATHPILOTJSON_FILEPATH.append("/pathpilot.json");
    if (0 != access(PATHPILOTJSON_FILEPATH.c_str(), F_OK))
    {
        bool result = false;
        DEBUG_LOG("configcheck - probing for flash on devices");
        for (unsigned int ix = 0; !result && (ix < NUM_ELEMENTS(devices)); ++ix)
        {
            Storage* pStorage = new Storage(devices[ix].board, devices[ix].address);

            // Try to read the flash on this device.
            DEBUG_LOG(devices[ix].board.c_str());

            result = pStorage->read_flash();
            if (result)
            {
                DEBUG_LOG("configcheck - flash read successful");

                // Found the right mesa device for this machine and controller.
                // Overwrite machine.json BUT preserve
                //    lic key:value pair if its a 1
                // critically picks up from flash
                //    - guid, sn, machine class, model
                auto pclaims = pStorage->get_machine_jwt()->get_payload_claims();

                if (0 == access(pStorage->MACHINEJSON_FILEPATH.c_str(), F_OK))
                {
                    ifstream jsonfile;
                    jsonfile.open(pStorage->MACHINEJSON_FILEPATH, ios_base::in);
                    jsonfile.seekg(0, ios::end);
                    size_t cb = jsonfile.tellg();
                    jsonfile.seekg(0);
                    char* buffer = new char[cb+1];
                    jsonfile.read(buffer, cb);
                    buffer[cb] = '\0';
                    jsonfile.close();
                    nlohmann::json machinejson = nlohmann::json::parse(buffer);
                    delete[] buffer;
                    buffer = NULL;
                    int lic = machinejson["lic"];
                    pclaims["lic"] = jwt::basic_claim<nlohmann_traits>(lic);
                }

                LicenseMgr::write_json_claims(pStorage->MACHINEJSON_FILEPATH,
                                              pclaims);

                // Then call config chooser to generate pathpilot.json from the model
                // and mesa device we know is working (5i25 or 7i92 config for stuff that has BOTH of them)

                retcode = 0;
            }

            delete pStorage;
            pStorage = NULL;
        }
    }

	return retcode;
}
