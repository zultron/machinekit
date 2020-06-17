/********************************************************************
* Description: emctask.cc
*   Mode and state management for EMC_TASK class
*
*   Derived from a work by Fred Proctor & Will Shackleford
*
* Author:
* License: GPL Version 2
* System: Linux
*    
* Copyright (c) 2004 All rights reserved.
*
* Last change:
********************************************************************/

#include <stdlib.h>
#include <string.h>		// strncpy()
#include <sys/stat.h>		// struct stat
#include <unistd.h>		// stat()
#include <limits.h>		// PATH_MAX
#include <dlfcn.h>

#include "rcs.hh"		// INIFILE
#include "emc.hh"		// EMC NML
#include "emc_types.hh"
#include "emc_statics.hh"
#include "emc_nml.hh"
#include "emcglb.h"		// EMC_INIFILE
#include "interpl.hh"		// NML_INTERP_LIST, interp_list
#include "canon.hh"		// CANON_VECTOR, GET_PROGRAM_ORIGIN()
#include "rs274ngc_interp.hh"	// the interpreter
#include "interp_return.hh"	// INTERP_FILE_NOT_OPEN
#include "inifile.hh"
#include "rcs_print.hh"
#include "task.hh"		// emcTaskCommand etc
#include "python_plugin.hh"
#include "taskclass.hh"
#include "modal_state.hh"
#include "interp_parameter_def.hh"
using namespace interp_param_global;

#define CALL_AND_RETURN(call__) do {\
    int retval = call__; \
    if (emc_debug & EMC_DEBUG_INTERP) { \
        rcs_print(__FILE__ ": %d: %s returned %d\n", __LINE__,  __FUNCTION__, retval); \
    } \
    if (retval > INTERP_MIN_ERROR) { \
        print_interp_error(retval); \
    } \
    return retval; \
} while (0)

#define USER_DEFINED_FUNCTION_MAX_DIRS 5
#define MAX_M_DIRS (USER_DEFINED_FUNCTION_MAX_DIRS+1)
//note:the +1 is for the PROGRAM_PREFIX or default directory==nc_files

/* flag for how we want to interpret traj coord mode, as mdi or auto */
static int mdiOrAuto = EMC_TASK_MODE_AUTO;

InterpBase *pinterp = nullptr;

setup_pointer _is = 0; // helper for gdb hardware watchpoints FIXME

// Saved tag for restoring interp after abort
static state_tag_t saved_tag = {};

/*
  format string for user-defined programs, e.g., "programs/M1%02d" means
  user-defined programs are in the programs/ directory and are named
  M1XX, where XX is a two-digit string.
*/
static char user_defined_fmt[MAX_M_DIRS][EMC_SYSTEM_CMD_LEN]; // ex: "dirname/M1%02d"

// index to directory for each user defined function:
static int user_defined_function_dirindex[USER_DEFINED_FUNCTION_NUM];

static void user_defined_add_m_code(int num, double arg1, double arg2)
{
    // num      is the m_code number, typically 00-99 corresponding to M100-M199
    char fmt[EMC_SYSTEM_CMD_LEN];
    EMC_SYSTEM_CMD system_cmd;

    //we call FINISH() to flush any linked motions before the M1xx call, 
    //otherwise they would mix badly
    FINISH();
    strcpy(fmt, user_defined_fmt[user_defined_function_dirindex[num]]);
    strcat(fmt, " %f %f");
    snprintf(system_cmd.string, sizeof(system_cmd.string), fmt, num, arg1, arg2);
    interp_list.append(system_cmd);
}

