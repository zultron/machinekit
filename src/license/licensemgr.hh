
#pragma once

#include "storage.hh"

class LicenseMgr
{
private:
    Storage _storage;

public:
    LicenseMgr(const std::string& ini_file_path);

	void perform_boot_maintenance();

	// Called by Cycle Start effectively - any time we try to switch into EMC_TASK_MODE_AUTO
	bool is_license_valid();
};
