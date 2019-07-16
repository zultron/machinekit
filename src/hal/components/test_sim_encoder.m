## Copyright (C) 2019 Robert W Ellenberg
## 
## This program is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see
## <https://www.gnu.org/licenses/>.

## Author: Robert W Ellenberg <rellenberg@rellenberg-desktop>
## Created: 2019-05-24

encoder_reader_freq = 1e5 %Hz
encoder_resolution = 2048*4 %counts / revolution

dt = 0.001 %sec

% Assume sinusoidal variation around a constant speed
omega_beat = 10 * 2*pi
spindle_speed_cps = 500 / 60 * 2 * pi * encoder_resolution
amplitude_counts = spindle_speed_cps / omega_beat;

% One seconds worth of data
vel_est = [];
t_vec = 0:dt:(2*pi/omega_beat*1.5);
pos_vec = amplitude_counts * sin(omega_beat * t_vec) + spindle_speed_cps * t_vec;
vel_vec = omega_beat * amplitude_counts * cos(omega_beat * t_vec) + spindle_speed_cps;
acc_vec = -omega_beat^2 * amplitude_counts * sin(omega_beat * t_vec);
jerk_vec = -omega_beat^3 * amplitude_counts * cos(omega_beat * t_vec);

v_naive = [diff(pos_vec),0]/dt;

for k = 1:length(t_vec)
  t=t_vec(k);
  t_last = t - dt;
  pos = pos_vec(k);
  vel = vel_vec(k);
  acc = acc_vec(k);
  jerk = jerk_vec(k);
  
  [p_disc, t_disc, v_est_new] = sim_encoder_reader(pos, vel, 0, 0, dt, encoder_reader_freq, 0.25);
  
  vel_est = [vel_est, v_est_new];
endfor

figure(1)
clf
hold on
plot(t_vec, [vel_vec/max(vel_vec);
             vel_est./max(vel_vec);
             v_naive./max(vel_vec);
             (vel_vec - vel_est) ./ max(vel_vec);
             (vel_vec - v_naive) ./ max(vel_vec)]);
legend('actual vel', 'estimated', 'naive', 'est error', 'naive error');

grid on
max(abs((vel_vec - vel_est) ./ vel_vec))'
hold off
ax = axis();
ax([3,4])=[-0.2,1.2];
axis(ax);