int emcTaskInit()
{
    char mdir[MAX_M_DIRS][PATH_MAX+1];
    int num,dct,dmax;
    char path[EMC_SYSTEM_CMD_LEN];
    struct stat buf;
    IniFile inifile;
    const char *inistring;

    inifile.Open(emc_inifile);

    // Identify user_defined_function directories
    if (NULL != (inistring = inifile.Find("PROGRAM_PREFIX", "DISPLAY"))) {
        strncpy(mdir[0],inistring, sizeof(mdir[0]));
        if (mdir[0][sizeof(mdir[0])-1] != '\0') {
            rcs_print("[DISPLAY]PROGRAM_PREFIX too long (max len %zu)\n", sizeof(mdir[0]));
            return -1;
        }
    } else {
        // default dir if no PROGRAM_PREFIX
        strncpy(mdir[0],"nc_files", sizeof(mdir[0]));
        if (mdir[0][sizeof(mdir[0])-1] != '\0') {
            rcs_print("default nc_files too long (max len %zu)\n", sizeof(mdir[0]));
            return -1;
        }
    }
    dmax = 1; //one directory mdir[0],  USER_M_PATH specifies additional dirs

    // user can specify a list of directories for user defined functions
    // with a colon (:) separated list
    if (NULL != (inistring = inifile.Find("USER_M_PATH", "RS274NGC"))) {
        char* nextdir;
        char tmpdirs[PATH_MAX];

        for (dct=1; dct < MAX_M_DIRS; dct++) mdir[dct][0] = 0;

        strncpy(tmpdirs,inistring, sizeof(tmpdirs));
        if (tmpdirs[sizeof(tmpdirs)-1] != '\0') {
            rcs_print("[RS274NGC]USER_M_PATH too long (max len %zu)\n", sizeof(tmpdirs));
            return -1;
        }

        nextdir = strtok(tmpdirs,":");  // first token
        dct = 1;
        while (dct < MAX_M_DIRS) {
            if (nextdir == NULL) break; // no more tokens
            strncpy(mdir[dct],nextdir, sizeof(mdir[dct]));
            if (mdir[dct][sizeof(mdir[dct])-1] != '\0') {
                rcs_print("[RS274NGC]USER_M_PATH component (%s) too long (max len %zu)\n", nextdir, sizeof(mdir[dct]));
                return -1;
            }
            nextdir = strtok(NULL,":");
            dct++;
        }
        dmax=dct;
    }
    inifile.Close();

    /* check for programs named programs/M100 .. programs/M199 and add
       any to the user defined functions list */
    for (num = 0; num < USER_DEFINED_FUNCTION_NUM; num++) {
	for (dct=0; dct < dmax; dct++) {
            char expanddir[LINELEN];
	    if (!mdir[dct][0]) continue;
            if (inifile.TildeExpansion(mdir[dct],expanddir,sizeof(expanddir))) {
		rcs_print("emcTaskInit: TildeExpansion failed for %s, ignoring\n",
			 mdir[dct]);
            }
	    snprintf(path, sizeof(path), "%s/M1%02d",expanddir,num);
	    if (0 == stat(path, &buf)) {
	        if (buf.st_mode & S_IXUSR) {
		    // set the user_defined_fmt string with dirname
		    // note the %%02d means 2 digits after the M code
		    // and we need two % to get the literal %
		    snprintf(user_defined_fmt[dct], sizeof(user_defined_fmt[0]), 
			     "%s/M1%%02d", expanddir); // update global
		    USER_DEFINED_FUNCTION_ADD(user_defined_add_m_code,num);
		    if (emc_debug & EMC_DEBUG_CONFIG) {
		        rcs_print("emcTaskInit: adding user-defined function %s\n",
			     path);
		    }
	            user_defined_function_dirindex[num] = dct;
	            break; // use first occurrence found for num
	        } else {
		    if (emc_debug & EMC_DEBUG_CONFIG) {
		        rcs_print("emcTaskInit: user-defined function %s found, but not executable, so ignoring\n",
			     path);
		    }
	        }
	    }
	}
    }

    return 0;
}

int emcTaskHalt()
{
    return 0;
}

