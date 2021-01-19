
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <iostream>
#include <cstdlib>
#include <fstream>

#include "storage.hh"

using namespace std;


#ifdef DEBUG
#define DEBUG_LOG(x) {cerr << x << endl;}
#else
#define DEBUG_LOG(x)
#endif

Storage::Storage(const std::string& ini_file_path) :
    _block_a_read_ok(false),
    _block_b_read_ok(false),
    _write_block_a_first(true),
    _p_machine_jwt_decoded(NULL),
    _p_license_jwt_decoded(NULL),
    _machinepubkey_file_mtime(0),
    _licensepubkey_file_mtime(0),
    _offlinepubkey_file_mtime(0),
    _machinedat_file_mtime(0),
    _licensedat_file_mtime(0),
    MACHINEDAT_FILEPATH("/boot"),
    LICENSEDAT_FILEPATH("/boot"),
    MACHINEPUBKEY_FILEPATH("/boot"),
    LICENSEPUBKEY_FILEPATH("/boot"),
    OFFLINEPUBKEY_FILEPATH("/boot"),
    MACHINEJSON_FILEPATH(std::getenv("HOME")),
    LICENSEJSON_FILEPATH(std::getenv("HOME"))
{
    MACHINEJSON_FILEPATH.append("/machine.json");
    LICENSEJSON_FILEPATH.append("/license.json");

    // Breaking things up and composing the string constants below
    // makes it harder to discern where the file are if someone
    // uses the 'strings' app to hunt through the static data segment
    // of the binary.  If we put /boot/home/.d/.m as one complete
    // string, it sticks out like a flashing light.

    std::string homedir("/home");
    std::string hiddendir("/.d");

    MACHINEDAT_FILEPATH.append(homedir);
    LICENSEDAT_FILEPATH.append(homedir);
    MACHINEPUBKEY_FILEPATH.append(homedir);
    LICENSEPUBKEY_FILEPATH.append(homedir);
    OFFLINEPUBKEY_FILEPATH.append(homedir);

    MACHINEDAT_FILEPATH.append(hiddendir);
    LICENSEDAT_FILEPATH.append(hiddendir);
    MACHINEPUBKEY_FILEPATH.append(hiddendir);
    LICENSEPUBKEY_FILEPATH.append(hiddendir);
    OFFLINEPUBKEY_FILEPATH.append(hiddendir);

    MACHINEDAT_FILEPATH.append("/.m");
    LICENSEDAT_FILEPATH.append("/.l");
    MACHINEPUBKEY_FILEPATH.append("/.0");
    LICENSEPUBKEY_FILEPATH.append("/.1");
    OFFLINEPUBKEY_FILEPATH.append("/.2");

    // Harden so that if /boot/home/.d is missing, it will be created
    // automatically with proper permissions
    // But be careful not to have a string constant with the full path
    // all in one shot.
    std::string tmpdir("/boot");
    tmpdir.append(homedir);
    struct stat statbuf;
    if (0 != stat(tmpdir.c_str(), &statbuf))
    {
        mkdir(tmpdir.c_str(), 0777);
    }
    tmpdir.append(hiddendir);
    if (0 != stat(tmpdir.c_str(), &statbuf))
    {
        mkdir(tmpdir.c_str(), 0700);
    }

    IniFile inifile;
    if (inifile.Open(ini_file_path.c_str()))
    {
        const char* board = inifile.Find("BOARD", "HOSTMOT2");
        if (NULL != board)
        {
            _mesa_board.assign(board);

            if (_mesa_board != "5i25")
            {
                const char* params = inifile.Find("DRIVER_PARAMS", "HOSTMOT2");
                // Pull the board_ip=n.n.n.n string which has to be first without any spaces
                const char* addr = strstr(params, "board_ip=");
                if (NULL != addr)
                {
                    addr += strlen("board_ip=");
                    const char* addrend = strstr(addr, " ");
                    if (NULL != addrend)
                    {
                        _mesa_ipaddr.assign(addr, addrend);
                    }
                }
            }
        }

        inifile.Close();
    }
}


Storage::~Storage()
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


