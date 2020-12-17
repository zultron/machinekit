#pragma once

#include "storage.hh"

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


enum license_type_t
{
    LICENSE_TYPE_UNMANAGED   = 0,
    LICENSE_TYPE_TIMELIMITED
};

class LicenseMgr
{
private:
    Storage _storage;
    bool _flash_update_needed;
    license_type_t _license_type;

    jwt::decoded_jwt<nlohmann_traits>* _p_machine_jwt_decoded;
    jwt::decoded_jwt<nlohmann_traits>* _p_license_jwt_decoded;

    void update_json_files();
    bool verify_machine_jwt();
    bool verify_license_jwt();
    bool check_all_claims();
    void write_json_claims(const std::string& filepath,
                           const std::unordered_map< typename nlohmann_traits::string_type, jwt::basic_claim<nlohmann_traits>  >& claims);

public:
    LicenseMgr(const std::string& ini_file_path);
    virtual ~LicenseMgr();

    void perform_boot_maintenance();

    // Called by emctaskmain
    void perform_emctaskmain_init();
    // Called by Cycle Start effectively - any time we try to switch into EMC_TASK_MODE_AUTO
    bool is_license_valid();
};