int emcTaskStateRestore()
{
    // clear out the pending command
    emcTaskCommand = 0;

    // Do not discard the interp_list here.  So far, this is only called during abort cleanup
    // and its after the on abort ngc code has already been interpreted so the interp list is
    // valuable and must be run.
    // 2/20/2020 jwf
    //interp_list.clear();

    // Do NOT restore on MDI command
    if (emcStatus->task.mode == EMC_TASK_MODE_AUTO) {
        // Validity of state tag checked within restore function
	if (emc_debug & EMC_DEBUG_STATUS)
	    rcs_print("emcTaskStateRestore: restoring interp from line %d state tag\n",
		      saved_tag.fields[GM_FIELD_LINE_NUMBER]);

        pinterp->restore_from_tag(saved_tag);

        // Sigh...the interp object has some stale static tag data that can get used by
		// new commands that will be added to the interp list so update that.
        static_cast<Interp*>(pinterp)->write_and_update_state_tag((block_pointer) NULL);
		// And the interp list itself has a line number that is now stale also so be sure
		// to update that as well.
        interp_list.set_line_number(saved_tag.fields[GM_FIELD_LINE_NUMBER]);
    }
    return 0;
}

int emcTaskAbort()
{
    // Call motion abort before state restore to make absolutely sure no
    // stray restore commands make it down to motion
    emcMotionAbort();

    // clear out the pending command
    emcTaskCommand = 0;
    interp_list.clear();

    // clear out the interpreter state
    emcTaskSetInterpState(EMC_TASK_INTERP_IDLE);
    emcStatus->task.execState = EMC_TASK_EXEC_DONE;
    emcStatus->task.task_paused = 0;
    //FIXME should actually clear out the motion HAL pin?
    emcStatus->task.motionLine = 0;
    emcStatus->task.readLine = 0;
    emcStatus->task.command[0] = 0;
    emcStatus->task.callLevel = 0;

    stepping = 0;
    steppingWait = 0;

    // now queue up command to resynch interpreter

    // hits interp list directly so uses whatever is hanging around in there for 'next line number'   
    EMC_TASK_PLAN_SYNCH taskPlanSynchCmd;
    emcTaskQueueCommand(&taskPlanSynchCmd);

    // without emcTaskPlanClose(), a new run command resumes at
    // aborted line-- feature that may be considered later
    {
	int was_open = taskplanopen;
	emcTaskPlanClose();
	if (emc_debug & EMC_DEBUG_INTERP && was_open) {
	    rcs_print("emcTaskPlanClose() called at %s:%d\n", __FILE__,
		      __LINE__);
	}
    }

    return 0;
}

int emcTaskSetMode(int mode)
{
    int retval = 0;

    switch (mode) {
    case EMC_TASK_MODE_MANUAL:
	// go to manual mode
	emcTrajSetMode(EMC_TRAJ_MODE_FREE);
	mdiOrAuto = EMC_TASK_MODE_AUTO;	// we'll default back to here
	break;

    case EMC_TASK_MODE_MDI:
	// go to mdi mode
	emcTrajSetMode(EMC_TRAJ_MODE_COORD);
	emcTaskAbort();
	emcTaskPlanSynch();
	mdiOrAuto = EMC_TASK_MODE_MDI;
	break;

    case EMC_TASK_MODE_AUTO:
	// go to auto mode
	emcTrajSetMode(EMC_TRAJ_MODE_COORD);
	emcTaskAbort();
	emcTaskPlanSynch();
	mdiOrAuto = EMC_TASK_MODE_AUTO;
	break;

    default:
	retval = -1;
	break;
    }

    return retval;
}

/**
 * Common function to turn everything off for either e-stop or a user-requested power-down.
 */
static void abort_everything(EMC_IO_ABORT_REASON_ENUM reason)
{
    emcMotionAbort();
    emcSpindleAbort();
    for (int t = 0; t < emcStatus->motion.traj.axes; t++) {
        emcAxisDisable(t);
    }
    emcTrajDisable();
    task_methods->emcLubeOff();
    task_methods->emcIoAbort(reason);
    emcTaskAbort();
    emcAxisUnhome(-2); // only those joints which are volatile_home
    emcAbortCleanup(reason);
    emcTaskPlanSynch();
}

