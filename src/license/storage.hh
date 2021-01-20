#pragma once

#include <string>

#include <inifile.hh>

// Uncommenting this turns on TONS of great output.  Even better if compiled with DEBUG=1 at the shell for all of linuxcnc.
#define DEBUG

// Licenses are implemented using JSON web tokens (JWT). JWTs are used to share 'claims' securely between
// two parties - in this case the PathPilot HUB server and PathPilot software on the controller.
// They combine a json data structure, several standard algorithims for detecting modifications
// (we use RSA public/private key pairs), standard header info for identifying algorithims used, and the
// results is all base64url encoded to make it easy to transport them around.
//
// See https://jwt.io/introduction/ for more.

// Header only C++ 11 libraries for JSON Web Tokens (JWT) and JSON
// https://github.com/Thalhammer/jwt-cpp
// https://github.com/nlohmann/json

#define DISABLE_PICOJSON // Make sure JWT compiles with this flag
#include "jwt-cpp/include/jwt-cpp/jwt.h"
#include "jwt-cpp/include/nlohmann/json.hpp"

// The jwt-cpp library has a legacy json library named picojson that it still includes for backward compatibility.
// But instead the nlohmann json library is recommended by many so this traits struct lets the jwt-cpp use nlohmann
// through C++ template magic.
// "Think of a trait as a small object whose main purpose is to carry information used by another object or
//  algorithm to determine "policy" or "implementation details". - Bjarne Stroustrup

struct nlohmann_traits {
    using json = nlohmann::json;
    using value_type = json;
    using object_type = json::object_t;
    using array_type = json::array_t;
    using string_type = std::string; // current limitation of traits implementation
    using number_type = json::number_float_t;
    using integer_type = json::number_integer_t;
    using boolean_type = json::boolean_t;

    static jwt::json::type get_type(const json &val) {
        using jwt::json::type;

        if (val.type() == json::value_t::boolean)
            return type::boolean;
        else if (val.type() == json::value_t::number_integer)
            return type::integer;
        else if (val.type() == json::value_t::number_unsigned) // nlohmann internally tracks two types of integers
            return type::integer;
        else if (val.type() == json::value_t::number_float)
            return type::number;
        else if (val.type() == json::value_t::string)
            return type::string;
        else if (val.type() == json::value_t::array)
            return type::array;
        else if (val.type() == json::value_t::object)
            return type::object;
       else
            throw std::logic_error("invalid type");
    }

    static json::object_t as_object(const json &val) {
        if (val.type() != json::value_t::object)
            throw std::bad_cast();
        return val.get<json::object_t>();
    }

    static std::string as_string(const json &val) {
        if (val.type() != json::value_t::string)
            throw std::bad_cast();
        return val.get<std::string>();
    }

    static json::array_t as_array(const json &val) {
        if (val.type() != json::value_t::array)
            throw std::bad_cast();
        return val.get<json::array_t>();
    }

    static int64_t as_int(const json &val) {
        switch(val.type())
        {
        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
            return val.get<int64_t>();
        default:
            throw std::bad_cast();
        }
    }

    static bool as_bool(const json &val) {
        if (val.type() != json::value_t::boolean)
            throw std::bad_cast();
        return val.get<bool>();
    }

    static double as_number(const json &val) {
        if (val.type() != json::value_t::number_float)
            throw std::bad_cast();
        return val.get<double>();
    }

    static bool parse(json &val, std::string str) {
        val = json::parse(str.begin(), str.end());
        return true;
    }

    static std::string serialize(const json &val) {
        return val.dump();
    }
};


#define NUM_ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

class Storage
{
private:
    bool _block_a_read_ok;
    bool _block_b_read_ok;
    bool _write_block_a_first;

    std::string _machine_public_key;
    std::string _license_public_key;
    std::string _offline_public_key;

    std::string _machine_jwt_encoded;
    std::string _license_jwt_encoded;

    jwt::decoded_jwt<nlohmann_traits>* _p_machine_jwt_decoded;
    jwt::decoded_jwt<nlohmann_traits>* _p_license_jwt_decoded;

    std::string _mesa_board;    // 5i25, 7i92, or ECM1
    std::string _mesa_ipaddr;   // Only used for 7i92 or ECM1 boards

    time_t _machinepubkey_file_mtime;
    time_t _licensepubkey_file_mtime;
    time_t _offlinepubkey_file_mtime;
    time_t _machinedat_file_mtime;
    time_t _licensedat_file_mtime;

    std::string MACHINEDAT_FILEPATH;
    std::string LICENSEDAT_FILEPATH;

    std::string MACHINEPUBKEY_FILEPATH;
    std::string LICENSEPUBKEY_FILEPATH;
    // The offline public key is used as a backstop last chance
    // The server signs machine and license jwts using that key pair when a user uploads
    // a license request file and/or downloads a license.  We do this to avoid packaging the public key
    // used to validate the jwt right along side the jwt in the download.
    std::string OFFLINEPUBKEY_FILEPATH;

    void recursive_rmdir(const std::string& dirpath);

    bool create_random_datafile(std::string& path);

    time_t read_timestamp(const std::string& path);

    bool read_entire_file(const std::string& path, std::string& contents, time_t* p_mtime = NULL);

    bool write_file(const std::string& filepath, const std::string data);

    bool mesaflash_exec(const std::string& rwcmd,     // --nvread    or   --nvwrite
                        const std::string& filepath,
                        const std::string& nvblock);

    bool zip_exec(const std::string& contents_dir_path,
                  std::string& zipfile_path);

    bool unzip_exec(const std::string& zipfilepath,
                    const std::string& tmpdirpath);

    // These use the zip container to house the keys inside the flash zip
    bool read_public_keys_from_expanded_zip(const std::string& zipfilepath);
    bool write_public_keys_to_zip(std::string& zipfile_path);

    // These use the files scattered on the SSD drive
    bool read_public_keys_from_files();
    void write_public_keys_to_files();

    void trim_key_bookends(std::string& public_key);
    void restore_key_bookends(std::string& public_key);

    bool read_flash_block(const std::string& block, std::string& zipdir, time_t& timestamp);

    bool verify_machine_jwt();
    bool verify_license_jwt();

public:
    std::string MACHINEJSON_FILEPATH;
    std::string LICENSEJSON_FILEPATH;

    Storage(const std::string& ini_file_path);
    ~Storage();

    bool read_flash();
    bool is_either_flash_block_corrupt();
    bool write_flash();

    bool read_ssd_files();
    void write_ssd_keys_and_machine_jwt();

    bool do_license_ssd_files_exist();
    void delete_license_ssd_file();

    jwt::decoded_jwt<nlohmann_traits>* get_machine_jwt() { return _p_machine_jwt_decoded; }
    jwt::decoded_jwt<nlohmann_traits>* get_license_jwt() { return _p_license_jwt_decoded; }

    const std::string& get_machine_pk() const { return _machine_public_key; }
    const std::string& get_license_pk() const { return _license_public_key; }
};
