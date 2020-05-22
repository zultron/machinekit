/**
 * @file emccanon.hh
 *
 * Declarations for some internal functions in emccanon
 *
 * @author Robert W. Ellenberg <rwe24g@gmail.com>
 *
 * @copyright Copyright 2020, Robert W. Ellenberg
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License (V2) as published by the Free Software Foundation.
 */

#ifndef EMCCANON_HH
#define EMCCANON_HH

#include <positionarray.hh>
#include <StateTag.hh>

bool linkable(PositionArray const &goal_pos_canon);

void see_segment(
    StateTag const &tag,
    PositionArray goal_pos_canon);

double distance_to_segment(
    PositionArray const & P0,
    PositionArray const & P1,
    PositionArray const & R);

double get_naivecam_mm_per_deg();
#endif // EMCCANON_HH
