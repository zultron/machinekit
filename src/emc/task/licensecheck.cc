
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// Header only C++ 11 libraries for JSON Web Tokens (JWT) and JSON
// https://github.com/Thalhammer/jwt-cpp
// https://github.com/nlohmann/json

#include <iostream>

#define DISABLE_PICOJSON // Make sure JWT compiles with this flag
#include "jwt-cpp/include/jwt-cpp/jwt.h"
#include "jwt-cpp/include/nlohmann/json.hpp"

// The jwt-cpp library has a legacy json library named picojson that it includes still for backward compatibility.
// But instead the nlohmann json library is recommended by many so this traits struct lets the jwt-cpp use nlohmann
// through C++ template magic.
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


// use all stdlib stuff to drop in easily to linuxcnc without
// much C++ STL dependencies and vagaries

const char* OPENSSL_BINARY = "/usr/bin/openssl";
const char* LICENSE_FILE = "/home/operator/license.json";
const char* LICENSE_FILE_SIGNATURE = "/home/operator/license.sig";
const char* PUBLIC_KEY = "/home/operator/public.pem";

// 64K is way bigger than any possible license.json file in the real world
const size_t BUFFER_SIZE = (sizeof(char) * 64 * 1024);

#define ZERO_STRUCT(ptr) { memset(ptr, 0, sizeof(*ptr)); }

bool is_license_expired()
{
    bool license_expired = true;  // assume the worst

    char* buffer = (char*) malloc(BUFFER_SIZE);
    if (buffer)
    {
        memset(buffer, 0, BUFFER_SIZE);  // null terminate entire buffer so later strstr can't possibly overflow

        FILE* licfile = fopen(LICENSE_FILE, "r");
        if (licfile)
        {
            size_t bytecount = fread(buffer, 1, BUFFER_SIZE-1, licfile);
            if (bytecount > 0)
            {
                const char* buffer_limit = buffer + bytecount;
                // buffer contains a small json structure and we're hunting for the substring
                // "expiration_timestamp_utc": "2020-09-30 22:31:09"
                char* token = strstr(buffer, "\"expiration_timestamp_utc\":");
                if (token)
                {
                    token += strlen("\"expiration_timestamp_utc\":");
                    while ((*token == ' ') && (token < buffer_limit)) ++token;
                    while ((*token == '"') && (token < buffer_limit)) ++token;
                    char* expire_timestamp_utc = token;
                    while ((*token != '"') && (token < buffer_limit)) ++token;
                    if (token < buffer_limit)
                    {
                        *token = '\0';

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
                    }
                }
            }

            fclose(licfile);
            licfile = NULL;
        }

        free(buffer);
        buffer = NULL;
    }

    return license_expired;
}


