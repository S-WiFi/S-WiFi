% Problem 4: Plot CDF of delay against multiple retry counts.
clear; close all;

K = 1;
figure; hold on;
for k = 0:1:(K-1)
    fname = sprintf('swifi_delay_uplink_%d.dat', k);
    if ~exist(fname, 'file')
        system(sprintf('ns swifi.tcl delay uplink %d', k));
    else
        y = load(fname);
        [f(k+1,:), x(k+1,:)] = ecdf(y);
        %plot(x(k+1,:), f(k+1,:));
    end
end
createfigure_P4(x,f);