void Storage::recursive_rmdir(const std::string& dirpath)
{
    DIR* dir = opendir(dirpath.c_str());
    if (NULL != dir)
    {
        for (struct dirent* dirent = readdir(dir);
             dirent != NULL;
             dirent = readdir(dir))
        {
            if ((0 != strcmp(dirent->d_name, ".")) &&
                (0 != strcmp(dirent->d_name, "..")))
            {
                std::string fullpath(dirpath);
                fullpath.append("/");
                fullpath.append(dirent->d_name);
                if (DT_REG == dirent->d_type)
                {
                    unlink(fullpath.c_str());
                }
                else if (DT_DIR == dirent->d_type)
                {
                    // recurse to continue deleting...
                    recursive_rmdir(fullpath);
                }
            }
        }

        closedir(dir);
        dir = NULL;
    }

    // Now we know the dir is empty so rmdir will work reliably
    rmdir(dirpath.c_str());
}


time_t Storage::read_timestamp(const std::string& path)
{
    time_t timestamp = 0;
    int fd = open(path.c_str(), O_RDONLY);
    if (-1 != fd)
    {
        if (read(fd, &timestamp, sizeof(timestamp)) != sizeof(timestamp))
        {
            timestamp = 0;  // recover from partial read and also to shutup compiler warning
        }
        close(fd);
        fd = -1;
    }
    return timestamp;
}


bool Storage::read_entire_file(const std::string& path,
                               std::string& contents,
                               time_t* p_mtime)
{
    // Does caller care about last mod time?
    if (p_mtime)
    {
        struct stat statbuf;
        if (0 == stat(path.c_str(), &statbuf))
        {
            if (statbuf.st_mtime == *p_mtime)
            {
                // File has same last mod time so no need to do anymore
                return true;
            }
        }
    }

    bool result = false;
    int fd = open(path.c_str(), O_RDONLY);
    if (fd >= 0)
    {
        struct stat sb;
        if (0 == fstat(fd, &sb))
        {
            if (p_mtime)  // update caller mtime if desired
            {
                *p_mtime = sb.st_mtime;
            }

            char* buffer = new char[sb.st_size + 1];
            if (sb.st_size == read(fd, buffer, sb.st_size))
            {
                buffer[sb.st_size] = '\0';
                contents.assign(buffer);
                result = true;
            }
            delete[] buffer;
            buffer = NULL;
        }

        close(fd);
        fd = -1;
    }

    return result;
}


bool Storage::create_random_datafile(std::string& path)
{
    bool result = false;
    srand(time(NULL));
    ssize_t cb = 1024*64;
    ssize_t elements = cb / sizeof(int);
    int* buffer = new int[elements];   // 64K exactly
    for (int ix = 0; ix < elements; ++ix)
    {
        buffer[ix] = rand();
    }

    char* tmpfilename = new char[path.capacity() + 10];
    strcpy(tmpfilename, path.c_str());
    strcat(tmpfilename, "/XXXXXX");
    int fd = mkstemp(tmpfilename);
    if (-1 != fd)
    {
        if (cb == write(fd, buffer, cb))
        {
            path.assign(tmpfilename);
            result = true;
        }
        close(fd);
        fd = -1;
    }

    delete[] buffer;
    buffer = NULL;
    return result;
}


