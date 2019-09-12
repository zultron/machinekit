/**
 * @file rtapi_json5.h
 *
 * @author Robert W. Ellenberg
 * @copyright Copyright (c) 2019 All rights reserved. This project is released under the GPL v2 license.
 */

#ifndef RTAPI_JSON5_H
#define RTAPI_JSON5_H

#include "rtapi.h"  /* printing functions */
#include "posemath.h"
#include "emcpos.h"
#include "blendmath.h"

// Macro wrappers to stringify the variable name
// NOTE: callers can directly call the underlying print function if a custom field name is needed
#define print_json5_double(varname_) print_json5_double_(#varname_, (double)varname_)
#define print_json5_string(varname_) print_json5_string_(#varname_, varname_)
#define print_json5_long(varname_) print_json5_long_(#varname_, (long)varname_)
#define print_json5_int(varname_) print_json5_int_(#varname_, (int)varname_)
#define print_json5_unsigned(varname_) print_json5_unsigned_(#varname_, (unsigned)varname_)

#define print_json5_double_field(base_, varname_) print_json5_double_(#varname_, (base_)->varname_)
#define print_json5_string_field(base_, varname_) print_json5_string_(#varname_, (base_)->varname_ ?:"NULL")
#define print_json5_int_field(base_, varname_) print_json5_int_(#varname_, (base_)->varname_)
#define print_json5_unsigned_field(base_, varname_) print_json5_unsigned_(#varname_, (base_)->varname_)
#define print_json5_long_field(base_, varname_) print_json5_long_(#varname_, (base_)->varname_)

#define print_json5_PmCartesian(varname_) print_json5_PmCartesian_(#varname_, varname_)
#define print_json5_EmcPose(varname_) print_json5_EmcPose_(#varname_, varname_)
#define print_json5_PmCartLine(varname_) print_json5_PmCartLine_( #varname_, varname_)
#define print_json5_PmCircle(varname_) print_json5_PmCircle_(#varname_, varname_)

#define print_json5_PmCartesian_field(base_, varname_) print_json5_PmCartesian_(#varname_, (base_)->varname_)
#define print_json5_EmcPose_field(base_, varname_) print_json5_EmcPose_(#varname_, (base_)->varname_)
#define print_json5_PmCartLine_field(base_, varname_) print_json5_PmCartLine_( #varname_, (base_)->varname_)
#define print_json5_PmCircle_field(base_, varname_) print_json5_PmCircle_(#varname_, (base_)->varname_)

// Hacky way to do realtime-logging of
// KLUDGE shove these in the header to avoid linking issues

static inline void print_json5_start_()
{
    rtapi_print("{");
}

static inline void print_json5_end_()
{
    rtapi_print("}\n");
}

static inline void print_json5_fieldname_(const char *fname)
{
    rtapi_print("%s: ", fname ?: "NULL");
}

static inline void print_json5_array_start_(const char *arr_name)
{
    if (arr_name) {
        rtapi_print("%s: [", arr_name);
    } else {

        rtapi_print("[");
    }
}

static inline void print_json5_array_end_()
{
    rtapi_print("]\n");
}

static inline void print_json5_object_start_(const char *fname)
{
    if (fname) {
        rtapi_print("%s: {", fname);
    } else {
        rtapi_print("{");
    }
}

static inline void print_json5_object_end_()
{
    rtapi_print("}, ");
}

static inline void print_json5_double_(const char *varname, double value)
{
    rtapi_print("%s: %0.17g, ", varname ?: "NO_NAME", value);
}

static inline void print_json5_bool_(const char *varname, double value)
{
    rtapi_print("%s: %s, ", varname ?: "NO_NAME", value ? "true" : "false");
}

static inline void print_json5_int_(const char *varname, int value)
{
    rtapi_print("%s: %d, ", varname ?: "NO_NAME", value);
}

static inline void print_json5_long_(const char *varname, long value)
{
    rtapi_print("%s: %ld, ", varname ?: "NO_NAME", value);
}

static inline void print_json5_ll_(const char *varname, long long value)
{
    rtapi_print("%s: %lld, ", varname ?: "NO_NAME", value);
}

static inline void print_json5_unsigned_(const char *varname, unsigned value)
{
    rtapi_print("%s: %u, ", varname ?: "NO_NAME", value);
}

static inline void print_json5_string_(const char *field_name, const char *value)
{
    // TODO handle nulls gracefully with proper JSON null
    rtapi_print("%s: \"%s\", ", field_name ?: "NULL", value ?: "NULL");
}

static inline void print_json5_PmCartesian_(const char *varname, PmCartesian value)
{

    rtapi_print("%s: [%0.17g, %0.17g, %0.17g], ", varname ?: "NO_NAME", value.x, value.y, value.z);
}

