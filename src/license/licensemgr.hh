
#pragma once

#include "storage.hh"

class LicenseMgr
{
private:
    Storage& _storage;

public:
    LicenseMgr(Storage& storage);

    bool refresh_cleartext_files();
    void refresh_flash();
};
