
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
#include <string>

#include "licensemgr.hh"

using namespace std;

#if 0
bool is_license_expired()
{
                        // Now expire_timestamp_utc points to just the timestamp.
                        printf("expire timestamp: %s\n", expire_timestamp_utc);

                        // HUB returns license dates in UTC time zone in this format which
                        // conveniently maps to the strptime function from time.h
                        // '%Y-%m-%d %H:%M:%S'

                        tzset();  // time functions now aware of proper local time zone...
                        time_t now_utc = time(NULL);

                        struct tm expiretm;
                        ZERO_STRUCT(&expiretm);
                        if (strptime(expire_timestamp_utc, "%Y-%m-%d %H:%M:%S", &expiretm) != NULL)
                        {
                            // convert from struct tm in utc time to utc time_t for easy comparision
                            time_t expire_utc = timegm(&expiretm);
                            if (expire_utc > now_utc)
                            {
                                license_expired = false;  // license still has time left
                            }
                        }

    return license_expired;
}
#endif


LicenseMgr::LicenseMgr(const string& ini_file_path) :
    _storage(ini_file_path),
    _flash_update_needed(false),
    _license_type(LICENSE_TYPE_TIMELIMITED),
    _p_machine_jwt_decoded(NULL),
    _p_license_jwt_decoded(NULL)
{
}

LicenseMgr::~LicenseMgr()
{
    if (_p_machine_jwt_decoded)
    {
        delete _p_machine_jwt_decoded;
        _p_machine_jwt_decoded = NULL;
    }
    if (_p_license_jwt_decoded)
    {
        delete _p_license_jwt_decoded;
        _p_license_jwt_decoded = NULL;
    }
}

bool LicenseMgr::verify_machine_jwt()
{
    // Decode the machine jwt
    bool result = false;
    try
    {
        if (_p_machine_jwt_decoded)
        {
            delete _p_machine_jwt_decoded;
            _p_machine_jwt_decoded = NULL;
        }
        _p_machine_jwt_decoded = new jwt::decoded_jwt<nlohmann_traits>(_storage.get_machine_jwt_encoded());
        // Verify the jwt signature with the public key
        const string& public_key = _storage.get_machine_public_key();
        if (public_key.length() > 0)
        {
            auto verify = jwt::verify<jwt::default_clock, nlohmann_traits>({})
                            .allow_algorithm(jwt::algorithm::rs256(public_key, "", "", ""));
            try
            {
                verify.verify(*_p_machine_jwt_decoded);

                _license_type = static_cast<license_type_t>(_p_machine_jwt_decoded->get_payload_claim("lic").as_int());
                if ((_license_type != LICENSE_TYPE_UNMANAGED) && (_license_type != LICENSE_TYPE_TIMELIMITED))
                {
                    // Unsupported value - treat anything suspicious as requiring a license.
                    _license_type = LICENSE_TYPE_TIMELIMITED;
                }

               result = true;
            }
            catch (jwt::error::signature_verification_exception ex)
            {
            }
        }

        if (! result)
        {
            delete _p_license_jwt_decoded;
            _p_license_jwt_decoded = NULL;
            _flash_update_needed = false;  // We never write unverified jwts to flash
        }
    }
    catch (invalid_argument ex)
    {
        // Thrown when machine encoded string isn't valid
    }
    return result;
}


bool LicenseMgr::verify_license_jwt()
{
    // Decode the license jwt
    bool result = false;
    try
    {
        if (_p_license_jwt_decoded)
        {
            delete _p_license_jwt_decoded;
            _p_license_jwt_decoded = NULL;
        }
        _p_license_jwt_decoded = new jwt::decoded_jwt<nlohmann_traits>(_storage.get_license_jwt_encoded());

        // Verify the jwt signature with the public key
        const string& public_key = _storage.get_license_public_key();
        if (public_key.length() > 0)
        {
            auto verify = jwt::verify<jwt::default_clock, nlohmann_traits>({})
                            .allow_algorithm(jwt::algorithm::rs256(public_key, "", "", ""));
            try
            {
                verify.verify(*_p_license_jwt_decoded);
                result = true;
            }
            catch (jwt::error::signature_verification_exception ex)
            {
            }
        }

        if (! result)
        {
            delete _p_license_jwt_decoded;
            _p_license_jwt_decoded = NULL;
        }

    }
    catch (invalid_argument ex)
    {
        // Thrown when machine encoded string isn't valid
    }
    return result;
}