bool Storage::mesaflash_exec(const std::string& rwcmd,     // --nvread    or   --nvwrite
                             const std::string& filepath,
                             const std::string& nvblock)   //  A    or    B
{
    bool result = false;

    // mesaflash binary lives in same directory as *this* process.  so figure out
    // where that is and slice off our executable filename and replace it with mesaflash
    pid_t pid = getpid();
    char path[100];
    char mesapath[512];
    sprintf(path, "/proc/%d/exe", pid);
    ssize_t cb = readlink(path, mesapath, sizeof(mesapath));
    if (cb > 0)
    {
        mesapath[cb] = '\0';  // Add null termination
        char* slash = strrchr(mesapath, '/');
        if (slash)
        {
            ++slash;
            strcpy(slash, "mesaflash");

            pid_t pid = fork();
            if (pid == 0)
            {
                // Child
                // We don't want the output of the exec program to be visible in the
                // normal inherited stdout and stderr logs as it reveals a lot of detail.

                #ifndef DEBUG
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                #endif

                int ev;
                if (_mesa_board == "5i25")
                {
                    ev = execlp("/usr/bin/sudo",
                                "/usr/bin/sudo",
                                mesapath,
                                "--device", _mesa_board.c_str(),
                                rwcmd.c_str(), filepath.c_str(),
                                "--nvblock", nvblock.c_str(),
                                NULL);
                }
                else
                {
                    // xxxx fix path - sudo needs a full path for mesaflash
                    ev = execlp("/usr/bin/sudo",
                                "/usr/bin/sudo",
                                mesapath,
                                "--device", _mesa_board.c_str(),
                                "--addr", _mesa_ipaddr.c_str(),
                                rwcmd.c_str(), filepath.c_str(),
                                "--nvblock", nvblock.c_str(),
                                NULL);
                }
                exit(ev);
            }
            else
            {
                // Parent - wait for result
                int status = 0;
                if (pid == waitpid(pid, &status, 0))
                {
                    if ((WIFEXITED(status)) && (0 == WEXITSTATUS(status)))
                    {
                        result = true;
                    }
                }
            }
        }
    }

    return result;
}


bool Storage::zip_exec(const std::string& contents_dir_path,
                            std::string& zipfile_path)
{
    bool result = false;

    // if caller has a path in zipfile_path already, use it.
    // otherwise we zip into a tmp file - and we have to just pick a
    // name because zip is braindead.  if we create an empty file to
    // reserve the name in the /tmp, there is no overwrite flag to zip.
    // it always thinks its an existing zip its adding to so the file cannot
    // exist beforehand.
    if (zipfile_path.length() == 0)
    {
        zipfile_path.assign("/tmp/jeijf23FFwei434.0");
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child
        // zip is rather dumb in giving control over how much of the dir path is stored in the zip
        // so must chdir first to control this.
        int ev = 12;
        if (0 == chdir(contents_dir_path.c_str()))
        {
            // We don't want the output of the exec program to be visible in the
            // normal inherited stdout and stderr logs as it reveals a lot of detail.
            #ifndef DEBUG
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            #endif

            unlink(zipfile_path.c_str());
            ev = execlp("zip",
                        "zip",
                        "-r",
                        zipfile_path.c_str(),
                        ".",
                        NULL);
        }
        exit(ev);
    }
    else
    {
        // Parent - wait for result
        int status = 0;
        if (pid == waitpid(pid, &status, 0))
        {
            if ((WIFEXITED(status)) && (0 == WEXITSTATUS(status)))
            {
                result = true;
            }
        }
    }

    return result;
}


bool Storage::unzip_exec(const std::string& zipfilepath,
                              const std::string& tmpdirpath)
{
    bool result = false;
    pid_t pid = fork();
    if (pid == 0)
    {
        // We don't want the output of the exec program to be visible in the
        // normal inherited stdout and stderr logs as it reveals a lot of detail.
        #ifndef DEBUG
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        #endif

        // Child
        int ev = execlp("unzip",
                        "unzip",
                        "-qq",
                        zipfilepath.c_str(),
                        "-d",
                        tmpdirpath.c_str(),
                        NULL);
        exit(ev);
    }
    else
    {
        // Parent - wait for result
        int status = 0;
        if (pid == waitpid(pid, &status, 0))
        {
            if ((WIFEXITED(status)) && (0 == WEXITSTATUS(status)))
            {
                result = true;
            }
        }
    }

    return result;
}


