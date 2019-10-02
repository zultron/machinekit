#ifndef SAICANON_HH
#define SAICANON_HH

#include <stdio.h>
#include <interp_fwd.hh>
#include <canon.hh>
#include <string>
#include <coordinatetransform.hh>
#include <interp_parameter_def.hh>

struct StandaloneInterpInternals;
class InterpBase;

extern StandaloneInterpInternals _sai;
extern InterpBase *pinterp;
extern FILE *_outfile;

struct StandaloneInterpInternals
{
  StandaloneInterpInternals();
  /* Dummy world model */

  CANON_PLANE _active_plane;
  int _active_slot;
  int _feed_mode;
  double _feed_rate;
  int _flood;
  CANON_UNITS _length_unit_type;
  int _line_number;
  int _mist;
  CANON_MOTION_MODE _motion_mode;
  PositionArray _probe_position;
  double _g5x_x;
  double _g5x_y;
  double _g5x_z;
  double _g5x_a;
  double _g5x_b;
  double _g5x_c;
  ShiftTransform g92_offset;
  AxisOffsetParams axis_offset_param;
  PositionArray _program_position;
  double _spindle_speed;
  CANON_DIRECTION _spindle_turning;
  int _pockets_max;
  CANON_TOOL_TABLE _tools[CANON_POCKETS_MAX];
  WorkOffsetTable _g5x_table; // Stored according to 1-indexed work offset number
  UnitsTransform programToUser;
  /* optional program stop */
  bool optional_program_stop;
  /* optional block delete */
  bool block_delete;
  double motion_tolerance;
  double naivecam_tolerance;
  /* Dummy status variables */
  double _traverse_rate;

  EmcPose _tool_offset;
  bool _toolchanger_fault;
  int  _toolchanger_reason ;
};

void reset_internals();
#endif // SAICANON_HH
