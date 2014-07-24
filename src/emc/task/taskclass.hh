#ifndef TASKCLASS_HH
#define TASKCLASS_HH

#include "emc_types.hh"
#include "initool.hh"
#include "tool_parse.h"
#include "inifile.hh"
#include "cmd_msg.hh"

class Task {
public:
    Task();

    int emcIoInit();
    int emcIoHalt();
    int emcIoAbort(int reason);
    int emcToolStartChange();
    int emcAuxEstopOn();
    int emcAuxEstopOff();
    int emcCoolantMistOn();
    int emcCoolantMistOff();
    int emcCoolantFloodOn();
    int emcCoolantFloodOff();
    int emcLubeOn();
    int emcLubeOff();
    int emcIoSetDebug(int debug);
    int emcToolSetOffset(int pocket, int toolno, EmcPose offset, double diameter,
				 double frontangle, double backangle, int orientation);
    int emcToolPrepare(int p, int tool);
    int emcToolPassMsg(RCS_CMD_MSG * toolMsg);
    int emcToolLoad();
    int emcToolLoadToolTable(const char *file);
    int emcToolUnload();
    int emcToolSetNumber(int number);
    int emcIoUpdate(EMC_IO_STAT * stat);

    int emcIoPluginCall(int len, const char *msg);
    int emcIoSetLineNumber( RCS_CMD_MSG * cmd);

    int use_iocontrol;
    int random_toolchanger;
    const char *ini_filename;
    const char *tooltable_filename;
private:

    char *ttcomments[CANON_POCKETS_MAX];
    int fms[CANON_POCKETS_MAX];
};

extern Task *task_methods;

#endif
