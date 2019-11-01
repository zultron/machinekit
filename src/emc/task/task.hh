//    Copyright 2007 Jeff Epler <jepler@unpythonic.net>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#ifndef EMC_TASK_HH
#define EMC_TASK_HH
#include "taskclass.hh"

// global command pointer
extern NMLmsg *emcTaskCommand;

extern InterpBase *pinterp;
struct StartLineSettings;

extern int stepping;
extern int steppingWait;
extern int emcTaskQueueCommand(NMLmsg *cmd);
extern int emcPluginCall(EMC_EXEC_PLUGIN_CALL *call_msg);
extern int emcIoPluginCall(EMC_IO_PLUGIN_CALL *call_msg);
extern int emcTaskOnce(const char *inifile);
extern int emcRunHalFiles(const char *filename);

int emcTaskInit();
int emcTaskHalt();
int emcTaskStateRestore();
void emcTaskPause();
void emcTaskResume();
int emcTaskAbort(int restore_task_state);
int emcTaskAbort();
int emcTaskSetMode(int mode);
int emcTaskSetState(int state);
void emcTaskSetInterpState(EMC_TASK_INTERP_ENUM new_state);
int emcTaskPlanInit();
int emcTaskPlanSetWait();
int emcTaskPlanIsWait();
int emcTaskPlanClearWait();
int emcTaskPlanSynch();
void emcTaskPlanPurgeInterpQueue();
int emcTaskPlanFinishLineSeek(AutoRunSettings const &run_settings, ExpandedModalState const &run_initial_state);
int emcTaskPlanSetOptionalStop(bool state);
int emcTaskPlanSetBlockDelete(bool state);
void emcTaskPlanExit();
int emcTaskPlanOpen(const char *file, const char * working_dir);
int emcTaskPlanRead();
int emcTaskPlanExecute();
int emcTaskPlanExecute(const char *command, CommandSource cmd_src); //used in case of MDI to pass the pseudo line number to interp
int emcTaskPlanPause();
int emcTaskPlanResume();
int emcTaskPlanClose();
int emcTaskPlanReset();

int emcTaskPlanLine();
int emcTaskPlanLevel();
int emcTaskPlanCommand(char *cmd);

int emcTaskUpdate(EMC_TASK_STAT * stat);
int emcStatusUpdateFromTag(state_tag_t const * const tag);
int emcStatusUpdateWorkOffset(int new_origin);
#endif

