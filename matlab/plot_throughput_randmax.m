% Assume the data file contains the average throughput for each rand_max
% from 1 to the total number of lines.
R = load('realtime_throughput_randmax.dat');

figure; box on;
plot(R, 'LineWidth', 2);
hold on;
% x = 7;
% y = R(x);
% txt = sprintf('(%d, %3.1f)', x, y);
% text(x, y, txt, 'HorizontalAlignment', 'right')
plot(1:8, 1:8, 'k--', 'LineWidth', 2);
xlabel('$N_{\mathrm{max}}$', 'interpreter', 'latex');
ylabel('$\bar{R}$', 'interpreter', 'latex');
fig = gcf();
set(findall(fig, '-property', 'FontSize'), 'FontSize', 16);
print('-depsc2', 'realtime_throughput_randmax.eps');