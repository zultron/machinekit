
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>

#include "licensemgr.hh"

using namespace std;

// TODO
#define DEBUG
#ifdef DEBUG
#define DEBUG_LOG(x) {cerr << x << endl;}
#else
#define DEBUG_LOG(x)
#endif

LicenseMgr::LicenseMgr(const string& ini_file_path) :
    _flash_storage(ini_file_path),
    _ssd_storage(ini_file_path),
    _flash_update_needed(false),
    _license_type(LICENSE_TYPE_TIMELIMITED)
{
}


bool LicenseMgr::check_all_claims()
{
    bool valid = true;

    // ver
    if (_ssd_storage.get_machine_jwt()->get_payload_claim("ver").as_int() < 1)
    {
        valid = false;
    }
    if (_ssd_storage.get_license_jwt()->get_payload_claim("ver").as_int() < 1)
    {
        valid = false;
    }

    // check iat claims in both jwts
    chrono::system_clock::time_point now = chrono::system_clock::now();
    chrono::system_clock::time_point iat = _ssd_storage.get_machine_jwt()->get_issued_at();
    if (iat > now)
    {
        DEBUG_LOG("                       machine jwt iat is in the future");
        valid = false; // suspicious.
    }
    iat = _ssd_storage.get_license_jwt()->get_issued_at();
    if (iat > now)
    {
        DEBUG_LOG("                       license jwt iat is in the future");
        valid = false; // suspicious.
    }

    // Correlate license jwt claims with machine jwt to make sure its for the same machine.
    if (_ssd_storage.get_machine_jwt()->get_payload_claim("guid").as_string() != _ssd_storage.get_license_jwt()->get_payload_claim("guid").as_string())
    {
        DEBUG_LOG("                       machine and license jwt guid mismatch");
        valid = false;
    }
    if (_ssd_storage.get_machine_jwt()->get_payload_claim("sn").as_string() != _ssd_storage.get_license_jwt()->get_payload_claim("sn").as_string())
    {
        DEBUG_LOG("                       machine and license sn mismatch");
        valid = false;
    }

    // exp for license
    chrono::system_clock::time_point exp = _ssd_storage.get_license_jwt()->get_expires_at();
    if (now > exp)
    {
        DEBUG_LOG("                       license exp time is in the past");
        valid = false;
    }

    return valid;
}


void LicenseMgr::write_json_claims(
    const std::string& filepath,
    const std::unordered_map< typename nlohmann_traits::string_type, jwt::basic_claim<nlohmann_traits>  >& claims)
{
    // Maybe the json library or the jwt library have some way to serialize
    // to a json string, but damned if I can figure that out (c++ header only
    // template libraries are unreadable by anyone other than a compiler).

    fstream json;
    json.open(filepath, ios_base::out | ios_base::trunc);
    json << "{" << endl;
    bool first = true;
    for (auto &e : claims)
    {
        if (! first)
        {
            json << "," << endl;
        }
        json << "    \"" << e.first << "\": " << e.second.to_json();
        first = false;
    }
    json << endl << "}" << endl;
    json.close();
}


