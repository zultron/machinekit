#!/usr/bin/env python3
import pyjson5

import sys
import re
from matplotlib import pyplot as plt
import numpy as np

axes={
    'X': 0,
    'Y': 1,
    'Z': 2,
    'A': 3,
    'B': 4,
    'C': 5,
    'U': 6,
    'V': 7,
    'W': 8,
}

def extract_field1(logs, log_entry, field1):
    return np.array([e.get(field1, None) for e in logs if e.get('log_entry', None) == log_entry])


def extract_field2(logs, log_entry, field1, field2):
    return np.array([e.get(field1, {}).get(field2, None) for e in logs if e.get('log_entry', None) == log_entry])


def parse_log(logname):
    logs = []
    with open(logname, 'r') as log_file:
        for l in log_file.readlines():
            # Try to filter out obviously non-JSON lines
            if re.search('{', l) is None or re.search('}', l) is None:
                continue
            try:
                logs.append(pyjson5.decode(l))
            except Exception as e:
                print('Ignoring exception ', e)

    blend1_size = np.array([e['blend1']['target'] for e in logs if e.get('log_entry', None) == 'tpCreateBiarcBlend'])
    blend2_size = np.array([e['blend2']['target'] for e in logs if e.get('log_entry', None) == 'tpCreateBiarcBlend'])

    axis_pos = np.array([e.get('axis_pos', None) for e in logs if e.get('log_entry', None) == 'tpRunCycle'])
    axis_vel = np.array([e.get('axis_vel', None) for e in logs if e.get('log_entry', None) == 'tpRunCycle'])
    axis_accel = np.array([e.get('axis_accel', None) for e in logs if e.get('log_entry', None) == 'tpRunCycle'])
    times = np.array([e.get('time', None) for e in logs if e.get('log_entry', None) == 'tpRunCycle'])

    np.savetxt('axis_pos.csv', axis_pos, delimiter=',')
    np.savetxt('axis_vel.csv', axis_vel, delimiter=',')
    np.savetxt('axis_accel.csv', axis_accel, delimiter=',')

    plt_axis = 'X'
    idx = axes[plt_axis]
    plt.close()


    if len(axis_pos):
        plt.figure(1)
        plt.plot(times * 1000.0, axis_pos[:, idx])
        plt.grid(True)
        plt.title('{} Axis position vs time, ms'.format(plt_axis))

    if len(axis_vel):
        plt.figure(2)
        plt.plot(times * 1000.0, axis_vel[:, idx])
        plt.grid(True)
        plt.title('{} velocity vs time, ms'.format(plt_axis))

    if len(axis_accel):
        plt.figure(3)
        plt.plot(times * 1000.0, axis_accel[:, idx])
        plt.grid(True)
        plt.title('{} Axis acceleration vs time, ms'.format(plt_axis))

    if len(axis_vel):
        xyz_vel = np.sqrt(np.sum(np.square(axis_vel[:, 0:2]), 1))
        nz_ind = np.where(xyz_vel>=0.000001)[0]
        times_ms = (times - times[nz_ind[0]]) * 1000.0;
        print("total run time = {} ms".format(times_ms[nz_ind[-1]]))
        plt.figure(4)
        plt.plot(times_ms, xyz_vel)
        plt.grid(True)
        plt.title('XYZ velocity vs relative time, ms')

    if len(blend1_size) and len(blend2_size):
        plt.figure(5)
        print(blend2_size)
        plt.plot(blend1_size, label='blend1')
        plt.plot(blend2_size, label='blend2')
        plt.xlabel('Segment index')
        plt.grid(True)
        plt.legend()
        plt.title('Blend segment sizes')
    segment_id = extract_field1(logs, 'tpActivateSegment', 'id')
    target_vel = extract_field1(logs, 'tpActivateSegment', 'target_vel')
    finalvel = extract_field1(logs, 'tpActivateSegment', 'finalvel')
    kink_vel = extract_field1(logs, 'tpActivateSegment', 'kink_vel')
    parabolic_equiv_vel = extract_field1(logs, 'tpActivateSegment', 'parabolic_equiv_vel')
    motion_type = extract_field1(logs, 'tpActivateSegment', 'motion_type')
    blend_mode = extract_field1(logs, 'tpActivateSegment', 'blend_mode')


    plt.figure(8)
    plt.plot(segment_id, target_vel, label='target_vel')
    plt.plot(segment_id, finalvel, label='finalvel')

    nonblend_seg_ids = np.where(motion_type!=4)[0]

    plt.plot(segment_id[nonblend_seg_ids], kink_vel[nonblend_seg_ids], '*',label='kink_vel')
    plt.plot(segment_id[nonblend_seg_ids], parabolic_equiv_vel[nonblend_seg_ids], '+',label='parabolic_equiv_vel')
    plt.plot(segment_id[nonblend_seg_ids], blend_mode[nonblend_seg_ids], '.',label='blend_mode')
    plt.xlabel('Segment ID')
    plt.grid(True)
    plt.legend()
    plt.title('Final optimization results for velocities at segment activation')

    plt.show()


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(sys.stderr, 'No logfile specified')
        exit(1)
    parse_logs(sys.argv[1])


