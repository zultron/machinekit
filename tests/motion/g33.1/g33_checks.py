#!/usr/bin/env python

import sys
import csv
import numpy as np
import json
import os

import re
# String helpers


def get_spindle_direction(constants):
    return -1.0 if 'ccw' == constants.get('spindle_direction', 'cw') else 1.0


def get_spindle_mcode(constants):
    return 'M4' if get_spindle_direction(constants) < 0 else 'M3'


def get_sample_file_name(test_name):
    return 'test_case_{}.samples'.format(test_name)


def get_test_name_from_case_file(test_file_name):
    m = re.match(r'^test_case_(.+)\.json', test_file_name)
    if m is not None:
        return m.groups()[0]

    return None


def load_test_case(test_name):
    with open('test_case_{}.json'.format(test_name), 'r') as f:
        return json.load(f)


def load_samples_after_sync(samples_file):
    samples=[]
    with open(samples_file, 'r') as csvfile:
        sample_reader = csv.reader(csvfile, delimiter=' ', quoting=csv.QUOTE_NONNUMERIC)

        for row in sample_reader:
            samples.append([x for x in row if not isinstance(x, str)])

    return np.array(samples)


def check_all(test_name):
    constants = load_test_case(test_name)

    samples = load_samples_after_sync(get_sample_file_name(test_name))

    exceeds_abs_dev = check_abs_deviation(samples, constants)

    z_endpt = -constants["z_distance"]
    exceeds_overrun = check_overrun(samples[:, 4], z_endpt, 10. * constants["pitch"])

    return exceeds_abs_dev or exceeds_overrun


def check_abs_deviation(samples, constants):
    spindle_pos = samples[:, 1] * get_spindle_direction(constants)
    z_axis_pos = samples[:, 4]

    # Find the last point where index-enable is high (it's the next one
    spindle_start = np.argwhere(samples[:, 7] == 1)[-1].item() + 1

    z_axis_nominal_feed = constants["pitch"] * constants["spindle_speed_rps"]
    sync_offset_expected = int(z_axis_nominal_feed / constants["z_axis_max_accel"] * constants["servo_loop_freq"])
    sync_idx_expected = spindle_start + sync_offset_expected
    # Find when sync starts in rigid tap state
    pos_sync_active_range = np.argwhere(samples[:, 6] == 2)
    sync_idx = pos_sync_active_range[0].item()
    end_sync_idx = pos_sync_active_range[-1].item()

    sync_pos = samples[sync_idx, 4]
    endpoint = -constants["z_distance"]
    if sync_pos < endpoint:
        print >> sys.stderr, "Failed: position sync starts at position %f, after the endpoint %f)" % (sync_pos, endpoint)
        return True

    max_sync_delay = constants.get("max_sync_delay", 1)
    if abs(sync_idx - sync_idx_expected) > max_sync_delay:
        print >> sys.stderr, "Failed: position sync started at %d), not within %d steps of expected (%d)" % (
            sync_idx, max_sync_delay, sync_idx_expected)
        return True

    print >> sys.stderr, "Passed: spindle sync starts within expected limits"

    spindle_pos_active = spindle_pos[sync_idx:end_sync_idx]
    z_axis_pos_active = z_axis_pos[sync_idx:end_sync_idx]

    p_bestfit = np.polyfit(spindle_pos_active, z_axis_pos_active, 1)
    print >> sys.stderr, "Linear fit for spindle / axis motion: ", np.poly1d(p_bestfit)

    # Force the slope to match the pitch to check for effective deviation from the ideal path
    p_from_pitch = p_bestfit
    p_from_pitch[0] = -constants["pitch"]

    p = np.poly1d(p_from_pitch)

    z_axis_expected = p(spindle_pos_active)

    max_abs_dev = np.max(np.abs(z_axis_expected - z_axis_pos_active))

    deviation_failure = max_abs_dev > constants["max_tracking_error"] and not constants.get("expect_deviation_violation", False)

    print >> sys.stderr, "{}: Maximum position deviation is: {} ".format("Passed" if not deviation_failure else "Failed", max_abs_dev)
    return deviation_failure


def check_overrun(axis_pos, nominal_endpt, allowed_overshoot):
    min_pos = np.min(np.array(axis_pos))
    max_pos = np.max(np.array(axis_pos))

    print >> sys.stderr, "min axis position: ", min_pos
    print >> sys.stderr, "max axis position: ", max_pos

    if min_pos < nominal_endpt - allowed_overshoot:
        print >> sys.stderr, "Failed: Overrun detected at bottom of hole"
        return True

    if max_pos > allowed_overshoot:
        print >> sys.stderr, "Failed: Overrun detected at retract"
        return True

    print >> sys.stderr, "Passed: no overrun detected"
    return False


def find_test_cases():
    cases = []
    for _, _, files in os.walk('.'):
        for f in files:
            t = get_test_name_from_case_file(f)
            if t is not None:
                cases.append(t)
    return cases


if __name__ == '__main__':

    for test_name in find_test_cases():
        sf = get_sample_file_name(test_name)
        print >> sys.stderr, "Checking results of test", test_name
        if not os.path.exists(sf):
            print >> sys.stderr, "No samples found for", test_name
            continue
        exceeds_limits = check_all(test_name)
        if exceeds_limits:
            sys.exit(1)