int emcTaskSetState(int state)
{
    int t;
    int retval = 0;

    // TODO actually catch errors here
    switch (state) {
    case EMC_TASK_STATE_OFF:
        abort_everything(EMC_ABORT_TASK_STATE_OFF);
        break;

    case EMC_TASK_STATE_ESTOP_RESET:
        // reset the estop but don't turn the machine fully back on
        task_methods->emcAuxEstopOff();
        task_methods->emcLubeOff();
        emcTaskAbort();
        task_methods->emcIoAbort(EMC_ABORT_TASK_STATE_ESTOP_RESET);
        emcSpindleAbort();
        emcAbortCleanup(EMC_ABORT_TASK_STATE_ESTOP_RESET);
        emcTaskPlanSynch();
        break;

    case EMC_TASK_STATE_ESTOP:
        task_methods->emcAuxEstopOn();
        abort_everything(EMC_ABORT_TASK_STATE_ESTOP);
        break;

    case EMC_TASK_STATE_ON:
        // turn the machine servos and lube on
        emcTrajEnable();
        for (t = 0; t < emcStatus->motion.traj.axes; t++) {
            emcAxisEnable(t);
        }
        task_methods->emcLubeOn();
        break;

    default:
        retval = -1;
        break;
    }

    return retval;
}

// WM access functions

/*
  determineMode()

  Looks at mode of subsystems, and returns associated mode

  Depends on traj mode, and mdiOrAuto flag

  traj mode   mdiOrAuto     mode
  ---------   ---------     ----
  FREE        XXX           MANUAL
  COORD       MDI           MDI
  COORD       AUTO          AUTO
  */
static int determineMode()
{
    // if traj is in free mode, then we're in manual mode
    if (emcStatus->motion.traj.mode == EMC_TRAJ_MODE_FREE ||
	emcStatus->motion.traj.mode == EMC_TRAJ_MODE_TELEOP) {
	return EMC_TASK_MODE_MANUAL;
    }
    // else traj is in coord mode-- we can be in either mdi or auto
    return mdiOrAuto;
}

/*
  determineState()

  Looks at state of subsystems, and returns associated state

  Depends on traj enabled, io estop, and desired task state

  traj enabled   io estop      state
  ------------   --------      -----
  DISABLED       ESTOP         ESTOP
  ENABLED        ESTOP         ESTOP
  DISABLED       OUT OF ESTOP  ESTOP_RESET
  ENABLED        OUT OF ESTOP  ON
  */
static int determineState()
{
    if (emcStatus->io.aux.estop) {
	return EMC_TASK_STATE_ESTOP;
    }

    if (!emcStatus->motion.traj.enabled) {
	return EMC_TASK_STATE_ESTOP_RESET;
    }

    return EMC_TASK_STATE_ON;
}

static int waitFlag = 0;

static char interp_error_text_buf[ERROR_TEXT_LEN];
// Don't need this buffer if we don't throw run-time warnings
//static char interp_warning_text_buf[LINELEN];
static char interp_stack_buf[LINELEN+1];

static void print_interp_error(int retval)
{
    int index = 0;
    if (retval == 0) {
	return;
    }

    if (0 != emcStatus) {
	emcStatus->task.interpreter_errcode = retval;
    }

    interp_error_text_buf[0] = 0;
    pinterp->error_text(retval, interp_error_text_buf, ERROR_TEXT_LEN);
    if (0 != interp_error_text_buf[0]) {
	rcs_print_error("interp_error: %s\n", interp_error_text_buf);
    }
    emcOperatorError("%s", interp_error_text_buf);
    index = 0;
    if (emc_debug & EMC_DEBUG_INTERP) {
	rcs_print("Interpreter stack: \t");
	while (index < 5) {
	    interp_stack_buf[0] = 0;
        pinterp->stack_name(index, interp_stack_buf, LINELEN);
	    if (0 == interp_stack_buf[0]) {
		break;
	    }
	    rcs_print(" - %s ", interp_stack_buf);
	    index++;
	}
	rcs_print("\n");
    }
}

