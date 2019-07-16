%% Assumes units of encoder pulses
function [p_disc, t_disc, v_est] = sim_encoder_reader(p_real, v_real, a_real, j_real, dt, reader_freq, window_size)
  p_disc = [floor(p_real)];
  t_disc = [0];
  v_est = 0;
  
  count_cutoff = ceil(reader_freq * dt * window_size);
  % Sloppy, lots of repeated work per cycle but ok for simulation on a high-power machine
  for t = 0:-1/reader_freq:-2*dt
    p_est = p_real + v_real * t + a_real/2 * t^2 + j_real/6*t^3;
    p_est_disc = floor(p_est);
    if p_est_disc != p_disc(end)
      p_disc = [p_disc; p_est_disc];
      t_disc = [t_disc; t];
      % Stop searching if we found at least once position delta past one servo tick
      if t <= -dt || length(p_disc) > count_cutoff
        t
        break;
      end
    endif
  endfor
  
  if length(p_disc) > 1
    %T = [ones(size(t_disc)), t_disc, t_disc.^2];
    %theta = (pinv(T'*T))*T'*p_disc;
    
    %v_est = theta(2);
    v_est = (p_disc(1) - p_disc(end)) / (t_disc(1) - t_disc(end));
  endif
  
endfunction
