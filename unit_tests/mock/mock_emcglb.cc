#include "emcglb.h"
#include "algorithm"
#include "emcpos.h"

char emc_inifile[LINELEN]="dummy.ini";
char emc_nmlfile[LINELEN]="dummy.nml";
char rs274ngc_startup_code[LINELEN]="";

int emc_debug = 0;
double emc_task_cycle_time = 0.01;
double emc_io_cycle_time = 0.01;
int emc_task_interp_max_len = 1000;
char tool_table_file[LINELEN]="dummy.tbl";

double traj_default_velocity = 1.0;
double traj_max_velocity = 1.0;

double axis_max_position_limit[EMCMOT_MAX_JOINTS];
double axis_min_position_limit[EMCMOT_MAX_JOINTS];
double axis_max_velocity[EMC_AXIS_MAX]={1,1,1,5,0,0,0,0,0};
double axis_max_acceleration[EMC_AXIS_MAX]={10,10,10,50,0,0,0,0,0};

struct EmcPose tool_change_position={{0,0,0},0,0,0,0,0,0};
unsigned char have_tool_change_position=0;

double traj_default_tolerance = 0.005;
double traj_default_naivecam_tolerance = 0.0;

int emc_traj_axes = 0;
unsigned emc_traj_axis_mask = 0;
int taskplanopen = 0;
