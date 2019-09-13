#!/usr/bin/env python3
import pyjson5
import json

import sys
import re
from matplotlib import pyplot as plt
import numpy as np

axes = [
    'X',
    'Y',
    'Z',
    'A',
    'B',
    'C',
    'U',
    'V',
    'W',
]


def extract_field1(logs, log_entry, field1):
    return np.array([e.get(field1, None) for e in logs if e.get('log_entry', None) == log_entry])


def extract_field2(logs, log_entry, field1, field2):
    return np.array([e.get(field1, {}).get(field2, None) for e in logs if e.get('log_entry', None) == log_entry])


def extract_field1_all(logs, field1):
    return np.array([f for f in (e.get(field1, None) for e in logs) if f is not None])


def extract_field2_all(logs, field1, field2):
    return np.array([f for f in (e.get(field1, {}).get(field2, None) for e in logs) if f is not None])


def parse_log(logname):
    logs = []
    json_str=[]
    with open(logname, 'r') as log_file:
        for l in log_file.readlines():
            # Try to filter out obviously non-JSON lines
            if re.search('{', l) is None or re.search('}', l) is None:
                continue
            try:
                logs.append(pyjson5.decode(l))
                json_str.append(pyjson5.encode(logs[-1]))
            except Exception as e:
                print('Ignoring exception ', e)
    with open(logname + '.txt', 'w') as json_out:
        json_out.writelines('\n'.join(json_str))
    return logs

def plot_biarc(logs_raw):
    logs = [l for l in logs_raw if l.get('log_entry', None) == 'tpCreateBiarcBlend']

    blend_id = extract_field1_all(logs, 'id')
    blend1_reqvel = extract_field2_all(logs, 'blend1', 'reqvel')
    blend2_reqvel = extract_field2_all(logs, 'blend2', 'reqvel')
    segment_reqvel = extract_field1_all(logs, 'reqvel')

    blend1_size = extract_field2_all(logs, 'blend1', 'target')
    blend2_size = extract_field2_all(logs, 'blend2','target')
    blend1_type = extract_field2_all(logs, 'blend1','motion_type')
    blend2_type = extract_field2_all(logs, 'blend2','motion_type')
    blend1_maxvel = extract_field2_all(logs, 'blend1','maxvel')
    blend2_maxvel = extract_field2_all(logs, 'blend2','maxvel')
    segment_maxvel = extract_field1_all(logs, 'maxvel')
    consume = extract_field1_all(logs, 'consume')

    p1_diff = extract_field1_all(logs, 'p1_diff')
    p_mid_diff = extract_field1_all(logs, 'p_mid_diff')
    p2_diff = extract_field1_all(logs, 'p2_diff')
    param_line_length = extract_field1_all(logs, 'param_line_length')
    for d in zip(blend_id, p1_diff, p_mid_diff, p2_diff, consume, param_line_length):
        if (d[1] != 0).any() or (d[2] != 0).any() or (d[3] != 0).any():
            print(d)

    if len(blend1_size) and len(blend2_size):
        plt.figure(5)
        plt.plot(blend_id, blend1_size, label='blend1')
        plt.plot(blend_id, blend2_size, label='blend2')
        plt.xlabel('Segment ID')
        plt.grid(True)
        plt.legend()
        plt.title('Blend segment sizes')

    if len(blend1_type) and len(blend2_type):
        plt.figure(6)
        plt.plot(blend_id, blend1_type, label='blend1')
        plt.plot(blend_id, blend2_type, label='blend2')
        plt.xlabel('Segment ID')
        plt.grid(True)
        plt.legend()
        plt.title('Blend segment types')

    if len(blend1_maxvel) and len(blend2_maxvel):
        plt.figure(7)
        plt.plot(blend_id, blend1_maxvel, label='blend1_max')
        plt.plot(blend_id, blend2_maxvel, label='blend2_max')
        plt.plot(blend_id, segment_maxvel, label='maxvel_segments')
        plt.plot(blend_id, blend1_reqvel, label='blend1_req')
        plt.plot(blend_id, blend2_reqvel, label='blend2_req')
        plt.plot(blend_id, segment_reqvel, label='reqvel_segments')
        plt.xlabel('Segment ID')
        plt.grid(True)
        plt.legend()
        plt.title('Blend segment maxvels')


