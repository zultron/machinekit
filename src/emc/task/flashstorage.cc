
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <string>
#include <iostream>

#include "flashstorage.hh"


#if 0
FlashStorage::FlashStorage(IniFile& inifile)
{
    const char* board = inifile.Find("HOSTMOT2", "BOARD");
    if (NULL != board)
    {
        this->_mesa_board.assign(board);

        if (this->_mesa_board != "5i25")
        {
            const char* params = inifile.Find("HOSTMOT2", "DRIVER_PARAMS");
            // Pull the board_ip=n.n.n.n string which has to be first without any spaces
            const char* addr = strstr(params, "board_ip=");
            if (NULL != addr)
            {
                addr += strlen("board_ip=");
                const char* addrend = strstr(addr, " ");
                if (NULL != addrend)
                {
                    this->_mesa_ipaddr.assign(addr, addrend-1);
                }
            }
        }
    }
}
#endif

void FlashStorage::recursive_rmdir(const std::string& dirpath)
{
    DIR* dir = opendir(dirpath.c_str());
    if (NULL != dir)
    {
        for (struct dirent* dirent = readdir(dir);
             dirent != NULL;
             dirent = readdir(dir))
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
                this->recursive_rmdir(fullpath);
            }
        }

        closedir(dir);
        dir = NULL;
    }

    // Now we know the dir is empty so rmdir will work reliably
    rmdir(dirpath.c_str());
}


time_t FlashStorage::read_timestamp(const std::string& path)
{
    time_t timestamp = 0;
    int fd = open(path.c_str(), O_RDONLY);
    if (-1 != fd)
    {
        read(fd, &timestamp, sizeof(timestamp));
        close(fd);
        fd = -1;
    }
    return timestamp;
}


std::string FlashStorage::read_entire_file(const std::string& path)
{
    std::string res;
    int fd = open(path.c_str(), O_RDONLY);
    if (fd >= 0)
    {
        struct stat sb;
        if (0 == fstat(fd, &sb))
        {
            res.resize(sb.st_size);
            read(fd, const_cast<char*>(res.data()), sb.st_size);
        }

        close(fd);
        fd = -1;
    }

    return res;
}


bool FlashStorage::create_random_datafile(std::string& path)
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


bool FlashStorage::mesaflash_exec(const std::string& rwcmd,     // --nvread    or   --nvwrite
                                  const std::string& filepath,
                                  const std::string& nvblock)   //  A    or    B
{
    bool result = false;
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child
        int ev;
        if (this->_mesa_board == "5i25")
        {
            // xxxx fis path
            ev = execlp("/usr/bin/sudo",
                        "/usr/bin/sudo",
                        "/tmp/mesaflash",
                        "--device", this->_mesa_board.c_str(),
                        rwcmd.c_str(), filepath.c_str(),
                        "--nvblock", nvblock.c_str(),
                        NULL);
        }
        else
        {
            // xxxxx fix path
            ev = execlp("/usr/bin/sudo",
                        "/usr/bin/sudo",
                        "/tmp/mesaflash",
                        "--device", this->_mesa_board.c_str(),
                        "--addr", this->_mesa_ipaddr.c_str(),
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

    return result;
}


bool FlashStorage::zip_exec(const std::string& contents_dir_path,
                            std::string& zipfile_path)
{
    bool result = false;

    char zipfile_path_raw[] = { "/tmp/jeijf23FFwei434.0\0" };
    unlink(zipfile_path_raw);

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child
        // setuid(0);   // effectively do a sudo

        // zip is rather dumb in giving control over how much of the dir path is stored in the zip
        // so must chdir first to control this.
        chdir(contents_dir_path.c_str());
        int ev = execlp("zip",
                        "zip",
                        "-r",
                        zipfile_path_raw,
                        ".",
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
                zipfile_path.assign(zipfile_path_raw);
                result = true;
            }
        }
    }

    return result;
}


