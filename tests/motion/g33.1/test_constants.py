spindle_speed_rps = 10.0  # revolutions per second
spindle_speed_rpm = spindle_speed_rps * 60.0
pitch = 0.05   # in
z_dist = 1.0  # in
z_axis_accel = 50.0  # IPS^2
z_axis_feed = pitch * spindle_speed_rps
servo_loop_freq = 1000.0
max_allowed_tracking_error = 0.001