bool Storage::read_flash_block(
    const std::string& block,
    std::string& zipdir,
    time_t& timestamp)
{
    bool result = false;
    char filepath_raw[] = { "/tmp/XXXXXX\0" };
    char filepath_zip[] = { "/tmp/XXXXXX\0" };
    int fd = mkstemp(filepath_raw);
    if (-1 != fd)
    {
        close(fd);
        fd = -1;
        std::string tmppath(filepath_raw);
        result = mesaflash_exec("--nvread", tmppath, block);
        if (result)
        {
            fd = open(filepath_raw, O_RDONLY);
            if (-1 != fd)
            {
                // copy the rest of the file to a new tmp file
                int zipfd = mkstemp(filepath_zip);
                if (-1 != zipfd)
                {
                    char* buffer = new char[64*1024];
                    ssize_t cb = read(fd, buffer, 64*1024);
                    if (cb == (64*1024))
                    {
                        // Now we reverse the entire 64K block
                        char* final_buffer = new char[64*1024];
                        for (int ix = 0; ix < (64*1024); ++ix)
                        {
                            final_buffer[ix] = buffer[64*1024 - ix - 1];
                        }
                        ssize_t zipcb = write(zipfd, final_buffer, cb);
                        if (cb == zipcb)
                        {
                            // Perfect.
                            result = true;
                        }
                        delete[] final_buffer;
                        final_buffer = NULL;
                    }

                    delete[] buffer;
                    buffer = NULL;
                    close(zipfd);
                    zipfd = -1;

                    if (!result)
                    {
                        // something botched above - don't need this around anymore so wipe it.
                        unlink(filepath_zip);
                    }
                }

                close(fd);
                fd = -1;
            }
        }

        // regardless how above worked, we don't need filepath_raw anymore so wipe it.
        unlink(filepath_raw);
    }

    if (result)
    {
        // We now have filepath_zip containing what should be valid zip data.
        // Use zip to find out and read the timestamp file inside it
        result = false;  // assume the rest goes badly again.

        char zipdir_path[] = { "/tmp/XXXXXX\0" };
        if (NULL != mkdtemp(zipdir_path))
        {
            // unzip filepath_zip -d dirpath
            std::string tmp1(filepath_zip);
            std::string tmp2(zipdir_path);
            result = unzip_exec(tmp1, tmp2);
            if (result)
            {
                zipdir.assign(zipdir_path);
                timestamp = read_timestamp(tmp2.append("/timestamp"));
                result = (0 != timestamp);
            }

            // caller is responsible for cleaning up zipdir_path upon success
            if (!result)
            {
                recursive_rmdir(zipdir_path);
            }
        }

        // don't need this around anymore so wipe it.
        unlink(filepath_zip);
    }

    return result;
}


bool Storage::read_flash()
{
    DEBUG_LOG("                       reading flash blocks");

    struct blockinfo
    {
        std::string zipdir_path;
        time_t timestamp;
    };

    struct blockinfo blocka;
    struct blockinfo blockb;
    struct blockinfo* finalblock = NULL;
    _block_a_read_ok = read_flash_block("A", blocka.zipdir_path, blocka.timestamp);
    _block_b_read_ok = read_flash_block("B", blockb.zipdir_path, blockb.timestamp);
    if ((_block_a_read_ok) && (_block_b_read_ok))
    {
        if (blocka.timestamp >= blockb.timestamp)
        {
            // Use A
            finalblock = &blocka;
            recursive_rmdir(blockb.zipdir_path);  // Cleanup B
            // Upon write, do B first since its older
            _write_block_a_first = false;
        }
        else
        {
            // Use B
            finalblock = &blockb;
            recursive_rmdir(blocka.zipdir_path);  // Cleanup A
            _write_block_a_first = true;
        }
    }
    else if (_block_a_read_ok)
    {
        finalblock = &blocka;
        // Write B first since its corrupt
        _write_block_a_first = false;
    }
    else if (_block_b_read_ok)
    {
        finalblock = &blockb;
        // Write A first since its corrupt
        _write_block_a_first = true;
    }

    if (NULL != finalblock)
    {
        // read the machine jwt which is named machine.dat
        std::string tmp(finalblock->zipdir_path);
        tmp.append("/machine.dat");
        bool result = read_entire_file(tmp, _machine_jwt_encoded);
        if (result)
        {
            // read the public key nested zip which is named compensation.dat as a misdirection
            tmp.assign(finalblock->zipdir_path);
            tmp.append("/compensation.dat");
            result = read_public_keys_from_expanded_zip(tmp);
            if (result)
            {
                result = verify_machine_jwt();
            }
        }

        recursive_rmdir(finalblock->zipdir_path);

        return result;
    }

    return false;
}


