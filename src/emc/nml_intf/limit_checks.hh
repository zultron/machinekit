#ifndef LIMIT_CHECKS_HH
#define LIMIT_CHECKS_HH

#include <emcpos.h>
#include <string>
#include "interp_shared_fwd.hh"
using namespace interp_shared;

unsigned jointAccelViolation(int joint_idx, double acc);
unsigned jointVelocityViolation(int joint_idx, double v_actual);

unsigned findAccelViolations(PositionArray const &axis_accel);
unsigned findVelocityViolations(PositionArray const &axis_vel);
unsigned findMaxPositionViolations(PositionArray const &axis_pos);
unsigned findMinPositionViolations(PositionArray const& axis_pos);

std::string formatAxisError(unsigned below, unsigned above, const char *msg_prefix, int line_number=0);

#endif // LIMIT_CHECKS_HH
