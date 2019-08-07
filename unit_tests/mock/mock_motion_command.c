#include "mot_priv.h"

// Mocks for motion functions
void emcmotCommandHandler(void *arg, long period){}
void emcmotController(void *arg, long period){}
void emcmotSetCycleTime(unsigned long nsec){}

/* these are related to synchronized I/O */
void emcmotDioWrite(int index, char value){}
void emcmotAioWrite(int index, double value){}

void emcmotSetRotaryUnlock(int axis, int unlock){}
int emcmotGetRotaryIsUnlocked(int axis){return 1;}

/* homing is no longer in control.c, make functions public */
void do_homing_sequence(void){}
void do_homing(void){}

/* loops through the active joints and checks if any are not homed */
int checkAllHomed(void){return 1;}
/* recalculates jog limits */
void refresh_jog_limits(struct emcmot_joint_t *joint){}
/* handles 'homed' flags, see command.c for details */
void clearHomes(int joint_num){}

void emcmot_config_change(void){}