bool LicenseMgr::check_all_claims()
{
    bool valid = true;

    // ver
    if (_p_machine_jwt_decoded->get_payload_claim("ver").as_int() < 1)
    {
        valid = false;
    }
    if (_p_license_jwt_decoded->get_payload_claim("ver").as_int() < 1)
    {
        valid = false;
    }

    // check iat claims in both jwts
    chrono::system_clock::time_point iat = _p_machine_jwt_decoded->get_issued_at();
    chrono::system_clock::time_point now = chrono::system_clock::now();
    if (iat > now)
    {
        valid = false; // suspicious.
    }
    iat = _p_license_jwt_decoded->get_issued_at();
    if (iat > now)
    {
        valid = false; // suspicious.
    }

    // Correlate license jwt claims with machine jwt to make sure its for the same machine.
    if (_p_machine_jwt_decoded->get_payload_claim("guid").as_string() != _p_license_jwt_decoded->get_payload_claim("guid").as_string())
    {
        valid = false;
    }
    if (_p_machine_jwt_decoded->get_payload_claim("sn").as_string() != _p_license_jwt_decoded->get_payload_claim("sn").as_string())
    {
        valid = false;
    }

    // exp for license
    chrono::system_clock::time_point exp = _p_license_jwt_decoded->get_expires_at();
    if (now > exp)
    {
        valid = false;
    }

    return valid;
}


void LicenseMgr::write_json_claims(
    const std::string& filepath,
    const std::unordered_map< typename nlohmann_traits::string_type, jwt::basic_claim<nlohmann_traits>  >& claims)
{
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


void LicenseMgr::update_json_files()
{
    // We can trust the claims of jwt
    if (_p_machine_jwt_decoded)
    {
        // Update the cleartext machine.json for the benefit of the python UI
        // Maybe the json library or the jwt library have some way to serialize
        // to a json string, but damned if I can figure that out (c++ header only
        // template libraries are unreadable by anyone other than a compiler).
        write_json_claims(_storage.MACHINEJSON_FILEPATH,
                          _p_machine_jwt_decoded->get_payload_claims());
    }
    else
    {
        unlink(_storage.MACHINEJSON_FILEPATH.c_str());
    }

    if (_p_license_jwt_decoded)
    {
        // Update the cleartext license.json for the benefit of the python UI
        // Maybe the json library or the jwt library have some way to serialize
        // to a json string, but damned if I can figure that out (c++ header only
        // template libraries are unreadable by anyone other than a compiler).
        write_json_claims(_storage.LICENSEJSON_FILEPATH,
                          _p_license_jwt_decoded->get_payload_claims());
    }
    else
    {
        unlink(_storage.LICENSEJSON_FILEPATH.c_str());
    }
}


void LicenseMgr::perform_boot_maintenance()
{
    // out-of-box context for time limited machines is machine jwt and public key exist.
    // but license jwt and public key do not.

    // Read the jwts and public keys from NV flash
    bool result = _storage.read_flash();
    if (result)
    {
        cerr << "               flash read successful" << endl;

        // TODO
        // WHEN are the SSD files overwritten with flash contents?
        // Flash is harder to spoof and the custody chain from it
        // to the Cycle Start button verification flows through SSD jwt files.
        // Could always overwrite them, but then we risk
        //    overwriting a newer jwt on disk that hub wrote that hasn't made it into flash yet (power loss)
        //    breaking license verification is cached license public key is old and the new one is on the SSD
    }
    else
    {
        cerr << "               flash read failed - either never written to or corrupt - falling back to ssd files" << endl;

        // Flash has either never been written or is corrupt.
        _flash_update_needed = true;

        // Read the SSD files instead
        result = _storage.read_ssd_files();
        if (! result)
        {
            // flash never written to or corrupt
            // ssd files not present
            // have to assume we are a license unmanaged machine

            // TODO
            // check to make sure NONE of the license related files exist
            // as the above may fail if they exist, but pri and sec don't match or something fishy like that.
            _license_type = LICENSE_TYPE_UNMANAGED;
        }
    }

    if ((result) && (_license_type == LICENSE_TYPE_TIMELIMITED))
    {
        result = verify_machine_jwt();
        if ((result) && (_license_type == LICENSE_TYPE_TIMELIMITED))
        {
            // out of box context is machine jwt should verify, but there is no license yet
            // so this might fail, but we still want to update flash with valid machine jwt.
            verify_license_jwt();

            if (_flash_update_needed)
            {
                // this writes the verified machine jwt, machine public key,
                // and license public key if it exists
                _storage.write_flash();
            }
        }
    }

    // we don't do any jwt claim checking here because that's all enforced
    // if necessary at Cycle Start time with the call to is_license_valid()

    update_json_files();
}


void LicenseMgr::perform_emctaskmain_init()
{
    // Goal is to read the SSD files and validate enough
    // to reliably tell at this point what the license type is.
    // Cannot read flash at this point so must rely on SSD files.
    bool result = _storage.read_ssd_files();
    if (result)
    {
        verify_machine_jwt();  // updates _license_type from jwt
    }
    else
    {
        // TODO
        // check to make sure NONE of the license related files exist
        // as the above may fail if they exist, but pri and sec don't match or something fishy like that.
        _license_type = LICENSE_TYPE_UNMANAGED;
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
        valid = _storage.read_ssd_files();
        if (valid)
        {
            valid = verify_machine_jwt();
            if (valid)
            {
                // we can't verify a license without a machine jwt to match claims against
                // so don't even bother trying to verify the license unless machine does first.
                valid = verify_license_jwt();
                if (valid)
                {
                    valid = check_all_claims();
                }
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