int emcTaskPlanInit()
{
    if(!pinterp) {
	IniFile inifile;
	const char *inistring;
	inifile.Open(emc_inifile);
	if((inistring = inifile.Find("INTERPRETER", "TASK"))) {
	    pinterp = interp_from_shlib(inistring);
        fprintf(stderr, "interp_from_shlib() -> %p\n", (void*)pinterp);
	}
        inifile.Close();
    }
    if(!pinterp) {
        pinterp = new Interp;
    }

    Interp *rs274_interp = dynamic_cast<Interp*>(pinterp);
    _is = rs274_interp ? &rs274_interp->_setup : nullptr; // FIXME
    pinterp->ini_load(emc_inifile);
    waitFlag = 0;

    int retval = pinterp->init();
	// In task, enable M99 main program endless looping
    pinterp->set_loop_on_main_m99(true);
    if (retval > INTERP_MIN_ERROR) {  // I'd think this should be fatal.
	print_interp_error(retval);
    } else {
    if (0 != rs274ngc_startup_code[0]) {
        retval = pinterp->execute(rs274ngc_startup_code, CommandSource::INIT);
	    while (retval == INTERP_EXECUTE_FINISH) {
        retval = pinterp->execute();
	    }
	    if (retval > INTERP_MIN_ERROR) {
		print_interp_error(retval);
	    }
	}
    }

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanInit() returned %d\n", retval);
    }

    return retval;
}

int emcTaskPlanSetWait()
{
    waitFlag = 1;

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanSetWait() called\n");
    }

    return 0;
}

int emcTaskPlanIsWait()
{
    return waitFlag;
}

int emcTaskPlanClearWait()
{
    waitFlag = 0;

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanClearWait() called\n");
    }

    return 0;
}

int emcTaskPlanSetOptionalStop(bool state)
{
    SET_OPTIONAL_PROGRAM_STOP(state);
    return 0;
}

int emcTaskPlanSetBlockDelete(bool state)
{
    SET_BLOCK_DELETE(state);
    return 0;
}

int emcTaskPlanSynch()
{
    int retval = pinterp->synch();

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanSynch() returned %d\n", retval);
    }

    if (retval > INTERP_MIN_ERROR) {
        print_interp_error(retval);
    }
    return retval;
}

int emcTaskPlanFinishLineSeek(AutoRunSettings const &run_settings, ExpandedModalState const &run_initial_state)
{
    Interp *rs274_interp = dynamic_cast<Interp*>(pinterp);

    ExpandedModalState state_at_seek{};
    int res_interp = INTERP_OK;
    if (rs274_interp) {
        // NOTE: modal state is recorded before the transition for use in the restore sequence
        res_interp = rs274_interp->cbBeforeSeekReadTransition(run_settings, run_initial_state, state_at_seek);
    }

    emcTaskPlanPurgeInterpQueue();
    emcTaskSetInterpState(EMC_TASK_INTERP_READING);
    if (run_settings.single_step_mode) {
        emcOperatorText(CANON_MSG_DEBUG, "Pausing for single-step mode before start line %d", run_settings.start_line);
        emcTrajPause();
        emcTaskPause();
    }

    if (!isInterpError((InterpReturn)res_interp) && rs274_interp) {
        // Do a full modal state restore as if the program had executed from line 1,
        // and insert any preparatory actions like tool changes / lead-in moves.
        res_interp = rs274_interp->cbAfterSeekReadTransition(state_at_seek, run_settings, run_initial_state);
    } else {
        // Legacy behavior that just synchs up with world without doing explicit modal state restore
        emcTaskPlanSynch();
    }

    if (isInterpError((InterpReturn)res_interp)) {
        print_interp_error(res_interp);
        // Can't continue here with an error
        emcTaskSetInterpState(EMC_TASK_INTERP_WAITING);
    }
    return res_interp;
}

int emcTaskPlanSynchExport(const char *export_file)
{
    auto rs274_interp = dynamic_cast<Interp*>(pinterp);
    if (!rs274_interp) {
        return 0;
    }
    setup_struct tmp = rs274_interp->_setup;
    CALL_AND_RETURN(rs274_interp->synch_and_export(&tmp, export_file));
}