def plot_axis(logs_raw):
    logs = [l for l in logs_raw if l.get('log_entry', None) == 'tpRunCycle']
    axis_pos = extract_field1_all(logs, 'axis_pos')
    axis_vel = extract_field1_all(logs, 'axis_vel')
    axis_accel = extract_field1_all(logs, 'axis_accel')
    times = extract_field1_all(logs, 'time')

    np.savetxt('axis_pos.csv', axis_pos, delimiter=',')
    np.savetxt('axis_vel.csv', axis_vel, delimiter=',')
    np.savetxt('axis_accel.csv', axis_accel, delimiter=',')

    plt.close()

    if len(axis_pos):
        plt.figure(1)
        for idx in range(4):
            plt.plot(times * 1000.0, axis_pos[:, idx], label=f'{axes[idx]} position')
        plt.grid(True)
        plt.legend()
        plt.xlabel('time, ms')
        plt.title('Axis position vs time, ms')

    if len(axis_vel):
        plt.figure(2)
        for idx in range(4):
            plt.plot(times * 1000.0, axis_vel[:, idx], label=f'{axes[idx]} velocity')
        plt.grid(True)
        plt.legend()
        plt.xlabel('time, ms')
        plt.title('Axis velocity vs time, ms')

    if len(axis_accel):
        plt.figure(3)
        for idx in range(4):
            plt.plot(times * 1000.0, axis_accel[:, idx], label=f'{axes[idx]} acceleration')
        plt.grid(True)
        plt.legend()
        plt.xlabel('time, ms')
        plt.title('Axis acceleration vs time, ms')

    if len(axis_vel):
        xyz_vel = np.sqrt(np.sum(np.square(axis_vel[:, 0:2]), 1))
        nz_ind = np.where(xyz_vel>=0.000001)[0]
        times_ms = (times - times[nz_ind[0]]) * 1000.0;
        print("total run time = {} ms".format(times_ms[nz_ind[-1]]))
        plt.figure(4)
        plt.plot(times_ms, xyz_vel)
        plt.grid(True)
        plt.xlabel('time, ms')
        plt.title('XYZ velocity vs relative time, ms')


def plot_activations(logs_raw):
    logs = [l for l in logs_raw if l.get('log_entry', None) == 'tpActivateSegment']
    segment_id = extract_field1_all(logs, 'id')
    target_vel = extract_field1_all(logs, 'target_vel')
    finalvel = extract_field1_all(logs, 'finalvel')
    kink_vel = extract_field1_all(logs, 'kink_vel')
    parabolic_equiv_vel = extract_field1_all(logs, 'parabolic_equiv_vel')
    motion_type = extract_field1_all(logs, 'motion_type')
    blend_mode = extract_field1_all(logs, 'blend_mode')
    tp_position_error = extract_field1_all(logs, 'tp_position_error')

    if len(segment_id):
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

    tp_position_error = extract_field1(logs, 'tpActivateSegment', 'tp_position_error')

    if len(segment_id):
        plt.figure(9)
        plt.plot(segment_id, tp_position_error[:, 0], label='X position error')
        plt.plot(segment_id, tp_position_error[:, 1], label='Y position error')
        plt.plot(segment_id, tp_position_error[:, 2], label='Z position error')

        plt.xlabel('Segment ID')
        plt.grid(True)
        plt.legend()
        plt.title('Position error')


def plot_all(logfile):
    logs = parse_log(logfile)
    plot_axis(logs)
    plot_activations(logs)
    plot_biarc(logs)
    plt.show()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(sys.stderr, 'No logfile specified')
        exit(1)
    logs = parse_log(sys.argv[1])
    plot_axis(logs)
    plot_biarc(logs)
    plot_activations(logs)
    plt.show()


