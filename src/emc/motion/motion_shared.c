#include "motion_shared.h"
#include "motion_struct.h"
#include "rtapi_math.h"
#include "math_util.h"

double get_spindle_speed_out_rpm(struct emcmot_status_t const * const stat)
{
    if(stat->spindle_cmd.css_factor) {
        double denom = fabs(stat->spindle_cmd.xoffset - emcmotStatus->carte_pos_cmd.tran.x);
        double speed;
        double maxpositive;
        if(denom > 0) speed = stat->spindle_cmd.css_factor / denom;
        else speed = stat->spindle_cmd.velocity_rpm_out;

        speed = speed * stat->net_spindle_scale;

        maxpositive = fabs(stat->spindle_cmd.velocity_rpm_out);
        // cap speed to G96 D...
        if(speed < -maxpositive)
            speed = -maxpositive;
        if(speed > maxpositive)
            speed = maxpositive;
        return speed;
    } else {
        return stat->spindle_cmd.velocity_rpm_out * emcmotStatus->net_spindle_scale;
    }
}

double get_spindle_command_direction(struct emcmot_status_t const * const stat)
{
    return signum(stat->spindle_cmd.velocity_rpm_out);
}

int is_spindle_on(const emcmot_status_t * const stat)
{
    return ((stat->spindle_cmd.velocity_rpm_out * stat->net_spindle_scale) != 0) ? 1 : 0;
}
