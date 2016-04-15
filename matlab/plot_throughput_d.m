% Assume the data file contains the average throughput for each distance.

figure; box on;
str = {'baseline', 'smart'};
for k = 1:2
    R = load(sprintf('../throughput_d_%s.dat', str{k}));
    plot(R(:,1), R(:,2), 'LineWidth', 2);
    hold on;
%     xlim([1, 5]);
    xlabel('$d$', 'interpreter', 'latex');
    ylabel('$\bar{R}$', 'interpreter', 'latex');
    fig = gcf();
    set(findall(fig, '-property', 'FontSize'), 'FontSize', 16);
end
legend(str);
print('-depsc2', 'throughput_d.eps');