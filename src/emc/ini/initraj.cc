/********************************************************************
* Description: initraj.cc
*   INI file initialization for trajectory level
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

#include <stdio.h>		// NULL
#include <stdlib.h>		// atol()
#include <string.h>		// strlen()
#include <ctype.h>		// isspace()

#include "emc.hh"
#include "emcpos.h"             // EmcPose
#include "rcs_print.hh"
#include "posemath.h"		// PM_POSE, PM_RPY
#include "emcIniFile.hh"
#include "initraj.hh"		// these decls
#include "emcglb.h"		// A bunch of global values used across canon and interp
#include "inihal.hh"

extern value_inihal_data old_inihal_data;

// Follow interp convention to return on error
#define CHP(call__) \
do { \
    if (call__) { \
        if (emc_debug & EMC_DEBUG_CONFIG) { \
            rcs_print("%s: %d: bad return value from %s\n", __FUNCTION__, __LINE__, #call__); \
        } \
        return -1; \
    } \
} while (0)

struct FindsIniFieldOrDefault
{
    EmcIniFile &inifile;
    const char *section;
    FindsIniFieldOrDefault(EmcIniFile &inifile_in, const char *section_in) : inifile(inifile_in), section(section_in) {}

    template <typename T>
    T operator()(const char *section_name, const char *field_name, T def_value) const {
        T val = def_value;
        inifile.Find(&val, field_name, section_name);
        return val;
    }
    template <typename T>
    T operator()(const char *field_name, T def_value) const {
        return (*this)(section, field_name, def_value);
    }
};

/*
  loadTraj()

  Loads ini file params for traj

  AXES <int>                    number of axes in system
  LINEAR_UNITS <float>          units per mm
  ANGULAR_UNITS <float>         units per degree
  CYCLE_TIME <float>            cycle time for traj calculations
  DEFAULT_VELOCITY <float>      default velocity
  MAX_VELOCITY <float>          max velocity
  MAX_ACCELERATION <float>      max acceleration
  DEFAULT_ACCELERATION <float>  default acceleration
  HOME <float> ...              world coords of home, in X Y Z R P W

  calls:

  emcTrajSetAxes(int axes);
  emcTrajSetUnits(double linearUnits, double angularUnits);
  emcTrajSetCycleTime(double cycleTime);
  emcTrajSetVelocity(double vel);
  emcTrajSetAcceleration(double acc);
  emcTrajSetMaxVelocity(double vel, double vela);
  emcTrajSetMaxAcceleration(double acc);
  emcTrajSetHome(EmcPose home);
  */

