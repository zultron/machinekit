/**
 * @file motion_shared.h
 *
 * Common declarations for shared objects in motion module.
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2019, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#ifndef MOTION_SHARED_H
#define MOTION_SHARED_H

extern struct emcmot_command_t *emcmotCommand;
extern struct emcmot_status_t *emcmotStatus;
extern struct emcmot_config_t *emcmotConfig;
extern struct emcmot_debug_t *emcmotDebug;
extern struct emcmot_internal_t *emcmotInternal;
extern struct emcmot_error_t *emcmotError;

double get_spindle_speed_out_rpm(struct emcmot_status_t const * const stat);
double get_spindle_command_direction(struct emcmot_status_t const * const stat);
int is_spindle_on(struct emcmot_status_t const * const stat);
#endif // MOTION_SHARED_H
