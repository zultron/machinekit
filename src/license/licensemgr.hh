#pragma once

#include "storage.hh"

enum license_type_t
{
    LICENSE_TYPE_UNMANAGED   = 0,
    LICENSE_TYPE_TIMELIMITED
};

class LicenseMgr
{
private:
    Storage _flash_storage;
    Storage _ssd_storage;

    bool _flash_update_needed;
    license_type_t _license_type;

    bool check_all_claims();
    void write_json_claims(const std::string& filepath,
                           const std::unordered_map< typename nlohmann_traits::string_type, jwt::basic_claim<nlohmann_traits>  >& claims);

public:
    LicenseMgr(const std::string& ini_file_path);

    // Used by licensecomp user comp (which is named hwmon as misdirection)
    void perform_boot_maintenance();
    void perform_shutdown_maintenance();

    // Called by emctaskmain
    void perform_emctaskmain_init();
    // Called by Cycle Start effectively - any time we try to switch into EMC_TASK_MODE_AUTO
    bool is_license_valid();
};
