
#pragma once

#include <string>

#include <inifile.hh>


class Storage
{
private:
    std::string _machine_public_key;
    std::string _license_public_key;
    std::string _machine_jwt_encoded;
    std::string _license_jwt_encoded;

    std::string _mesa_board;    // 5i25, 7i92, or ECM1
    std::string _mesa_ipaddr;   // Only used for 7i92 or ECM1 boards

    // TODO
    // store mtimes for each of these files
    // have read_ssd_files check those and skip even reading the file
    // if the time hasn't changed.
    std::string PRI_MACHINEDAT_FILEPATH;
    std::string SEC_MACHINEDAT_FILEPATH;

    std::string PRI_LICENSEDAT_FILEPATH;
    std::string SEC_LICENSEDAT_FILEPATH;

    std::string MACHINEPUBKEY_FILEPATH;
    std::string LICENSEPUBKEY_FILEPATH;


    void recursive_rmdir(const std::string& dirpath);

    bool create_random_datafile(std::string& path);

    time_t read_timestamp(const std::string& path);

    bool read_entire_file(const std::string& path, std::string& contents);
    bool write_file(const std::string& filepath, const std::string data);

    bool mesaflash_exec(const std::string& rwcmd,     // --nvread    or   --nvwrite
                        const std::string& filepath,
                        const std::string& nvblock);

    bool zip_exec(const std::string& contents_dir_path,
                  std::string& zipfile_path);

    bool unzip_exec(const std::string& zipfilepath,
                    const std::string& tmpdirpath);

    // These the zip container to house the keys inside the flash zip
    bool read_public_keys(const std::string& zipfilepath);
    bool write_public_keys(std::string& zipfile_path);

    // These use the files scattered on the SSD drive
    bool read_public_keys_from_files();
    bool write_public_keys_to_files();

    void trim_key_bookends(std::string& public_key);
    void restore_key_bookends(std::string& public_key);

    bool read_flash_block(const std::string& block, std::string& zipdir, time_t& timestamp);


public:
    std::string MACHINEJSON_FILEPATH;
    std::string LICENSEJSON_FILEPATH;

    Storage(const std::string& ini_file_path);

    bool read_flash();
    bool write_flash();

    bool read_ssd_files();

    const std::string& get_machine_jwt_encoded() { return _machine_jwt_encoded; }
    const std::string& get_license_jwt_encoded() { return _license_jwt_encoded; }
    const std::string& get_machine_public_key() { return _machine_public_key; }
    const std::string& get_license_public_key() { return _license_public_key; }
};
