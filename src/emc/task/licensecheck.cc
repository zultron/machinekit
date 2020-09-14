
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

const char* OPENSSL_BINARY = "/usr/bin/openssl";
const char* LICENSE_FILE = "/home/operator/license.json";
const char* LICENSE_FILE_SIGNATURE = "/home/operator/license.sig";
const char* PUBLIC_KEY = "/home/operator/public.pem";

// use all stdlib stuff to drop in easily to linuxcnc

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
                                if (0 ==WEXITSTATUS(status))
                                {
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
