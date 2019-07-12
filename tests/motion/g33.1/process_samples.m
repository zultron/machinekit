sample_files = glob('*_samples');

close all
for k = 1:length(sample_files)
  prefix = sample_files{k};
  % Poor-mans rstrip
  prefix = prefix(1:end-8)
  d=importdata([prefix, '_samples']);
  constants = loadjson([prefix, '_constants.json']);

  z_feed_nominal = constants.spindle_speed_rps * constants.pitch;

  idx = 1;

  spindle_dpos=diff(d(:,2));
  idx=find(spindle_dpos < -1,1);
  est_sync_point = -z_feed_nominal / constants.z_axis_max_accel * 0.5;
  idx=find(d(:,7) > 1, 1);
  idx_end=find(d(:,2) > d(idx,2),1,'last');


  t_sync=d(idx:idx_end, 1);
  spindle_pos = d(idx:idx_end,2);
  z_pos = d(idx:idx_end,5);

  % figure(2)
  % plot(t_sync, spindle_pos, t_sync, z_pos)
  figure()

  % Count how many data points we have
  m = length(spindle_pos);
  % Add a column of all ones (intercept term) to x
  X = [ones(size(spindle_pos)) spindle_pos];
  theta = (pinv(X'*X))*X'*z_pos;

  z_fit = theta(1) + theta(2)*spindle_pos;

  meas_err = z_pos-z_fit;
  motion_err = d(idx:idx_end, 6);
  plot(t_sync, z_pos-z_fit,t_sync, motion_err, '--');
  title(sprintf('Spindle tracking error estimates - %s', prefix),'interpreter','none')
  xlabel('Time, ms')
  ylabel('Deviation, in')
  legend('deviation from linear fit','motion.pos-tracking-error')
  grid on
end