bool Storage::is_either_flash_block_corrupt()
{
    return !(_block_a_read_ok && _block_b_read_ok);
}


static const char g_header[] = {"-----BEGIN PUBLIC KEY-----\n"};
static const char g_footer[] = {"-----END PUBLIC KEY-----\n"};

void Storage::trim_key_bookends(std::string& public_key)
{
    size_t pos = public_key.find(g_header);
    if (string::npos != pos)
    {
        public_key.erase(pos, strlen(g_header));
    }
    pos = public_key.find(g_footer);
    if (string::npos != pos)
    {
        public_key.erase(pos, strlen(g_footer));
    }
}

void Storage::restore_key_bookends(std::string& public_key)
{
    if (public_key.find(g_header) == string::npos)
    {
        public_key.insert(0, g_header);
    }
    if (public_key.find(g_footer) == string::npos)
    {
        public_key.append(g_footer);
    }
}


bool Storage::read_public_keys_from_files()
{
    bool result = read_entire_file(MACHINEPUBKEY_FILEPATH, _machine_public_key, &_machinepubkey_file_mtime);
    if (result)
    {
        restore_key_bookends(_machine_public_key);

        // The offline public key may or may not be present.  Any controller that wasn't "born" a licensed
        // version of software won't have it.
        result = read_entire_file(OFFLINEPUBKEY_FILEPATH, _offline_public_key, &_offlinepubkey_file_mtime);
        if (result)
        {
            restore_key_bookends(_offline_public_key);
        }

        // The license public key may or may not be present
        result = read_entire_file(LICENSEPUBKEY_FILEPATH, _license_public_key, &_licensepubkey_file_mtime);
        if (result)
        {
            restore_key_bookends(_license_public_key);
        }

        result = true;  // ignore failures if the license public key was missing
    }
    return result;
}


void Storage::write_public_keys_to_files()
{
    fstream keyfile;
    keyfile.open(MACHINEPUBKEY_FILEPATH, ios_base::out | ios_base::trunc);
    std::string tmpkey(_machine_public_key);
    trim_key_bookends(tmpkey);
    keyfile << tmpkey;
    keyfile.close();

    if (_offline_public_key.length() > 0)
    {
        keyfile.open(OFFLINEPUBKEY_FILEPATH, ios_base::out | ios_base::trunc);
        tmpkey.assign(_offline_public_key);
        trim_key_bookends(tmpkey);
        keyfile << tmpkey;
        keyfile.close();
    }

    if (_license_public_key.length() > 0)
    {
        keyfile.open(LICENSEPUBKEY_FILEPATH, ios_base::out | ios_base::trunc);
        tmpkey.assign(_license_public_key);
        trim_key_bookends(tmpkey);
        keyfile << tmpkey;
        keyfile.close();
    }
}


bool Storage::read_public_keys_from_expanded_zip(const std::string& zipfilepath)
{
    bool result = false;
    char zipdir_path[] = { "/tmp/XXXXXX\0" };
    if (NULL != mkdtemp(zipdir_path))
    {
         // unzip zipfilepath into /tmp/XXXXX temp directory
        std::string zipdir_path_str(zipdir_path);
        result = unzip_exec(zipfilepath, zipdir_path_str);
        if (result)
        {
            // The machine key file should exist.
            std::string keyfilepath(zipdir_path_str);
            keyfilepath.append("/0");
            result = read_entire_file(keyfilepath, _machine_public_key);
            if (result)
            {
                restore_key_bookends(_machine_public_key);

                // The offline license key file may or may not exist.  Only controllers "born"
                // with a licensed version of PP have it.
                keyfilepath.assign(zipdir_path_str);
                keyfilepath.append("/2");
                result = read_entire_file(keyfilepath, _offline_public_key);
                if (result)
                {
                    restore_key_bookends(_offline_public_key);
                }

                // The license key file may or may not.  Out of box time limited machines
                // won't have one until a successful license is acquired and written to flash.
                keyfilepath.assign(zipdir_path_str);
                keyfilepath.append("/1");
                result = read_entire_file(keyfilepath, _license_public_key);
                if (result)
                {
                    restore_key_bookends(_license_public_key);
                }

                // Overall we succeed even if license public key is missing.
                result = true;
            }
        }

        // Be sure to cleanup
        recursive_rmdir(zipdir_path_str);
    }

    return result;
}


