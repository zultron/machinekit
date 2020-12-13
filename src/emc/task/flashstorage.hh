
#pragma once

#include <string>

#include <inifile.hh>


class FlashStorage
{
private:
    std::string _machine_public_key;
    std::string _license_public_key;
    std::string _machine_jwt_encoded;

    std::string _mesa_board;    // 5i25, 7i92, or ECM1
    std::string _mesa_ipaddr;   // Only used for 7i92 or ECM1 boards

    void recursive_rmdir(const std::string& dirpath);

    bool create_random_datafile(std::string& path);

    time_t read_timestamp(const std::string& path);

    std::string read_entire_file(const std::string& path);

    bool mesaflash_exec(const std::string& rwcmd,     // --nvread    or   --nvwrite
                        const std::string& filepath,
                        const std::string& nvblock);

    bool zip_exec(const std::string& contents_dir_path,
                  std::string& zipfile_path);

    bool unzip_exec(const std::string& zipfilepath,
                    const std::string& tmpdirpath);

    bool read_flash_block(const std::string& block, std::string& zipdir, time_t& timestamp);

public:
    FlashStorage(IniFile& inifile);
    FlashStorage(const std::string& board, const std::string& ipaddr)
    {
        this->_mesa_board = board;
        this->_mesa_ipaddr = ipaddr;
    }

    bool read_flash();
    bool write_flash(const std::string& contents_dir_path);
};
