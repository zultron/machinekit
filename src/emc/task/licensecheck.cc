
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

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


bool is_license_valid()
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
                                    // awesome - the signature is valid so we can trust the contents of
                                    // license.json
                                    // now need to verify that the current time isn't past the license expiration
                                    // time.
                                    return !is_license_expired();
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
