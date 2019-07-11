#!/usr/bin/env python

import sys
import csv
import numpy as np
from test_constants import *

def load_samples_after_sync(samples_file):
    samples=[]
    with open(samples_file, 'r') as csvfile:
        sample_reader = csv.reader(csvfile, delimiter=' ', quoting=csv.QUOTE_NONNUMERIC)
        start_pos_check = -1.0;
        for row in sample_reader:
            if start_pos_check is not None and row[1] >= start_pos_check:
                start_pos_check = row[1] - 0.001 # Avoid drop-out due to coarse encoders
                samples.append([0 for x in row if not isinstance(x, str)])
                continue

            start_pos_check = None
            break;


        for row in sample_reader:
            samples.append([x for x in row if not isinstance(x, str)])

    return np.array(samples)


def check_tapping_accuracy(spindle_pos, axis_pos, expected_pitch, sync_row, cutoff):
    """
    NOTE: assumes that initial position is 0 revs and 0 axis displacement, and that the retract height is 0 (ignore data if Z is positive)
    Make sure to subtract away the start position before running this.
    """
    
    spindle_pos_with_offset = spindle_pos - spindle_pos[sync_row]
    axis_pos_with_offset = axis_pos - axis_pos[sync_row]

    spindle_pos_with_offset[0:sync_row] = 0
    axis_pos_with_offset[0:sync_row] = 0

    expected_axis_pos = np.array(spindle_pos_with_offset) * -expected_pitch
    
    trimmed_actual = np.where(expected_axis_pos <= 0, axis_pos_with_offset, 0)
    trimmed_expected = np.where(expected_axis_pos <= 0, expected_axis_pos, 0)

    abs_deviation = np.abs(trimmed_expected - trimmed_actual)

    print >> sys.stderr, 'id\tdev\tspindle\tactual\texpected'
    for n,d in enumerate(zip(abs_deviation, spindle_pos_with_offset, axis_pos_with_offset, expected_axis_pos)):
        if d[0] > cutoff:
            print >> sys.stderr, '{}\t{:4}\t{:4}\t{:4}\t{:4}'.format(n, *d)

    return np.max(abs_deviation)


def do_check():
    samples = load_samples_after_sync('samples')

    spindle_pos = samples[:, 1]
    z_axis_pos = samples[:, 4]

    sync_offset = int(z_axis_feed / z_axis_accel * servo_loop_freq) + 100
    start_row = np.argwhere(spindle_pos > 0)[0].item() + sync_offset

    max_abs_dev = check_tapping_accuracy(spindle_pos, z_axis_pos, pitch, start_row, max_allowed_tracking_error)
    print >> sys.stderr, "Got maximum position deviation: ", max_abs_dev
    return max_abs_dev > max_allowed_tracking_error


if __name__ == '__main__':
    sys.exit(do_check())