bool FlashStorage::unzip_exec(const std::string& zipfilepath,
                              const std::string& tmpdirpath)
{
    bool result = false;
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child
        // setuid(0);   // effectively do a sudo
        int ev = execlp("unzip",
                        "unzip",
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


bool FlashStorage::read_flash_block(
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
        result = this->mesaflash_exec("--nvread", tmppath, block);
        if (result)
        {
            // toss the first 31 bytes of the block as garbage to be ignored.
            fd = open(filepath_raw, O_RDONLY);
            if (-1 != fd)
            {
                if (-1 != lseek(fd, 31, SEEK_SET))
                {
                    // copy the rest of the file to a new tmp file
                    int zipfd = mkstemp(filepath_zip);
                    if (-1 != zipfd)
                    {
                        char* buffer = new char[64*1024];
                        ssize_t cb = read(fd, buffer, 64*1024);
                        if (cb == (64*1024-31))
                        {
                            ssize_t zipcb = write(zipfd, buffer, cb);
                            if (cb == zipcb)
                            {
                                // Perfect.
                                result = true;
                            }
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
            result = this->unzip_exec(tmp1, tmp2);
            if (result)
            {
                zipdir.assign(zipdir_path);
                timestamp = this->read_timestamp(tmp2.append("/timestamp"));
                result = (0 != timestamp);
            }

            // caller is responsible for cleaning up zipdir_path upon success
            if (!result)
            {
                this->recursive_rmdir(zipdir_path);
            }
        }

        // don't need this around anymore so wipe it.
        unlink(filepath_zip);
    }

    return result;
}


bool FlashStorage::read_flash()
{
    struct blockinfo
    {
        std::string zipdir_path;
        time_t timestamp;
    };

    struct blockinfo blocka;
    struct blockinfo blockb;
    struct blockinfo* finalblock = NULL;
    bool result_a = this->read_flash_block("A", blocka.zipdir_path, blocka.timestamp);
    bool result_b = this->read_flash_block("B", blockb.zipdir_path, blockb.timestamp);
    if ((result_a) && (result_b))
    {
        if (blocka.timestamp >= blockb.timestamp)
        {
            // Use A
            finalblock = &blocka;
            this->recursive_rmdir(blockb.zipdir_path);  // Cleanup B
        }
        else
        {
            // Use B
            finalblock = &blockb;
            this->recursive_rmdir(blocka.zipdir_path);  // Cleanup A
        }
    }
    else if (result_a)
    {
        finalblock = &blocka;
    }
    else if (result_b)
    {
        finalblock = &blockb;
    }

    if (NULL != finalblock)
    {
        // xxxx
        //read machine jwt file
        //read public keys file

        this->recursive_rmdir(finalblock->zipdir_path);

        return true;
    }

    return false;
}


bool FlashStorage::write_flash(const std::string& contents_dir_path)
{
    bool result = false;

    // update the /nv/timestamp file that will be inside the zip.
    time_t utcnow = time(NULL);
    std::string timestamp_path(contents_dir_path);
    timestamp_path.append("/timestamp");
    int fd = open(timestamp_path.c_str(), O_CREAT|O_RDWR);
    if (-1 != fd)
    {
        if (sizeof(utcnow) == write(fd, &utcnow, sizeof(utcnow)))
        {
            result = true;
        }
        close(fd);
        fd = -1;
        chmod(timestamp_path.c_str(), S_IRUSR|S_IWUSR);
    }

    if (result)
    {
        // zip up the zipdir_path directory contents
        std::string zipfile_path;
        result = this->zip_exec(contents_dir_path, zipfile_path);
        if (result)
        {
            std::string blockfile_path("/tmp");
            result = this->create_random_datafile(blockfile_path);
            if (result)
            {
                result = false;
                // now we can overlay the zip file into the random data file
                // starting at offset 31
                fd = open(blockfile_path.c_str(), O_WRONLY);
                if (-1 != fd)
                {
                    if (-1 != lseek(fd, 31, SEEK_SET))
                    {
                        int zipfd = open(zipfile_path.c_str(), O_RDONLY);
                        if (-1 != zipfd)
                        {
                            char* buffer = new char[64*1024];
                            ssize_t cb = read(zipfd, buffer, 64*1024);
                            if (-1 != cb)
                            {
                                if (cb == write(fd, buffer, cb))
                                {
                                    result = true;
                                }
                            }
                            delete[] buffer;
                            buffer = NULL;
                            close(zipfd);
                            zipfd = -1;
                        }
                    }
                    close(fd);
                    fd = -1;
                }

                result = this->mesaflash_exec("--nvwrite", blockfile_path, "A");
                if (result)
                {
                    result = this->mesaflash_exec("--nvwrite", blockfile_path, "B");
                }

                unlink(blockfile_path.c_str());
            }

            unlink(zipfile_path.c_str());
        }
    }

    return result;
}



int main()
{
    FlashStorage fs("5i25", "");

    bool result = fs.write_flash("/tmp/nvflash");
    if (result)
    {
        std::cout << "Success!" << std::endl;

        result = fs.read_flash();
    }

    return 0;
}
