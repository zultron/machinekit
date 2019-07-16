sample_files = glob('test_case_*.samples');

close all
for k = 1:length(sample_files)
  prefix = sample_files{k};
  % Poor-mans rstrip
  prefix = prefix(1:end-8)
  d=importdata([prefix, '.samples']);
  constants = loadjson([prefix, '.json']);

  z_feed_nominal = constants.spindle_speed_rps * constants.pitch;
  
  idx=find(d(:,7) == 2, 1, 'first');
  idx_end=find(d(:,7) == 2,1,'last');

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

  if isfield(constants, 'spindle_direction') && constants.spindle_direction == 'ccw'
    spindle_dir = -1
  else
    constants.spindle_direction = 'cw'
    spindle_dir = 1
  endif
  
  z_fit = theta(1) - constants.pitch * spindle_pos * spindle_dir;

  meas_err = z_pos-z_fit;
  motion_err = d(idx:idx_end, 6);
  
  hold on
  clf
  plot(t_sync, meas_err, t_sync, motion_err, '.');
  title(sprintf('Spindle tracking error pitch %0.3f in, %d RPM. distance %0.3f in, %s',...
     constants.pitch, constants.spindle_speed_rps * 60, constants.z_distance, constants.spindle_direction),'interpreter','none')
  xlabel('Time, ms')
  ylabel('Deviation, in')
  grid on
  line([t_sync(1),t_sync(end)],[1 1]*max(meas_err),'Color','r','linestyle','--')
  line([t_sync(1),t_sync(end)],[1 1]*constants.max_tracking_error,'Color','g','linestyle','--')
  line([t_sync(1),t_sync(end)],[1 1]*min(meas_err),'Color','r','linestyle','--')
  line([t_sync(1),t_sync(end)],[1 1]*-constants.max_tracking_error,'Color','g','linestyle','--')
  legend('deviation from linear fit','motion.pos-tracking-error', 'min / max measured error', 'allowed deviation')
  hold off
end