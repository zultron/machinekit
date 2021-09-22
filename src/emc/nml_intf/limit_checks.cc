#include "limit_checks.hh"
#include "emcglb.h"
#include "sstream"
#include <positionarray.hh>

unsigned jointAccelViolation(int joint_idx, double acc)
{
    // Allow some numerical tolerance here since max acceleration is a bit
    // fuzzy anyway. Torque is the true limiting factor, so there has to be
    // some slack here anyway due to variations in table load, friction, etc.
    static const double ABS_TOL = 1e-3;
    const double REL_TOL = 1.0 + ABS_TOL;
    const double a_max_nominal = axis_max_acceleration[joint_idx];
    const double a_limit = fmax(a_max_nominal * REL_TOL, a_max_nominal + ABS_TOL);
    return (unsigned)(fabs(acc) > a_limit) << joint_idx;
}

unsigned jointVelocityViolation(int joint_idx, double v_actual)
{
    // Allow some numerical tolerance here since max acceleration is a bit
    // fuzzy anyway. Torque is the true limiting factor, so there has to be
    // some slack here anyway due to variations in table load, friction, etc.
    static const double ABS_TOL = 1e-2;
    const double REL_ACC_TOL = 1.0 + ABS_TOL;
    const double v_max_nominal = axis_max_velocity[joint_idx];
    const double v_limit = fmax(v_max_nominal * REL_ACC_TOL, v_max_nominal + ABS_TOL);
    return (unsigned)(fabs(v_actual) > v_limit) << joint_idx;
}

/**
 * Checks for any acceleration violations based on axis limits.
 *
 * @return 0 if all acceleration bounds are respected, or a bit-set of failed axes (in XYZABCUVW bit order).
 */
unsigned findAccelViolations(PositionArray const &axis_accel)
{

    // Bit-mask each failure so we can report all failed axes
    unsigned fail_bits = (unsigned)(0x0);
    for (int i=0; i < EMCMOT_MAX_JOINTS; ++i) {
        fail_bits |= jointAccelViolation(i, axis_accel[i]);
    }
    return fail_bits;
}

unsigned findVelocityViolations(PositionArray const &axis_vel)
{

    // Bit-mask each failure so we can report all failed axes
    unsigned fail_bits = (unsigned)(0x0);
    for (int i=0; i < EMCMOT_MAX_JOINTS; ++i) {
        fail_bits |= jointVelocityViolation(i, axis_vel[i]);
    }
    return fail_bits;
}

unsigned findMaxPositionViolations(PositionArray const& axis_pos)
{
    static const double ABS_TOL = 1e-6;
    // Bit-mask each failure so we can report all failed axes
    unsigned fail_bits = (unsigned)(0x0);
    for (int i=0; i < EMCMOT_MAX_JOINTS; ++i) {
        fail_bits |= (unsigned)(axis_pos[i] > (axis_max_position_limit[i] + ABS_TOL)) << i;
    }
    return fail_bits;
}

unsigned findMinPositionViolations(PositionArray const& axis_pos)
{
    static const double ABS_TOL = 1e-6;
    // Bit-mask each failure so we can report all failed axes
    unsigned fail_bits = (unsigned)(0x0);
    for (int i=0; i < EMCMOT_MAX_JOINTS; ++i) {
        fail_bits |= (unsigned)(axis_pos[i] < (axis_min_position_limit[i] - ABS_TOL)) << i;
    }
    return fail_bits;
}

typedef struct {
    char axes[10];
} AxisMaskString;

AxisMaskString axisBitMaskToString(unsigned failed_axes)
{
    AxisMaskString axis_str = {{"XYZABCUVW"}};
    for (int i = 0; i < 9;++i) {
        if (failed_axes & (1 << i)) {
            continue;
        }
        axis_str.axes[i] =' ';
    }
    return axis_str;
}

std::string formatAxisError(unsigned below, unsigned above, const char *msg_prefix, int line_number)
{
    static const char axes[] = {"XYZABCUVW"};
    std::ostringstream oss;
    if (line_number) {
        oss << "Line " << line_number << ": ";
    }
    oss << (msg_prefix ?: "") << " ";
    static constexpr const char * const delim = ", ";
    static constexpr const size_t DELIM_LEN = 2;
    for (int i=0; i < EMCMOT_MAX_JOINTS; ++i) {
        const unsigned mask = 1 << i;
        if (below & mask) {
            oss << "-";
        } else if (above & mask) {
            oss << "+";
        } else {
            // No violation so skip printing the axis
            continue;
        }
        oss << axes[i] << delim;
    }
    std::string axis_err = oss.str();
    // NOTE: error string is always long enough to strip off the last delimiter
    return axis_err.substr(0, axis_err.size() - DELIM_LEN);
}
