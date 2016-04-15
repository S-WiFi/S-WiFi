% Assume the data file contains the average throughput for each rand_max
% from 1 to the total number of lines.

str = {'realtime', 'nonrealtime'};
for k = 1:2
    R = load(sprintf('%s_throughput_K.dat', str{k}));
    figure; box on;
    plot(R(:,1), R(:,2), 'LineWidth', 2);
    hold on;
    xlim([1, 5]);
    xlabel('$K$', 'interpreter', 'latex');
    ylabel('$\bar{R}$', 'interpreter', 'latex');
    fig = gcf();
    set(findall(fig, '-property', 'FontSize'), 'FontSize', 16);
    print('-depsc2', sprintf('%s_throughput_K.eps', str{k}));
end
