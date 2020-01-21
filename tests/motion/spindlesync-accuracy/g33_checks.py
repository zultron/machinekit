#!/usr/bin/env python

import sys
import csv
import numpy as np
import json
import os

import re
# String helpers

# KLUDGE copied from motion_types.h
SYNC_SEEK_INDEX = 2
SYNC_ACCEL_RAMP = 3
SYNC_POSITION = 4
SYNC_TRIGGER_WAIT = 6

RIGIDTAP_INACTIVE = 0
RIGIDTAP_TAPPING = 1
RIGIDTAP_REVERSING = 2
RIGIDTAP_RETRACTION = 3
RIGIDTAP_FINAL_REVERSAL = 4
RIGIDTAP_FINAL_PLACEMENT = 5

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
    
    with open('defaults.json'.format(test_name), 'r') as f:
        c= json.load(f)
    
    with open('test_case_{}.json'.format(test_name), 'r') as f:
        c.update(json.load(f))

    return c


def load_samples_after_sync(samples_file):
    samples=[]
    with open(samples_file, 'r') as csvfile:
        sample_reader = csv.reader(csvfile, delimiter=' ', quoting=csv.QUOTE_NONNUMERIC)

        for row in sample_reader:
            samples.append([x for x in row if not isinstance(x, str)])

    # KLUDGE throw out the last sampled point in case it's clobbered when halsampler was killed
    return np.array(samples[:-1])


def check_all(test_name):
    constants = load_test_case(test_name)

    samples = load_samples_after_sync(get_sample_file_name(test_name))

    if abs(constants["tap_pitch_scale"] - 1.0) > 0.0001:
        exceeds_abs_dev = check_abs_deviation(samples, constants, (RIGIDTAP_RETRACTION,))
    else:
        exceeds_abs_dev = check_abs_deviation(samples, constants)

    # Copied from test INI file
    z_max_accel = 50.0
    spindle_max_accel_rps2 = 100.0
    z_feed = constants["pitch"] * constants["spindle_speed_rps"]
    if constants["z_pecks"] > 1:
        z_retract_decel_distance = constants["pitch"] * constants["spindle_speed_rps"] ** 2. / (2.0 * spindle_max_accel_rps2)
    else:
        z_retract_decel_distance = z_feed ** 2. / (2.0 * z_max_accel)
    z_endpt = constants["z_depth"]

    target_allowance = constants["overrun_allowance_rev"] * constants["pitch"]
    retract_overshoot_tolerance = target_allowance + z_retract_decel_distance
    threading_overshoot_tolerance = target_allowance
    exceeds_overrun = check_overrun(samples[:, 4], z_endpt, threading_overshoot_tolerance, retract_overshoot_tolerance)

    return exceeds_abs_dev or exceeds_overrun


def find_transitions(states, s0, s1):
    return np.where((states == s1) * (np.roll(states, 1) == s0))[0]


def find_rising_edge(states, s):
    return np.where((states == s) * (np.roll(states, 1) != s))[0]


def find_falling_edge(states, s):
    return np.where((states != s) * (np.roll(states, 1) == s))[0]


def find_state(states, s):
    return np.where(states == s)[0]


def find_any_of_state(states, s_arr):
    res = []
    for s in s_arr:
        res.extend(np.where(states == s)[0])
    return np.unique(res)


def check_abs_deviation(samples, constants, rigid_tapping_phases=None):
    spindle_pos = samples[:, 1] * get_spindle_direction(constants)
    z_axis_pos = samples[:, 4]

    z_axis_nominal_feed = constants["pitch"] * constants["spindle_speed_rps"]
    # Find when sync starts in rigid tap state
    index_seek_start_idx = find_rising_edge(samples[:, 6], SYNC_SEEK_INDEX)
    sync_start_idx = find_rising_edge(samples[:, 6], SYNC_ACCEL_RAMP)
    sync_end_idx = find_falling_edge(samples[:, 6], SYNC_ACCEL_RAMP)

    for s, e in zip(sync_start_idx, sync_end_idx):
        sync_pos = samples[e, 4]
        endpoint = constants["z_depth"]
        if sync_pos < endpoint:
            print >> sys.stderr, "Failed: position sync starts at position %f, after endpoint %f)" % (sync_pos, endpoint)
            return True
        print >> sys.stderr, "Passed: position sync starts at position %f, before endpoint %f)" % (sync_pos, endpoint)

    max_sync_delay = constants.get("max_sync_delay", 1)
    for e, s in zip(index_seek_start_idx, sync_start_idx):
        if e-s > max_sync_delay:
            print >> sys.stderr, "Failed: position sync started at %d, not within %d steps of expected (%d)" % (
                e, max_sync_delay, s)
            return True

    print >> sys.stderr, "Passed: position sync started at %d, within %d steps of expected (%d)" % (
        e, max_sync_delay, s)

    pos_sync_active_range = find_state(samples[:, 6], SYNC_POSITION)
    if rigid_tapping_phases is not None:
        rigid_tap_state_range = find_any_of_state(samples[:, 8], rigid_tapping_phases)
        pos_sync_active_range = np.intersect1d(pos_sync_active_range, rigid_tap_state_range)
    spindle_pos_active = spindle_pos[pos_sync_active_range]
    z_axis_pos_active = z_axis_pos[pos_sync_active_range]
    print >> sys.stderr, np.size(pos_sync_active_range)

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


def check_overrun(axis_pos, nominal_endpt, max_threading_overshoot, max_retract_overshoot):
    min_pos = np.min(np.array(axis_pos))
    max_pos = np.max(np.array(axis_pos))

    print >> sys.stderr, "min axis position: ", min_pos
    print >> sys.stderr, "max axis position: ", max_pos
    allowed_min = nominal_endpt - max_threading_overshoot
    if min_pos < allowed_min:
        print >> sys.stderr, "Failed: threading overrun detected at bottom of hole, min_pos {} exceeds limit of {}".format(min_pos, allowed_min)
        return True
    else:
        print >> sys.stderr, "Passed: threading overrun {} within limit of {}".format(min_pos, allowed_min)

    if max_pos > max_retract_overshoot:
        print >> sys.stderr, "Failed: retract overrun detected at retract, max_pos {} exceeds limit of {}".format(max_pos, max_retract_overshoot)
        return True
    else:
        print >> sys.stderr, "Passed: retract overrun {} within limit of {}".format(max_pos, max_retract_overshoot)

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

