% Problem 4: Plot CDF of delay against multiple retry counts.
clean all; close all;

K = 6
f = cell(1, K);
figure; hold on;
for k = 0:1:(K-1)
    fname = sprintf('swifi_delay_uplink_%d.dat', k);
    if ~exist(fname)
        system(sprintf('ns swifi.tcl delay uplink %d', k));
    y = load(fname);
    [f{k}, x{k}] = ecdf(y);
    plot(x{k}, f{k});
end
print('swifi_delay_cdf.eps');