void LicenseMgr::perform_boot_maintenance()
{
    // out-of-box context for time limited machines is machine jwt and public key exist.
    // but license jwt and public key do not.

    // Read the jwts and public keys from NV flash
    bool flash_result = _flash_storage.read_flash();
    if (flash_result)
    {
        _license_type = static_cast<license_type_t>(_flash_storage.get_machine_jwt()->get_payload_claim("lic").as_int());
        if ((_license_type != LICENSE_TYPE_UNMANAGED) && (_license_type != LICENSE_TYPE_TIMELIMITED))
        {
            // Unsupported value - treat anything suspicious as requiring a license.
            _license_type = LICENSE_TYPE_TIMELIMITED;
        }
    }

    bool ssd_result = _ssd_storage.read_ssd_files();
    if ((flash_result) && (ssd_result))
    {
        DEBUG_LOG("               flash and ssd reads successful");

        // The ssd files might be 'newer' than flash contents because writing flash
        // never got a chance to occur on graceful shutdown.
        if (_ssd_storage.get_machine_jwt()->get_issued_at() >
            _flash_storage.get_machine_jwt()->get_issued_at())
        {
            // ssd files are newer, let's write them to flash as long as it appears to match.
            // in the out of box scenario on first boot we may have written a mostly empty, but verifiable
            // machine.jwt to flash that doesn't have a guid yet (and perhaps there was no graceful power down
            // where hwmon user comp would have written it).
            std::string ssd_guid = _ssd_storage.get_machine_jwt()->get_payload_claim("guid").as_string();
            std::string flash_guid = _flash_storage.get_machine_jwt()->get_payload_claim("guid").as_string();
            if ((ssd_guid == flash_guid) || ((ssd_guid.length() > 0) && (flash_guid.length() == 0)))
            {
                DEBUG_LOG("               ssd files are newer, writing to flash");

                // this writes the verified machine jwt, machine public key,
                // and license public key if it exists
                _ssd_storage.write_flash();

                // Update the cleartext machine.json for the benefit of the python UI
                write_json_claims(_ssd_storage.MACHINEJSON_FILEPATH,
                                  _ssd_storage.get_machine_jwt()->get_payload_claims());
            }
        }
        else
        {
            // Update the cleartext machine.json for the benefit of the python UI
            write_json_claims(_flash_storage.MACHINEJSON_FILEPATH,
                              _flash_storage.get_machine_jwt()->get_payload_claims());
        }

        // Update the cleartext license.json for the benefit of the python UI
        // Important to overwrite it from the jwt in case it has been altered.
        if (_ssd_storage.get_license_jwt())
        {
            write_json_claims(_ssd_storage.LICENSEJSON_FILEPATH,
                              _ssd_storage.get_license_jwt()->get_payload_claims());
        }
        else
        {
            unlink(_ssd_storage.LICENSEJSON_FILEPATH.c_str());
        }
    }
    else if ((flash_result) and (! ssd_result))
    {
        DEBUG_LOG("               flash worked, ssd files failed - using flash for machine.json");

        // might be a case where a different controller is attached to the machine in an attempt
        // to get around licensing.
        // use flash contents to lay down public keys and the machine dat.  then the UI side
        // has what it needs to acquire a fresh license from HUB.  and cycle start could
        // actually validate that the license matches the machine.
        _flash_storage.write_ssd_keys_and_machine_jwt();

        // flash machine.jwt can be relied on and used to overwrite machine.json
        // update the cleartext machine.json for the benefit of the python UI
        write_json_claims(_flash_storage.MACHINEJSON_FILEPATH,
                          _flash_storage.get_machine_jwt()->get_payload_claims());

        unlink(_ssd_storage.LICENSEJSON_FILEPATH.c_str());
    }
    else if ((! flash_result) and (ssd_result))
    {
        DEBUG_LOG("               flash failed, ssd files succeeded - writing them to flash");

        _license_type = static_cast<license_type_t>(_ssd_storage.get_machine_jwt()->get_payload_claim("lic").as_int());
        if ((_license_type != LICENSE_TYPE_UNMANAGED) && (_license_type != LICENSE_TYPE_TIMELIMITED))
        {
            // Unsupported value - treat anything suspicious as requiring a license.
            _license_type = LICENSE_TYPE_TIMELIMITED;
        }

        // this writes the verified machine jwt, machine public key,
        // and license public key if it exists
        _ssd_storage.write_flash();

        // during out of box boot, the machine jwt will only have the license flag set
        // and no guid or sn.  the config chooser sets those in the clear text json
        // file.  if we overwrite the json files with only verified machine jwt contents
        // we lose that.  so in this specific situation, we want to merge machine jwt
        // and the existing machine.json.  That way later the hub browser can provide
        // the full machine json to get verified and signed by the hub server and
        // get back a fully signed machine.jwt that can then be written into flash.
        bool json_overwrite = true;
        if ((_ssd_storage.get_machine_jwt()->get_payload_claim("sn").as_string() == "") &&
            (_ssd_storage.get_machine_jwt()->get_payload_claim("guid").as_string() == ""))
        {
            ifstream jsonfile;
            jsonfile.open(_ssd_storage.MACHINEJSON_FILEPATH, ios_base::in);
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
            if (machinejson["lic"] == _license_type)
            {
                // ok - we can skip overwriting the json in this one special case.
                json_overwrite = false;
            }
        }

        // Update the cleartext machine.json for the benefit of the python UI
        if (json_overwrite)
        {
            write_json_claims(_ssd_storage.MACHINEJSON_FILEPATH,
                              _ssd_storage.get_machine_jwt()->get_payload_claims());
        }

        // Update the cleartext license.json for the benefit of the python UI
        // Important to overwrite it from the jwt in case it has been altered.
        if (_ssd_storage.get_license_jwt())
        {
            write_json_claims(_ssd_storage.LICENSEJSON_FILEPATH,
                              _ssd_storage.get_license_jwt()->get_payload_claims());
        }
        else
        {
            unlink(_ssd_storage.LICENSEJSON_FILEPATH.c_str());
        }
    }
    else if ((! flash_result) && (! ssd_result))
    {
        // flash never written to or corrupt
        // ssd files either not present or fail verification
        // have to assume we are a license unmanaged machine, unless debris on the drive
        // reveals tampering perhaps.
        if (_ssd_storage.do_license_ssd_files_exist())
        {
            // Something fishy going on, assume its an attempt at circumventing
            _license_type = LICENSE_TYPE_TIMELIMITED;
        }
        else
        {
            _license_type = LICENSE_TYPE_UNMANAGED;
        }
    }

    // we don't do any jwt claim checking here because that's all enforced
    // if necessary at Cycle Start time with the call to is_license_valid()
}


