
#pragma once

#include "flashstorage.hh"

class License
{
private:
    FlashStorage& _flash;

public:
    License(FlashStorage& flash);
    bool refresh_cleartext_files();
    void refresh_flash();
};