bool is_license_signature_valid()
{
    // Don't trust contest of bash scripts, run the openssl binary directly
    pid_t pid = fork();
    if (0 == pid)
    {
        // Child - first convert the base64 license signature into appropriate binary form
        int ev = execl(OPENSSL_BINARY, OPENSSL_BINARY, "base64", "-d", "-in", LICENSE_FILE_SIGNATURE, "-out", "/tmp/sign.sha256", NULL);
        exit(ev);  // Unexpected as we should not have failed
    }
    else
    {
        // Parent - wait for result
        int status = 0;
        if (pid == waitpid(pid, &status, 0))
        {
            if (WIFEXITED(status))
            {
                if (0 == WEXITSTATUS(status))
                {
                    // Now actually validate the binary signature using the public.pem key
                    pid_t pid = fork();
                    if (0 == pid)
                    {
                        // Child
                        int ev = execl(OPENSSL_BINARY, OPENSSL_BINARY, "dgst", "-sha256", "-verify", PUBLIC_KEY, "-signature", "/tmp/sign.sha256", LICENSE_FILE, NULL);
                        exit(ev);  // Unexpected as we should not have failed
                    }
                    else
                    {
                        // Parent - wait for result
                        int status = 0;
                        if (pid == waitpid(pid, &status, 0))
                        {
                            if (WIFEXITED(status))
                            {
                                if (0 == WEXITSTATUS(status))
                                {
                                    // awesome - the signature is valid so we can trust the contents of license.json
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}


void test_jwt_cpp_lib()
{
  std::string rsa_priv_key = R"(-----BEGIN PRIVATE KEY-----
MIIEvwIBADANBgkqhkiG9w0BAQEFAASCBKkwggSlAgEAAoIBAQC4ZtdaIrd1BPIJ
tfnF0TjIK5inQAXZ3XlCrUlJdP+XHwIRxdv1FsN12XyMYO/6ymLmo9ryoQeIrsXB
XYqlET3zfAY+diwCb0HEsVvhisthwMU4gZQu6TYW2s9LnXZB5rVtcBK69hcSlA2k
ZudMZWxZcj0L7KMfO2rIvaHw/qaVOE9j0T257Z8Kp2CLF9MUgX0ObhIsdumFRLaL
DvDUmBPr2zuh/34j2XmWwn1yjN/WvGtdfhXW79Ki1S40HcWnygHgLV8sESFKUxxQ
mKvPUTwDOIwLFL5WtE8Mz7N++kgmDcmWMCHc8kcOIu73Ta/3D4imW7VbKgHZo9+K
3ESFE3RjAgMBAAECggEBAJTEIyjMqUT24G2FKiS1TiHvShBkTlQdoR5xvpZMlYbN
tVWxUmrAGqCQ/TIjYnfpnzCDMLhdwT48Ab6mQJw69MfiXwc1PvwX1e9hRscGul36
ryGPKIVQEBsQG/zc4/L2tZe8ut+qeaK7XuYrPp8bk/X1e9qK5m7j+JpKosNSLgJj
NIbYsBkG2Mlq671irKYj2hVZeaBQmWmZxK4fw0Istz2WfN5nUKUeJhTwpR+JLUg4
ELYYoB7EO0Cej9UBG30hbgu4RyXA+VbptJ+H042K5QJROUbtnLWuuWosZ5ATldwO
u03dIXL0SH0ao5NcWBzxU4F2sBXZRGP2x/jiSLHcqoECgYEA4qD7mXQpu1b8XO8U
6abpKloJCatSAHzjgdR2eRDRx5PMvloipfwqA77pnbjTUFajqWQgOXsDTCjcdQui
wf5XAaWu+TeAVTytLQbSiTsBhrnoqVrr3RoyDQmdnwHT8aCMouOgcC5thP9vQ8Us
rVdjvRRbnJpg3BeSNimH+u9AHgsCgYEA0EzcbOltCWPHRAY7B3Ge/AKBjBQr86Kv
TdpTlxePBDVIlH+BM6oct2gaSZZoHbqPjbq5v7yf0fKVcXE4bSVgqfDJ/sZQu9Lp
PTeV7wkk0OsAMKk7QukEpPno5q6tOTNnFecpUhVLLlqbfqkB2baYYwLJR3IRzboJ
FQbLY93E8gkCgYB+zlC5VlQbbNqcLXJoImqItgQkkuW5PCgYdwcrSov2ve5r/Acz
FNt1aRdSlx4176R3nXyibQA1Vw+ztiUFowiP9WLoM3PtPZwwe4bGHmwGNHPIfwVG
m+exf9XgKKespYbLhc45tuC08DATnXoYK7O1EnUINSFJRS8cezSI5eHcbQKBgQDC
PgqHXZ2aVftqCc1eAaxaIRQhRmY+CgUjumaczRFGwVFveP9I6Gdi+Kca3DE3F9Pq
PKgejo0SwP5vDT+rOGHN14bmGJUMsX9i4MTmZUZ5s8s3lXh3ysfT+GAhTd6nKrIE
kM3Nh6HWFhROptfc6BNusRh1kX/cspDplK5x8EpJ0QKBgQDWFg6S2je0KtbV5PYe
RultUEe2C0jYMDQx+JYxbPmtcopvZQrFEur3WKVuLy5UAy7EBvwMnZwIG7OOohJb
vkSpADK6VPn9lbqq7O8cTedEHttm6otmLt8ZyEl3hZMaL3hbuRj6ysjmoFKx6CrX
rK0/Ikt5ybqUzKCMJZg2VKGTxg==
-----END PRIVATE KEY-----)";
  std::string rsa_pub_key = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuGbXWiK3dQTyCbX5xdE4
yCuYp0AF2d15Qq1JSXT/lx8CEcXb9RbDddl8jGDv+spi5qPa8qEHiK7FwV2KpRE9
83wGPnYsAm9BxLFb4YrLYcDFOIGULuk2FtrPS512Qea1bXASuvYXEpQNpGbnTGVs
WXI9C+yjHztqyL2h8P6mlThPY9E9ue2fCqdgixfTFIF9Dm4SLHbphUS2iw7w1JgT
69s7of9+I9l5lsJ9cozf1rxrXX4V1u/SotUuNB3Fp8oB4C1fLBEhSlMcUJirz1E8
AziMCxS+VrRPDM+zfvpIJg3JljAh3PJHDiLu902v9w+Iplu1WyoB2aPfitxEhRN0
YwIDAQAB
-----END PUBLIC KEY-----)";

  std::string token =
      "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXUyJ9.eyJpc3MiOiJhdXRoMCJ9."
      "VA2i1ui1cnoD6I3wnji1WAVCf29EekysvevGrT2GXqK1dDMc8"
      "HAZCTQxa1Q8NppnpYV-hlqxh-X3Bb0JOePTGzjynpNZoJh2aHZD-"
      "GKpZt7OO1Zp8AFWPZ3p8Cahq8536fD8RiBES9jRsvChZvOqA7gMcFc4"
      "YD0iZhNIcI7a654u5yPYyTlf5kjR97prCf_OXWRn-bYY74zna4p_bP9oWCL4BkaoRcMxi-"
      "IR7kmVcCnvbYqyIrKloXP2qPO442RBGqU7Ov9"
      "sGQxiVqtRHKXZR9RbfvjrErY1KGiCp9M5i2bsUHadZEY44FE2jiOmx-"
      "uc2z5c05CCXqVSpfCjWbh9gQ";

  auto verify = jwt::verify<jwt::default_clock, nlohmann_traits>({})
                    .allow_algorithm(jwt::algorithm::rs256(
                        rsa_pub_key, rsa_priv_key, "", ""))
                    .with_issuer("auth0");

  auto decoded = jwt::decode<nlohmann_traits>(token);

  verify.verify(decoded);

  for (auto &e : decoded.get_header_claims())
    std::cout << e.first << " = " << e.second.to_json() << std::endl;
  for (auto &e : decoded.get_payload_claims())
    std::cout << e.first << " = " << e.second.to_json() << std::endl;
}

bool is_license_valid()
{
    test_jwt_cpp_lib();

    bool valid = is_license_signature_valid();
    if (valid)
    {
        valid = !is_license_expired();
        if (valid)
        {
            // Great, we have an unaltered license file that has a time window that hasn't expired yet.
            // Make sure its actually for this machine...

            // Check serial number
            // Check model
            // Read machine.json for now and trust it.
            // Assume that earlier in the launch the ecm1 flash is read, signature verified, and then unpacked.
            ;
        }
    }

    return valid;
}

#if 0
// Here just for easy testing
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