void LicenseMgr::perform_shutdown_maintenance()
{
    // licensecomp user comp calls this during graceful shutdowns after
    // hostmot2 is dead.  this gives us a clear shot at writing to
    // flash again to archive updated public keys or jwt data

    // During PP UI run, new maachine and license jwts and public keys
    // could have been acquired compared to when we last read them.

    // Read the jwts and public keys from NV flash
    bool flash_result = _flash_storage.read_flash();
    bool ssd_result = _ssd_storage.read_ssd_files();
    if ((flash_result) && (ssd_result))
    {
        DEBUG_LOG("               flash and ssd reads successful");

        // The ssd files might be 'newer' than flash contents because writing flash
        // never got a chance to occur on graceful shutdown.
        if (_ssd_storage.get_machine_jwt()->get_issued_at() >
            _flash_storage.get_machine_jwt()->get_issued_at())
        {
            // ssd files are newer, let's write them to flash as long as it appears to match.
            // in the out of box scenario on first boot we may have written a mostly empty, but verifiable
            // machine.jwt to flash that doesn't have a guid yet (and perhaps there was no graceful power down
            // where hwmon user comp would have written it).
            std::string ssd_guid = _ssd_storage.get_machine_jwt()->get_payload_claim("guid").as_string();
            std::string flash_guid = _flash_storage.get_machine_jwt()->get_payload_claim("guid").as_string();
            if ((ssd_guid == flash_guid) || ((ssd_guid.length() > 0) && (flash_guid.length() == 0)))
            {
                DEBUG_LOG("               ssd files are newer, writing to flash");

                // this writes the verified machine jwt, machine public key,
                // and license public key if it exists
                _ssd_storage.write_flash();
            }
        }
    }
    else if ((! flash_result) && (ssd_result))
    {
        DEBUG_LOG("               flash failed, ssd files succeeded - writing them to flash");

        // this writes the verified machine jwt, machine public key,
        // and license public key if it exists
        _ssd_storage.write_flash();
    }
}


void LicenseMgr::perform_emctaskmain_init()
{
    // Goal is to read the SSD files and validate enough
    // to reliably tell at this point what the license type is.
    // Cannot read flash at this point so must rely on SSD files.
    bool result = _ssd_storage.read_ssd_files();
    if (result)
    {
        _license_type = static_cast<license_type_t>(_ssd_storage.get_machine_jwt()->get_payload_claim("lic").as_int());
        if ((_license_type != LICENSE_TYPE_UNMANAGED) && (_license_type != LICENSE_TYPE_TIMELIMITED))
        {
            // Unsupported value - treat anything suspicious as requiring a license.
            _license_type = LICENSE_TYPE_TIMELIMITED;
        }
    }
    else
    {
        // check to make sure NONE of the license related files exist
        // as the above may fail if they exist, but do not verify with public keys
        if (_ssd_storage.do_license_ssd_files_exist())
        {
            // Something fishy going on, assume its an attempt at circumventing
            _license_type = LICENSE_TYPE_TIMELIMITED;
        }
        else
        {
            _license_type = LICENSE_TYPE_UNMANAGED;
        }
    }
}


bool LicenseMgr::is_license_valid()
{
    bool valid = false;

    // Called by emctask.cc anytime we try to go into EMC_TASK_MODE_AUTO
    // (effectively every time Cycle Start button is pressed)

    // emctaskmain.cc created the LicenseMgr and already performed
    // a read_ssd_files() call to init state so we can check _license_type
    // reliably.

    // In this context we cannot read or write NV flash so we must rely on the
    // disk primary and secondary .dat files.

    if (_license_type == LICENSE_TYPE_UNMANAGED)
    {
        valid = true;
    }
    else if (_license_type == LICENSE_TYPE_TIMELIMITED)
    {
        // Refresh jwt and key files from SSD as the UI may have gotten new ones from HUB.
        valid = _ssd_storage.read_ssd_files();
        if (valid)
        {
            valid = check_all_claims();

            // regardless of whether the license is valid or not, update the
            // json file so that the UI has our latest understanding.
            // helpful for troubleshooting log data zips.
            //
            // overwrite license.json to reflect all the current thinking so that the
            // UI has the info it needs to provide context to the user for how to fix
            // (i.e. the contact info and comment from the license that is now expired).
            if (_ssd_storage.get_license_jwt())
            {
                write_json_claims(_ssd_storage.LICENSEJSON_FILEPATH,
                                  _ssd_storage.get_license_jwt()->get_payload_claims());
            }
            else
            {
                unlink(_ssd_storage.LICENSEJSON_FILEPATH.c_str());
            }
        }
    }

    return valid;
}


#if 0
// Here just for easy testing
// Compile and run just this single independent file and then run ./a.out
//     g++ -std=c++11 licensecheck.cc -lcrypto -lssl
//
int main(int argc, char** argv)
{
    bool valid = is_license_valid();
    if (valid == true)
    {
        printf("Yeah Baby we're valid\n");
        return 0;
    }

    printf("Crap, we're lying\n");
    return 1;
}
#endif
