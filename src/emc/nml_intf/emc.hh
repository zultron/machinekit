/********************************************************************
* Description: emc.hh
*   Declarations for EMC NML vocabulary
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
#ifndef EMC_HH
#define EMC_HH

#include "config.h"
#include "emcglb.h"		// EMC_AXIS_MAX
#include "nml_type.hh"
#include "motion_types.h"
#include "emc_types.hh"

// --------------
// EMC VOCABULARY
// --------------

// NML formatting function
extern int emcFormat(NMLTYPE type, void *buffer, CMS * cms);

// NML Symbol Lookup Function
extern const char *emc_symbol_lookup(long type);
#define emcSymbolLookup(a) emc_symbol_lookup(a)

// ------------------------
// IMPLEMENTATION FUNCTIONS
// ------------------------

// implementation functions for EMC error, message types
// intended to be implemented in main() file, by writing to NML buffer

// print an error
extern int emcOperatorError(int id, const char *fmt, ...) __attribute__((format(printf,2,3)));

// print general text
extern int emcOperatorText(int id, const char *fmt, ...) __attribute__((format(printf,2,3)));

// print note to operator
extern int emcOperatorDisplay(int id, const char *fmt, ...) __attribute__((format(printf,2,3)));

// implementation functions for EMC_AXIS types

extern int emcAxisSetAxis(int axis, unsigned char axisType);
extern int emcAxisSetUnits(int axis, double units);
extern int emcAxisSetBacklash(int axis, double backlash);
extern int emcAxisSetMinPositionLimit(int axis, double limit);
extern int emcAxisSetMaxPositionLimit(int axis, double limit);
extern int emcAxisSetMotorOffset(int axis, double offset);
extern int emcAxisSetFerror(int axis, double ferror);
extern int emcAxisSetMinFerror(int axis, double ferror);
extern int emcAxisSetHomingParams(int axis, double home, double offset, double home_final_vel,
				  double search_vel, double latch_vel,
				  int use_index, int ignore_limits,
				  int is_shared, int home_sequence, int volatile_home, int locking_indexer);
extern int emcAxisSetMaxVelocity(int axis, double vel);
extern int emcAxisSetMaxAcceleration(int axis, double acc);

extern int emcAxisInit(int axis);
extern int emcAxisHalt(int axis);
extern int emcAxisAbort(int axis);
extern int emcAxisEnable(int axis);
extern int emcAxisDisable(int axis);
extern int emcAxisHome(int axis);
extern int emcAxisUnhome(int axis);
extern int emcAxisJog(int axis, double vel);
extern int emcAxisIncrJog(int axis, double incr, double vel);
extern int emcAxisAbsJog(int axis, double pos, double vel);
extern int emcAxisActivate(int axis);
extern int emcAxisDeactivate(int axis);
extern int emcAxisOverrideLimits(int axis);
extern int emcAxisLoadComp(int axis, const char *file, int type);


extern int emcAxisUpdate(EMC_AXIS_STAT stat[], int numAxes);

// implementation functions for EMC_TRAJ types

extern int emcTrajSetAxes(int axes, int axismask);
extern int emcTrajSetUnits(double linearUnits, double angularUnits);
extern int emcTrajSetCycleTime(double cycleTime);
extern int emcTrajSetMode(int axes);
extern int emcTrajSetTeleopVector(EmcPose vel);
extern int emcTrajSetVelocity(double vel, double ini_maxvel);
extern int emcTrajSetAcceleration(double acc);
extern int emcTrajSetMaxVelocity(double vel);
extern int emcTrajSetMaxAcceleration(double acc);
extern int emcTrajSetScale(double scale);
extern int emcTrajSetFOEnable(unsigned char mode);   //feed override enable
extern int emcTrajSetFHEnable(unsigned char mode);   //feed hold enable
extern int emcTrajSetSpindleScale(double scale);
extern int emcTrajSetSOEnable(unsigned char mode);   //spindle speed override enable
extern int emcTrajSetAFEnable(unsigned char enable); //adaptive feed enable
extern int emcTrajSetMotionId(int id);
extern double emcTrajGetLinearUnits();
extern double emcTrajGetAngularUnits();

extern int emcTrajInit();
extern int emcTrajHalt();
extern int emcTrajEnable();
extern int emcTrajDisable();
extern int emcTrajAbort();
extern int emcTrajPause();
extern int emcTrajStep();
extern int emcTrajResume();
extern int emcTrajDelay(double delay);
extern int emcTrajLinearMove(EmcPose end, int type, double vel,
                             double ini_maxvel, double acc, int indexrotary);
extern int emcTrajCircularMove(EmcPose end, PM_CARTESIAN center, PM_CARTESIAN
        normal, int turn, int type, double vel, double ini_maxvel, double acc);
extern int emcTrajSetTermCond(int cond, double tolerance);
extern int emcTrajSetSpindleSync(double feed_per_revolution, bool wait_for_index);
extern int emcTrajSetOffset(EmcPose tool_offset);
extern int emcTrajSetOrigin(EmcPose origin);
extern int emcTrajSetRotation(double rotation);
extern int emcTrajSetHome(EmcPose home);
extern int emcTrajClearProbeTrippedFlag();
extern int emcTrajProbe(EmcPose pos, int type, double vel, 
                        double ini_maxvel, double acc, unsigned char probe_type);
extern int emcAuxInputWait(int index, int input_type, int wait_type, int timeout);
extern int emcTrajRigidTap(EmcPose pos, double vel, double ini_maxvel, double acc);

extern int emcTrajUpdate(EMC_TRAJ_STAT * stat);

// implementation functions for EMC_MOTION aggregate types

extern int emcMotionInit();
extern int emcMotionHalt();
extern int emcMotionAbort();
extern int emcMotionSetDebug(int debug);
extern int emcMotionSetAout(unsigned char index, double start, double end,
                            unsigned char now);
extern int emcMotionSetDout(unsigned char index, unsigned char start,
			    unsigned char end, unsigned char now);

extern int emcMotionUpdate(EMC_MOTION_STAT * stat);

extern int emcAbortCleanup(int reason,const char *message = "");

// implementation functions for EMC_TOOL types

extern int emcToolInit();
extern int emcToolHalt();
extern int emcToolAbort();
extern int emcToolPrepare(int pocket, int tool);
extern int emcToolLoad();
extern int emcToolUnload();
extern int emcToolLoadToolTable(const char *file);
extern int emcToolSetOffset(int pocket, int toolno, EmcPose offset, double diameter,
                            double frontangle, double backangle, int orientation);
extern int emcToolSetNumber(int number);
extern int emcToolStartChange();

extern int emcToolSetToolTableFile(const char *file);

extern int emcToolUpdate(EMC_TOOL_STAT * stat);

// implementation functions for EMC_AUX types

extern int emcAuxEstopOn();
extern int emcAuxEstopOff();

extern int emcAuxUpdate(EMC_AUX_STAT * stat);

// implementation functions for EMC_SPINDLE types

extern int emcSpindleAbort();
extern int emcSpindleSpeed(double speed, double factor, double xoffset);
extern int emcSpindleOn(double speed, double factor, double xoffset);
extern int emcSpindleOrient(double orientation, int direction);
extern int emcSpindleWaitOrientComplete(double timout);
extern int emcSpindleOff();
extern int emcSpindleIncrease();
extern int emcSpindleDecrease();
extern int emcSpindleConstant();
extern int emcSpindleBrakeRelease();
extern int emcSpindleBrakeEngage();

extern int emcSpindleSetMode(int mode); //determines if Spindle needs to reset on abort

extern int emcSpindleUpdate(EMC_SPINDLE_STAT * stat);

// implementation functions for EMC_COOLANT types

extern int emcCoolantMistOn();
extern int emcCoolantMistOff();
extern int emcCoolantFloodOn();
extern int emcCoolantFloodOff();

extern int emcCoolantUpdate(EMC_COOLANT_STAT * stat);

// implementation functions for EMC_LUBE types

extern int emcLubeOn();
extern int emcLubeOff();

extern int emcLubeUpdate(EMC_LUBE_STAT * stat);

// implementation functions for EMC_IO types

extern int emcIoInit();
extern int emcIoHalt();
extern int emcIoAbort(int reason);
extern int emcIoSetCycleTime(double cycleTime);
extern int emcIoSetDebug(int debug);

extern int emcIoUpdate(EMC_IO_STAT * stat);

// implementation functions for EMC aggregate types

extern int emcInit();
extern int emcHalt();
extern int emcAbort();

int emcSetMaxFeedOverride(double maxFeedScale);
int emcSetupArcBlends(int arcBlendEnable,
        int arcBlendFallbackEnable,
        int arcBlendOptDepth,
        double arcBlendGapCycles,
        double arcBlendRampFreq);

extern int emcUpdate(EMC_STAT * stat);

#endif				// #ifndef EMC_HH
