function plot_throughput_d(symmetry, policy, lutf)
% Assume the data file contains the average throughput for each distance.

%% Set default arguments.
if nargin < 1
    symmetry = {'sym', 'asym'};
end
if nargin < 2
    policy = {'baseline', 'smart'}; % basic comparison
    %policy = {'baseline', '1', '3', 'smart'}; % incrementally added features
    %policy = {'baseline', '1', '2', '4', 'smart'}; % each individual feature
end
if nargin < 3
    lutf = '../report/swifi_reliability_uplink.dat';
end

%% Load mapping between distance and reliability.
lut = load(lutf);
lut_d = lut(:,1);
lut_p = lut(:,2);
d2p = @(d) lut_p(lut_d == d);

%% Translate policy numbers to strings.
str = policy;
for k = 1:length(policy)
    switch policy{k}
    case '0'
        str{k} = 'baseline';
    case '1'
        str{k} = 'selective';
    case '2'
        str{k} = 'piggyback';
    case '3'
        str{k} = 'selective+piggyback';
    case '4'
        str{k} = 'retry limit';
    case '5'
        str{k} = 'selective+retry limit';
    case '6'
        str{k} = 'piggyback+retry limit';
    case '7'
        str{k} = 'smart';
    end
end

%% Plot.
style = {'kx-', 'bo-', 'm^-', 'rd-'};
for k1 = 1:length(symmetry)
    figure; box on;
    for k2 = 1:length(policy)
        f = sprintf('../throughput_d_%s_%s.dat', symmetry{k1}, policy{k2});
        if exist(fullfile(pwd, f), 'file') == 2
            R = load(f);
        else
            fprintf(2, 'File %s does not exist!\n', f);
            R = [ones(11, 1), zeros(11, 1)];
        end
        d = R(:,1);
        p = arrayfun(d2p, d);
        plot(p, R(:,2), style{k2}, 'LineWidth', 2, 'MarkerSize', 10);
        hold on;
        xlabel('$p$', 'interpreter', 'latex');
        ylabel('$\bar{R}$', 'interpreter', 'latex');
        fig = gcf();
        set(findall(fig, '-property', 'FontSize'), 'FontSize', 16);
    end
    legend(str, 'location', 'northwest');
    print('-depsc2', sprintf('throughput_d_%s.eps', symmetry{k1}));
end
