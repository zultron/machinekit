#!/usr/bin/env python

import sys
import csv
import numpy as np
import json
import os


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
            break;

        for row in sample_reader:
            samples.append([x for x in row if not isinstance(x, str)])

    return np.array(samples)


def check_tapping_accuracy(spindle_pos, axis_pos, sync_row, constants):
    """
    NOTE: assumes that initial position is 0 revs and 0 axis displacement, and that the retract height is 0.
    This ignores any data corresponding to when Z is positive.
    Make sure to subtract away the start position before running this.
    """

    cutoff = constants["max_tracking_error"]
    spindle_pos_with_offset = spindle_pos - spindle_pos[sync_row]
    axis_pos_with_offset = axis_pos - axis_pos[sync_row]

    spindle_pos_with_offset[0:sync_row] = 0
    axis_pos_with_offset[0:sync_row] = 0

    expected_axis_pos = np.array(spindle_pos_with_offset) * -constants["pitch"]
    
    trimmed_actual = np.where(expected_axis_pos <= 0, axis_pos_with_offset, 0)
    trimmed_expected = np.where(expected_axis_pos <= 0, expected_axis_pos, 0)

    abs_deviation = np.abs(trimmed_expected - trimmed_actual)

    log_lines = ['id\tdev\tspindle\tactual\texpected']
    for n, d in enumerate(zip(abs_deviation, spindle_pos_with_offset, axis_pos_with_offset, expected_axis_pos)):
        if d[0] > cutoff:
            log_lines.append('{}\t{:4}\t{:4}\t{:4}\t{:4}'.format(n, *d))

    if len(log_lines) > 1:
        for l in log_lines:
            print >>sys.stderr, l

    return np.max(abs_deviation)


def load_constants(test_name):
    with open('{}_constants.json'.format(test_name), 'r') as f:
        return json.load(f)


def check_all(test_name):
    samples = load_samples_after_sync('{}_samples'.format(test_name))
    constants = load_constants(test_name)

    exceeds_abs_dev = check_abs_deviation(samples, constants)

    z_endpt = -constants["z_distance"]
    exceeds_overrun = check_overrun(samples[:, 4], z_endpt, 10. * constants["pitch"])

    return exceeds_abs_dev or exceeds_overrun


def check_abs_deviation(samples, constants):
    spindle_pos = samples[:, 1]
    z_axis_pos = samples[:, 4]

    z_axis_nominal_feed = constants["pitch"] * constants["spindle_speed_rps"]
    sync_offset_est = int(z_axis_nominal_feed / constants["z_axis_max_accel"] * constants["servo_loop_freq"]) + 100
    # Find when sync starts in rigid tap state
    sync_offset = np.argwhere(samples[:, 6] == 2)[0].item()
    start_row = np.argwhere(spindle_pos > 0)[0].item() + sync_offset
    print >> sys.stderr, "Expected sync offset at ", sync_offset, " actual sync offset at ", sync_offset_est
    if abs(sync_offset-sync_offset) > 100:
        print >> sys.stderr, "Failed to sync in time"
        return True

    max_abs_dev = check_tapping_accuracy(spindle_pos, z_axis_pos, start_row, constants)
    print >> sys.stderr, "Got maximum position deviation: ", max_abs_dev
    return max_abs_dev > constants["max_tracking_error"] and not constants.get("expect_deviation_violation", False)


def check_overrun(axis_pos, nominal_endpt, allowed_overshoot):
    min_pos = np.min(np.array(axis_pos))
    max_pos = np.max(np.array(axis_pos))

    print >> sys.stderr, "min axis position: ", min_pos
    print >> sys.stderr, "max axis position: ", max_pos

    if min_pos < nominal_endpt - allowed_overshoot:
        print >> sys.stderr, "Overrun detected at bottom of hole"
        return True

    if max_pos > allowed_overshoot:
        print >> sys.stderr, "Overrun detected at retract"
        return True

    return False


def find_test_cases():
    cases = []
    for _, _, files in os.walk('.'):
        for f in files:
            if '.json' in f:
                cases.append(f.rsplit('_constants.json')[0])
    return cases


if __name__ == '__main__':

    for test_name in find_test_cases():
        print >> sys.stderr, "Checking results of test ", test_name
        exceeds_limits = check_all(test_name)
        if exceeds_limits:
            sys.exit(1)