void emcTaskPlanExit()
{
    if (pinterp) {
        pinterp->exit();
    }
}

int emcTaskPlanOpen(const char *file, const char *working_dir)
{
    if (emcStatus != 0) {
        emcStatus->task.motionLine = 0;
        emcStatus->task.currentLine = 0;
        emcStatus->task.previousLine = 0;
        emcStatus->task.readLine = 0;
        emcStatus->task.recoveryLine = 0;
        emcStatus->task.stepping_id = 0;
    }

    int retval = pinterp->open(file, working_dir);
    if (retval > INTERP_MIN_ERROR) {
        print_interp_error(retval);
        return retval;
    }
    taskplanopen = 1;

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanOpen(%s) returned %d\n", file, retval);
    }

    return retval;
}


int emcTaskPlanRead()
{
    int retval = pinterp->read();
    if (retval == INTERP_FILE_NOT_OPEN) {
    if (emcStatus->task.file[0] != 0) {
        retval = pinterp->open(emcStatus->task.file, emcStatus->task.working_dir);
	    if (retval > INTERP_MIN_ERROR) {
		print_interp_error(retval);
	    }
	    retval = pinterp->read();
	}
    }
    if (retval > INTERP_MIN_ERROR) {
	print_interp_error(retval);
    }
    
    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanRead() returned %d\n", retval);
    }
    
    return retval;
}

int emcTaskPlanExecute()
{
    return emcTaskPlanExecute(nullptr, CommandSource::PROGRAM);
}

int emcTaskPlanExecute(const char *command, CommandSource cmd_src)
{
    int inpos = emcStatus->motion.traj.inpos;	// 1 if in position, 0 if not.

    if (command && (*command != '\0') && (inpos)) {
        // Need to synch with an MDI call, once motion has reached its final position
        int retval_sync = pinterp->synch();
        if (retval_sync > INTERP_MIN_ERROR) {
            print_interp_error(retval_sync);
        }
        //FIXME the standard behavior now is to keep going here but this seems wrong
    }
    int retval = pinterp->execute(command, cmd_src);
    if (retval > INTERP_MIN_ERROR) {
        print_interp_error(retval);
    }
    if(command) {
        FINISH();
    }

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanExecute() return %d\n", retval);
    }

    return retval;
}

int emcTaskPlanClose()
{
    int retval = pinterp->close();
    if (retval > INTERP_MIN_ERROR) {
	print_interp_error(retval);
    }

    taskplanopen = 0;
    return retval;
}

int emcTaskPlanReset()
{
    int retval = pinterp->reset();
    if (retval > INTERP_MIN_ERROR) {
	print_interp_error(retval);
    }
    emcStatus->task.recoveryLine = 0;

    return retval;
}

int emcTaskPlanLine()
{
    int retval = pinterp->line();
    
    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanLine() returned %d\n", retval);
    }

    return retval;
}

int emcTaskPlanLevel()
{
    int retval = pinterp->call_level();

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanLevel() returned %d\n", retval);
    }

    return retval;
}

int emcTaskPlanCommand(char *cmd)
{
    char buf[LINELEN];

    strcpy(cmd, pinterp->command(buf, LINELEN));

    if (emc_debug & EMC_DEBUG_INTERP) {
        rcs_print("emcTaskPlanCommand(%s) called. (line_number=%d)\n",
          cmd, emcStatus->task.readLine);
    }

    return 0;
}