static int loadTraj(EmcIniFile &trajInifile)
{
    FindsIniFieldOrDefault ini_traj_find(trajInifile, "TRAJ");
    const char *inistring;
    EmcLinearUnits linearUnits;
    EmcAngularUnits angularUnits;
    double vel, vela;
    double acc;
    unsigned char coordinateMark[6] = { 1, 1, 1, 0, 0, 0 };
    int t;
    int len;
    char homes[LINELEN];
    char home[LINELEN];
    EmcPose homePose = { {0.0, 0.0, 0.0}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    double d;

    trajInifile.EnableExceptions(EmcIniFile::ERR_CONVERSION);

    try {
        int axes = 0;
        int axismask = 0;
        const char *coord = trajInifile.Find("COORDINATES", "TRAJ");
        if(coord) {
            if(strchr(coord, 'x') || strchr(coord, 'X')) axismask |= 1;
            if(strchr(coord, 'y') || strchr(coord, 'Y')) axismask |= 2;
            if(strchr(coord, 'z') || strchr(coord, 'Z')) axismask |= 4;
            if(strchr(coord, 'a') || strchr(coord, 'A')) axismask |= 8;
            if(strchr(coord, 'b') || strchr(coord, 'B')) axismask |= 16;
            if(strchr(coord, 'c') || strchr(coord, 'C')) axismask |= 32;
            if(strchr(coord, 'u') || strchr(coord, 'U')) axismask |= 64;
            if(strchr(coord, 'v') || strchr(coord, 'V')) axismask |= 128;
            if(strchr(coord, 'w') || strchr(coord, 'W')) axismask |= 256;
        } else {
            axismask = 1 | 2 | 4;		// default: XYZ machine
        }
        trajInifile.Find(&axes, "AXES", "TRAJ");

        if (0 != emcTrajSetAxes(axes, axismask)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetAxes\n");
            }
            return -1;
        }

        linearUnits = 0;
        trajInifile.FindLinearUnits(&linearUnits, "LINEAR_UNITS", "TRAJ");
        angularUnits = 0;
        trajInifile.FindAngularUnits(&angularUnits, "ANGULAR_UNITS", "TRAJ");

        if (0 != emcTrajSetUnits(linearUnits, angularUnits)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetUnits\n");
            }
            return -1;
        }

        vel = 1.0;
        trajInifile.Find(&vel, "DEFAULT_VELOCITY", "TRAJ");

        // set the corresponding global
        traj_default_velocity = vel;
        old_inihal_data.traj_default_velocity = vel;

        // and set dynamic value
        if (0 != emcTrajSetVelocity(0, vel)) { //default velocity on startup 0
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetVelocity\n");
            }
            return -1;
        }

        vel = 1e99; // by default, use AXIS limit
        vela = 1e99;
        trajInifile.Find(&vel, "MAX_VELOCITY", "TRAJ");
        trajInifile.Find(&vela, "MAX_ANGULAR_VELOCITY", "TRAJ");

        // set the corresponding global
        traj_max_velocity = vel;
        old_inihal_data.traj_max_velocity = vel;
        old_inihal_data.traj_max_angular_velocity = vela;

        // and set dynamic value
        if (0 != emcTrajSetMaxVelocity(vel, vela)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetMaxVelocity\n");
            }
            return -1;
        }

        acc = 1e99; // let the axis values apply
        trajInifile.Find(&acc, "DEFAULT_ACCELERATION", "TRAJ");

        if (0 != emcTrajSetAcceleration(acc)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetAcceleration\n");
            }
            return -1;
        }
        old_inihal_data.traj_default_acceleration = acc;

        acc = 1e99; // let the axis values apply
        trajInifile.Find(&acc, "MAX_ACCELERATION", "TRAJ");

        if (0 != emcTrajSetMaxAcceleration(acc)) {
            if (emc_debug & EMC_DEBUG_CONFIG) {
                rcs_print("bad return value from emcTrajSetMaxAcceleration\n");
            }
            return -1;
        }
        old_inihal_data.traj_max_acceleration = acc;

        tp_arc_blend_config_t arc_blend_cfg = {
            ini_traj_find("ARC_BLEND_ENABLE", 1),
            ini_traj_find("ARC_BLEND_FALLBACK_ENABLE", 1),
            ini_traj_find("ARC_BLEND_OPTIMIZATION_DEPTH", 50),
            ini_traj_find("ARC_BLEND_GAP_CYCLES", 4),
            ini_traj_find("ARC_BLEND_RAMP_FREQ", 100.0),
            ini_traj_find("ARC_BLEND_RAMP_FREQ", 0.1),
        };

        CHP(emcSetupArcBlends(arc_blend_cfg));

        // Store default blend tolerance in emc globals
        trajInifile.Find(&traj_default_tolerance, "DEFAULT_TOLERANCE", "TRAJ");
        trajInifile.Find(&traj_default_naivecam_tolerance, "DEFAULT_NAIVECAM_TOLERANCE", "TRAJ");

        old_inihal_data.traj_arc_blend_enable = arc_blend_cfg.enable;
        old_inihal_data.traj_arc_blend_fallback_enable = arc_blend_cfg.allow_fallback;
        old_inihal_data.traj_arc_blend_optimization_depth = arc_blend_cfg.optimization_depth;
        old_inihal_data.traj_arc_blend_gap_cycles = arc_blend_cfg.gap_cycles;
        old_inihal_data.traj_arc_blend_ramp_freq = arc_blend_cfg.ramp_frequency;
        old_inihal_data.traj_arc_blend_tangent_kink_ratio = arc_blend_cfg.tangent_kink_ratio;
        //TODO update inihal

        consistency_check_config_t consistency_check_cfg = {
            (unsigned)ini_traj_find("EXTRA_CONSISTENCY_CHECKS", 0),
            ini_traj_find("MAX_POSITION_DRIFT_ERROR", 0.0001),
            ini_traj_find("CONTINUITY_ANGLE_LIMIT_DEG", 0.00005) * M_PI / 180.,
        };
        CHP(emcSetupConsistencyChecks(
            consistency_check_cfg
            ));

        CHP(emcSetMaxFeedOverride(
            ini_traj_find("MAX_FEED_OVERRIDE", 1.0)
            ));

        CHP(emcSetMaxPosTrackingError(
            ini_traj_find("MAX_POSITION_TRACKING_ERROR", (double)INFINITY)
            ));

        FindsIniFieldOrDefault ini_biarc_find(trajInifile, "TRAJ_BIARC_SOLVER");
        biarc_solver_config_t biarc_solver_cfg = {
            (unsigned)ini_biarc_find("MAX_ITERATIONS", 15 ),
            ini_biarc_find("VELOCITY_REL_TOLERANCE", 0.02),
            ini_biarc_find("VELOCITY_ABS_TOLERANCE", 0.001),
            ini_biarc_find("DEVIATION_ABS_TOLERANCE", 0.000001),
            ini_biarc_find("FEED_OVERRIDE_ALLOWANCE", 1.5),
            ini_biarc_find("VELOCITY_CUTOFF", 0.001),
            ini_biarc_find("CONVERGENCE_TOLERANCE", 1e-8),
        };
        CHP(emcSetupBiArcSolverCfg(biarc_solver_cfg));
        old_inihal_data.traj_biarc_max_iterations = biarc_solver_cfg.max_iterations;
        old_inihal_data.traj_biarc_max_iterations = biarc_solver_cfg.max_iterations;
        old_inihal_data.traj_biarc_velocity_rel_tolerance = biarc_solver_cfg.velocity_rel_tolerance;
        old_inihal_data.traj_biarc_velocity_abs_tolerance = biarc_solver_cfg.velocity_abs_tolerance;
        old_inihal_data.traj_biarc_deviation_abs_tolerance = biarc_solver_cfg.deviation_abs_tolerance;
        old_inihal_data.traj_biarc_feed_override_allowance = biarc_solver_cfg.feed_override_allowance;
        old_inihal_data.traj_biarc_velocity_cutoff = biarc_solver_cfg.velocity_cutoff;
        old_inihal_data.traj_biarc_convergence_tolerance = biarc_solver_cfg.convergence_tolerance;
    }

    catch(EmcIniFile::Exception &e){
        e.Print();
        return -1;
    }

    // set coordinateMark[] to hold 1's for each coordinate present,
    // so that home position can be interpreted properly
    if ((inistring = trajInifile.Find("COORDINATES", "TRAJ")) != NULL) {
        len = strlen(inistring);
        // there's an entry in ini file, so clear all the marks out first
        // so that defaults don't apply at all
        for (t = 0; t < 6; t++) {
            coordinateMark[t] = 0;
        }
        // now set actual marks
        for (t = 0; t < len; t++) {
            if (inistring[t] == 'X')
                coordinateMark[0] = 1;
            else if (inistring[t] == 'Y')
                coordinateMark[1] = 1;
            else if (inistring[t] == 'Z')
                coordinateMark[2] = 1;
            else if (inistring[t] == 'R')
                coordinateMark[3] = 1;
            else if (inistring[t] == 'P')
                coordinateMark[4] = 1;
            else if (inistring[t] == 'W')
                coordinateMark[5] = 1;
        }
    }

    if (NULL != (inistring = trajInifile.Find("HOME", "TRAJ"))) {
	// found it, now interpret it according to coordinateMark[]
	strcpy(homes, inistring);
	len = 0;
	for (t = 0; t < 6; t++) {
	    if (!coordinateMark[t]) {
		continue;	// position t at index of next non-zero mark
	    }
	    // there is a mark, so read the string for a value
	    if (1 == sscanf(&homes[len], "%s", home) &&
		1 == sscanf(home, "%lf", &d)) {
		// got an entry, index into coordinateMark[] is 't'
		if (t == 0)
		    homePose.tran.x = d;
		else if (t == 1)
		    homePose.tran.y = d;
		else if (t == 2)
		    homePose.tran.z = d;
		else if (t == 3)
		    homePose.a = d;
		else if (t == 4)
		    homePose.b = d;
		else
		    homePose.c = d;

		// position string ptr past this value
		len += strlen(home);
		// and at start of next value
		while ((len < LINELEN) && (homes[len] == ' ' || homes[len] == '\t')) {
		    len++;
		}
		if (len >= LINELEN) {
		    break;	// out of for loop
		}
	    } else {
		// badly formatted entry
		rcs_print("invalid inifile value for [TRAJ] HOME: %s\n",
			  inistring);
                return -1;
	    }
	}			// end of for-loop on coordinateMark[]
    }

    if (0 != emcTrajSetHome(homePose)) {
	if (emc_debug & EMC_DEBUG_CONFIG) {
	    rcs_print("bad return value from emcTrajSetHome\n");
	}
	return -1;
    }

    return 0;
}

/*
  iniTraj(const char *filename)

  Loads ini file parameters for trajectory, from [TRAJ] section
 */
int iniTraj(const char *filename)
{
    EmcIniFile trajInifile;

    if (!trajInifile.Open(filename)) {
        return -1;
    }
    // load trajectory values
    CHP(loadTraj(trajInifile));

    return 0;
}
