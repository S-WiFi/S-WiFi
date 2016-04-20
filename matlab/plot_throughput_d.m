% Assume the data file contains the average throughput for each distance.

lut = load('../report/swifi_reliability_uplink.dat');
lut_d = lut(:,1);
lut_p = lut(:,2);
d2p = @(d) lut_p(lut_d == d);
symmetry = {'sym', 'asym'};
str = {'baseline', 'smart'};
style = {'kx-', 'bo-'};
for k1 = 1:2
    figure; box on;
    for k2 = 1:2
        f = sprintf('../throughput_d_%s_%s.dat', symmetry{k1}, str{k2});
        R = load(f);
        d = R(:,1);
        p = arrayfun(d2p, d);
        plot(p, R(:,2), style{k2}, 'LineWidth', 2);
        hold on;
        xlabel('$p$', 'interpreter', 'latex');
        ylabel('$\bar{R}$', 'interpreter', 'latex');
        fig = gcf();
        set(findall(fig, '-property', 'FontSize'), 'FontSize', 16);
    end
    legend(str);
    print('-depsc2', sprintf('throughput_d_%s.eps', symmetry{k1}));
end