bool Storage::write_file(const std::string& filepath, const std::string data)
{
    bool result = false;
    int fd = open(filepath.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (-1 != fd)
    {
        ssize_t cb = write(fd, data.c_str(), data.length());
        result = (cb == static_cast<ssize_t>(data.length()));
        close(fd);
        fd = -1;
    }
    return result;
}


bool Storage::write_public_keys_to_zip(std::string& zipfile_path)
{
    bool result = false;
    char zipdir_path[] = { "/tmp/XXXXXX\0" };
    if (NULL != mkdtemp(zipdir_path))
    {
        std::string zipdir_path_str(zipdir_path);

        // machine public key should always exist
        std::string machine_public_key_trimmed(_machine_public_key);
        trim_key_bookends(machine_public_key_trimmed);
        std::string keyfilepath(zipdir_path_str);
        keyfilepath.append("/0");
        result = write_file(keyfilepath, machine_public_key_trimmed);
        if (result)
        {
            // offline public key will only exist if controller was "born" as a time limited licensed
            // machine.
            if (_offline_public_key.length() > 0)
            {
                std::string offline_public_key_trimmed(_offline_public_key);
                trim_key_bookends(offline_public_key_trimmed);
                keyfilepath.assign(zipdir_path_str);
                keyfilepath.append("/2");
                result = write_file(keyfilepath, offline_public_key_trimmed);
            }

            // license public key will only be present if we've been able to acquire
            // one from hub
            if ((result) && (_license_public_key.length() > 0))
            {
                std::string license_public_key_trimmed(_license_public_key);
                trim_key_bookends(license_public_key_trimmed);
                keyfilepath.assign(zipdir_path_str);
                keyfilepath.append("/1");
                result = write_file(keyfilepath, license_public_key_trimmed);
            }

            if (result)
            {
                result = zip_exec(zipdir_path_str, zipfile_path);
            }
        }

        // Be sure to cleanup
        recursive_rmdir(zipdir_path_str);
    }

    return result;
}

bool Storage::write_flash()
{
    bool result = false;

    char contents_path_raw[] = { "/tmp/XXXXXX\0" };
    if (NULL != mkdtemp(contents_path_raw))
    {
        std::string contents_path(contents_path_raw);

        // write encoded machine jwt as machine.dat
        std::string tmpfilepath(contents_path);
        tmpfilepath.append("/machine.dat");
        result = write_file(tmpfilepath, _machine_jwt_encoded);
        if (result)
        {
            // write public keys as a zip into the dir as compensation.dat
            tmpfilepath.assign(contents_path);
            tmpfilepath.append("/compensation.dat");
            result = write_public_keys_to_zip(tmpfilepath);
            if (result)
            {
                // create or update the /timestamp file that will be inside the zip.
                time_t utcnow = time(NULL);
                std::string timestamp_path(contents_path);
                timestamp_path.append("/timestamp");
                int fd = open(timestamp_path.c_str(), O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
                if (-1 != fd)
                {
                    if (sizeof(utcnow) == write(fd, &utcnow, sizeof(utcnow)))
                    {
                        result = true;
                    }
                    close(fd);
                    fd = -1;
                }

                if (result)
                {
                    // zip up the zipdir_path directory contents
                    std::string zipfile_path;
                    result = zip_exec(contents_path, zipfile_path);
                    if (result)
                    {
                        std::string blockfile_path("/tmp");
                        result = create_random_datafile(blockfile_path);
                        if (result)
                        {
                            result = false;
                            // now we can overlay the zip file into the random data file
                            // backwards starting at the end
                            fd = open(blockfile_path.c_str(), O_WRONLY);
                            if (-1 != fd)
                            {
                                int zipfd = open(zipfile_path.c_str(), O_RDONLY);
                                if (-1 != zipfd)
                                {
                                    char* buffer = new char[64*1024];
                                    ssize_t cb = read(zipfd, buffer, 64*1024);
                                    if (-1 != cb)
                                    {
                                        char* final_buffer = new char[cb];
                                        for (int ix = 0; ix < cb; ++ix)
                                        {
                                            final_buffer[ix] = buffer[cb-ix-1];
                                        }
                                        if (-1 != lseek(fd, 64*1024 - cb, SEEK_SET))
                                        {
                                            // Need to write cb bytes
                                            if (cb == write(fd, final_buffer, cb))
                                            {
                                                result = true;
                                            }
                                        }
                                        delete[] final_buffer;
                                        final_buffer = NULL;
                                    }
                                    delete[] buffer;
                                    buffer = NULL;
                                    close(zipfd);
                                    zipfd = -1;
		                        }
                                close(fd);
                                fd = -1;
                            }

                            // Be careful the order we write back if one of the blocks did not
                            // read successfully - or one of the blocks timestamp files is older.
                            if (_write_block_a_first)
                            {
                                result = mesaflash_exec("--nvwrite", blockfile_path, "A");
                                if (result)
                                {
                                    result = mesaflash_exec("--nvwrite", blockfile_path, "B");
                                }
                            }
                            else
                            {
                                result = mesaflash_exec("--nvwrite", blockfile_path, "B");
                                if (result)
                                {
                                    result = mesaflash_exec("--nvwrite", blockfile_path, "A");
                                }
                            }

                            unlink(blockfile_path.c_str());
                        }

                        unlink(zipfile_path.c_str());
                    }
                }
            }
        }

        // Be sure to cleanup
        recursive_rmdir(contents_path);
    }

    return result;
}


bool Storage::read_ssd_files()
{
    DEBUG_LOG("                       reading ssd files");

    // out-of-box context for time limited machines is machine jwt and public key exist.
    // but license jwt and public key do not.
    bool result = read_public_keys_from_files();
    if (result)
    {
        // Now read the encoded machine token
        result = false;
        read_entire_file(MACHINEDAT_FILEPATH, _machine_jwt_encoded, &_machinedat_file_mtime);
        if (_machine_jwt_encoded.length() > 0)
        {
            result = verify_machine_jwt();
        }

        if (result)
        {
            // The license files may not exist yet.
            // out-of-box context for time limited machines is machine jwt and public key exist.
            // but license jwt and public key do not.
            if (0 == access(LICENSEDAT_FILEPATH.c_str(), F_OK))
            {
                result = false;
                read_entire_file(LICENSEDAT_FILEPATH, _license_jwt_encoded, &_licensedat_file_mtime);
                if (_license_jwt_encoded.length() > 0)
                {
                    result = verify_license_jwt();
                }
            }
        }
    }

    return result;
}


void Storage::write_ssd_keys_and_machine_jwt()
{
    write_public_keys_to_files();

    fstream mdat;
    mdat.open(MACHINEDAT_FILEPATH, ios_base::out | ios_base::trunc);
    mdat << _machine_jwt_encoded;
    mdat.close();
}


bool Storage::verify_machine_jwt()
{
    bool result = false;
    try
    {
        if (_p_machine_jwt_decoded)
        {
            delete _p_machine_jwt_decoded;
        }
        _p_machine_jwt_decoded = new jwt::decoded_jwt<nlohmann_traits>(_machine_jwt_encoded);

        // We try to use the machine public key first, but then use the offline public key
        // as a backstop.
        const std::string* public_keys[] = { &_machine_public_key, &_offline_public_key };
        for (unsigned int ix = 0; ix < NUM_ELEMENTS(public_keys) && !result; ++ix)
        {
            // Verify the jwt signature with the public key
            if (public_keys[ix]->length() > 0)
            {
                try
                {
                    auto verify = jwt::verify<jwt::default_clock, nlohmann_traits>({})
                                    .allow_algorithm(jwt::algorithm::rs256(*(public_keys[ix]), "", "", ""));
                    try
                    {
                        verify.verify(*_p_machine_jwt_decoded);
#ifdef DEBUG
                        cerr << "            machine jwt verified with key index " << ix << endl;
#endif
                        result = true;
                    }
                    catch (jwt::error::signature_verification_exception ex)
                    {
                        // signature verify failed
                        ;
                    }
                    catch (jwt::error::token_verification_exception ex)
                    {
                        // typically the issued at time is in the future so user is performing clock attacks
                        if (ex.code() == jwt::error::token_verification_error::token_expired)
                        {
                            // enforcement of token issued at time is performed later within licensemgr.cc
                            result = true;
                        }
                    }
                }
                catch (jwt::error::rsa_exception ex)
                {
                }
            }
        }

        if (! result)
        {
            delete _p_machine_jwt_decoded;
            _p_machine_jwt_decoded = NULL;
        }
    }
    catch (invalid_argument ex)
    {
        // Thrown when machine encoded string isn't valid
    }

#ifdef DEBUG
    if (result)
    {
        DEBUG_LOG("                       machine jwt verified");
    }
    else
    {
        DEBUG_LOG("                       machine jwt verify FAILED");
    }
#endif

    return result;
}


bool Storage::verify_license_jwt()
{
    bool result = false;
    try
    {
        if (_p_license_jwt_decoded)
        {
            delete _p_license_jwt_decoded;
        }
        _p_license_jwt_decoded = new jwt::decoded_jwt<nlohmann_traits>(_license_jwt_encoded);

        // We try to use the license public key first, but then use the offline public key
        // as a backstop.
        const std::string* public_keys[] = { &_license_public_key, &_offline_public_key };
        for (unsigned int ix = 0; ix < NUM_ELEMENTS(public_keys) && !result; ++ix)
        {
            // Verify the jwt signature with the public key
            if (public_keys[ix]->length() > 0)
            {
                try
                {
                    auto verify = jwt::verify<jwt::default_clock, nlohmann_traits>({})
                                    .allow_algorithm(jwt::algorithm::rs256(*(public_keys[ix]), "", "", ""));
                    try
                    {
                        verify.verify(*_p_license_jwt_decoded);
#ifdef DEBUG
                        cerr << "            license jwt verified with key index " << ix << endl;
#endif
                        result = true;
                    }
                    catch (jwt::error::signature_verification_exception ex)
                    {
                        // signature verify failed
                    }
                    catch (jwt::error::token_verification_exception ex)
                    {
                        // typically the expiration time has passed or the issued at time is in the future
                        // so user is performing clock attacks
                        if (ex.code() == jwt::error::token_verification_error::token_expired)
                        {
                            // enforcement of token times is performed later within licensemgr.cc
                            result = true;
                        }
                    }
                }
                catch (jwt::error::rsa_exception ex)
                {
                }
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

#ifdef DEBUG
    if (result)
    {
        DEBUG_LOG("                       license jwt verified");
    }
    else
    {
        DEBUG_LOG("                       license jwt verify FAILED");
    }
#endif

    return result;
}


bool Storage::do_license_ssd_files_exist()
{
    bool result = false;
    result |= (0 == access(MACHINEDAT_FILEPATH.c_str(), F_OK));
    result |= (0 == access(LICENSEDAT_FILEPATH.c_str(), F_OK));
    result |= (0 == access(MACHINEPUBKEY_FILEPATH.c_str(), F_OK));
    result |= (0 == access(LICENSEPUBKEY_FILEPATH.c_str(), F_OK));
    result |= (0 == access(OFFLINEPUBKEY_FILEPATH.c_str(), F_OK));
    result |= (0 == access(LICENSEJSON_FILEPATH.c_str(), F_OK));
    return result;
}


#if 0
// Compile with  g++ -std=c++11 -g flashstorage.cc
// For standalone testing with cdt debugger

int main()
{
    Storage fs("5i25", "");

    bool result = fs.write_flash("/tmp/nvflash");
    if (result)
    {
        std::cout << "Success!" << std::endl;

        result = fs.read_flash();
    }

    return 0;
}

#endif