int emcTaskUpdate(EMC_TASK_STAT * stat)
{
    stat->mode = (enum EMC_TASK_MODE_ENUM) determineMode();
    int oldstate = stat->state;
    stat->state = (enum EMC_TASK_STATE_ENUM) determineState();

    if(oldstate == EMC_TASK_STATE_ON && oldstate != stat->state) {
	emcTaskAbort();
        emcSpindleAbort();
        task_methods->emcIoAbort(EMC_ABORT_TASK_STATE_NOT_ON);
	emcAbortCleanup(EMC_ABORT_TASK_STATE_NOT_ON);
    }

    // execState set in main
    // interpState set in main
    int motionLine = emcStatus->motion.traj.id;
    if (emcStatus->motion.traj.id > 0) {
        stat->motionLine = motionLine;
    }
    // currentLine set in main
    // readLine set in main

    char buf[LINELEN];
    strcpy(stat->file, pinterp->file(buf, LINELEN));
    strcpy(stat->working_dir, pinterp->working_dir(buf, LINELEN));
    // command set in main

    if (emcStatus->task.interpState == EMC_TASK_INTERP_IDLE) {
    	// When the machine is idle, update from interp
        pinterp->active_g_codes(&stat->modalState.g_codes[0]);
        pinterp->active_m_codes(&stat->modalState.m_codes[0]);
        pinterp->active_settings(&stat->modalState.settings[0]);
    } else if (emcStatus->motion.traj.queue &&
	       (emcStatus->task.stepping_id !=
		emcStatus->motion.traj.tag.fields[GM_FIELD_STEPPING_ID])) {
	// When the machine is in motion, update from motion tags (if
	// stepping ID changed)
	emcStatusUpdateFromTag(&emcStatus->motion.traj.tag);
	if (emc_debug & EMC_DEBUG_STATUS)
	    rcs_print(
		"emcTaskUpdate:  updating status "
		"from motion tag line %d, stepping %d\n",
		emcStatus->motion.traj.tag.fields[GM_FIELD_LINE_NUMBER],
		emcStatus->motion.traj.tag.fields[GM_FIELD_STEPPING_ID]);
    }

    //update state of optional stop
    stat->optional_stop_state = GET_OPTIONAL_PROGRAM_STOP();

    //update state of block delete
    stat->block_delete_state = GET_BLOCK_DELETE();

    stat->motionTolerance = GET_MOTION_TOLERANCE();
    stat->naivecamTolerance = GET_NAIVECAM_TOLERANCE();

    stat->heartbeat++;

    return 0;
}

int emcStatusUpdateFromTag(state_tag_t const * const tag)
{
    if (!tag || !StateTag(*tag).is_valid()) {
	if (emc_debug & EMC_DEBUG_STATUS)
	    rcs_print(
		"emcStatusUpdateFromTag: Not updating from invalid tag\n");
        return -1;
    }

    // Save tag in case of later abort
    saved_tag = *tag;

    //Check if the new segment marks a new macro-level motion
    if (emcStatus->task.currentLine != tag->fields[GM_FIELD_LINE_NUMBER]) {
        emcStatus->task.previousLine = emcStatus->task.currentLine;
        emcStatus->task.currentLine = tag->fields[GM_FIELD_LINE_NUMBER];
        emcStatus->task.recoveryLine = emcStatus->task.mode == EMC_TASK_MODE_MDI ? 0 : emcStatus->task.currentLine;
    }
    emcStatus->task.stepping_id = tag->fields[GM_FIELD_STEPPING_ID];
    emcStatus->task.programUnits = (
	(tag->packed_flags & (1<<GM_FLAG_UNITS)) ?
	CANON_UNITS_INCHES : CANON_UNITS_MM );
    // Set emcStatus gcodes, mcodes arrays
    pinterp->active_modes(
        emcStatus->task.modalState,
        StateTag(*tag));

    // Add more emcStatus updates here
    return emcStatusUpdateWorkOffset(emcStatus->task.modalState.work_offset_number);
}

int emcStatusUpdateWorkOffset(int new_origin)
{
    if (new_origin < 0 || new_origin > (int)emcStatus->task.g5x_table.size()) {
        // Bad origin
        return -1;
    }
    emcStatus->task.g5x_index = new_origin;
    return 0;
}

int emcAbortCleanup(int reason, const char *message)
{
    int status = pinterp->on_abort(reason,message);
    if (status > INTERP_MIN_ERROR)
	print_interp_error(status);
    return status;
}