static inline void print_json5_EmcPose_(const char *varname, EmcPose value)
{
    rtapi_print("%s: [%0.17g, %0.17g, %0.17g, %0.17g, %0.17g, %0.17g, %0.17g, %0.17g, %0.17g], ",
                    varname,
                    value.tran.x,
                    value.tran.y,
                    value.tran.z,
                    value.a,
                    value.b,
                    value.c,
                    value.u,
                    value.v,
                    value.w);
}

static inline void print_json5_PmCartLine_(const char *name, PmCartLine value)
{
    print_json5_object_start_(name);
    // Need manual field names here
    print_json5_PmCartesian_("start", value.start);
    print_json5_PmCartesian_("end", value.start);
    print_json5_PmCartesian_("uVec", value.uVec);
    print_json5_double_("tmag", value.tmag);
    print_json5_double_("tmag_zero", value.tmag_zero);
    print_json5_object_end_();
}

static inline void print_json5_PmCircle_(const char *name, PmCircle circle)
{
    print_json5_object_start_(name);
    print_json5_PmCartesian_("center", circle.center);
    print_json5_PmCartesian_("normal", circle.normal);
    print_json5_PmCartesian_("rTan", circle.rTan);
    print_json5_PmCartesian_("rPerp", circle.rPerp);
    print_json5_PmCartesian_("rHelix", circle.rHelix);
    print_json5_double_("radius", circle.radius);
    print_json5_double_("angle", circle.angle);
    print_json5_double_("spiral", circle.spiral);
    print_json5_object_end_();
}

static inline void print_json5_SpiralArcLengthFit_(const char *fname, SpiralArcLengthFit fit)
{
    print_json5_object_start_(fname);
    print_json5_double_("b0", fit.b0);
    print_json5_double_("b1", fit.b1);
    print_json5_double_("total_planar_length", fit.total_planar_length);
    print_json5_long_("spiral_in", fit.spiral_in);
    print_json5_object_end_();
}

static inline void print_json5_blend_boundary_t(const char *fname, blend_boundary_t const * const r)
{
    print_json5_object_start_(fname);
    print_json5_PmCartesian_field(r, P1);
    print_json5_PmCartesian_field(r, P2);
    print_json5_PmCartesian_field(r, u1);
    print_json5_PmCartesian_field(r, u2);
    print_json5_double_field(r, s1);
    print_json5_double_field(r, s2);
    print_json5_object_end_();
}

static inline void print_json5_biarc_control_points_t(const char *fname, biarc_control_points_t const * const c)
{
    print_json5_object_start_(fname);
    print_json5_PmCartesian_field(c, Pb1);
    print_json5_PmCartesian_field(c, Pb2);
    print_json5_PmCartesian_field(c, u_mid);
    print_json5_PmCartesian_field(c, P_mid);
    print_json5_double_field(c, d);
    print_json5_object_end_();
}

static inline void print_json5_biarc_solution_errors_t(const char *fname, biarc_solution_errors_t const * const r)
{
    print_json5_object_start_(fname);
    print_json5_double_field(r, deviation);
    print_json5_double_field(r, radius_rel);
    print_json5_double_field(r, radius_abs);
    print_json5_object_end_();
}

static inline void print_json5_biarc_solution_t(const char *fname, biarc_solution_t const * const r)
{
    print_json5_object_start_(fname);
    print_json5_double_field(r, Rb);
    print_json5_double_field(r, R_geom);
    print_json5_double_field(r, arc_len_est);
    print_json5_double_field(r, R_plan);
    print_json5_double_field(r, T_plan);
    print_json5_biarc_solution_errors_t("err", &r->err);
    print_json5_object_end_();
}

static inline void print_json5_SphericalArc(const char *fname, SphericalArc const * const arc) {
    print_json5_object_start_(fname);
    print_json5_PmCartesian_field(arc, start);
    print_json5_PmCartesian_field(arc, end);
    print_json5_PmCartesian_field(arc, center);
    print_json5_PmCartesian_field(arc, rStart);
    print_json5_PmCartesian_field(arc, rEnd);
    print_json5_PmCartesian_field(arc, uTan);
    print_json5_double_field(arc, radius);
    print_json5_double_field(arc, spiral);
    print_json5_double_field(arc, angle);
    print_json5_double_field(arc, Sangle);
    print_json5_double_field(arc, line_length);
    print_json5_object_end_();
}

static inline void print_json5_TC_STRUCT_kinematics(const char *fname, TC_STRUCT const * const tc)
{
    print_json5_object_start_(fname);
    print_json5_double_field(tc, maxvel);
    print_json5_double_field(tc, maxaccel);
    print_json5_double_field(tc, acc_normal_max);
    print_json5_double_field(tc, acc_ratio_tan);
    print_json5_double_field(tc, reqvel);
    print_json5_double_field(tc, target);
    print_json5_int_field(tc, motion_type);
    print_json5_object_end_();
}


#endif // RTAPI_JSON5_H
