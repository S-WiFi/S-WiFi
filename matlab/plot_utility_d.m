function plot_utility_d(symmetry, policy, lutf)
%function plot_throughput_d(symmetry, policy, lutf)
% Plot (timely-)throughput vs d for multiple test cases.
%
% param symmetry: cell of strings. Valid strings are 'sym' and 'asym'.
%                 (default: {'sym', 'asym'})
% param policy: cell of strings. Valid strings are 'baseline', 'smart', or the
%               string representation of numbers from 0 to 7.
%               (default: {'baseline', 'smart'})
% param lutf: path to the file containing the mapping from distances to channel
%             reliabilities. (default: '../report/swifi_reliability_uplink.dat')
% output: EPS figure files with name prefix 'throughput_d_'.
%
% Usage example:
%
% plot_throughput_d({'sym'}, {'baseline', '1', '2', '4', 'smart'})
% will generate the plots for baseline, smart, and each intermediate policy with
% a single feature enabled under the symmetric channel scenario.
% (Assuming you have generated the .dat files for these policies and channels.)

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
scale = 100;
for k1 = 1:length(symmetry)
    figure; box on;
    for k2 = 1:length(policy)
        f = sprintf('../throughput_d_%s_%s_long.dat', symmetry{k1}, policy{k2});
        if exist(fullfile(pwd, f), 'file') == 2
            R = load(f);
        else
            fprintf(2, 'File %s does not exist!\n', f);
            R = [ones(11, 1), zeros(11, 1)];
        end
        d = unique(R(:,1));
        p = arrayfun(d2p, d);
        util = zeros(length(d), 1);
        for k3 = 1:length(d)
           id = find(R(:,1)==d(k3));
           util(k3) = compute_utility(R(id,3), scale); 
        end
        plot(p, util, style{k2}, 'LineWidth', 2, 'MarkerSize', 10);
        hold on;
        xlabel('$p$', 'interpreter', 'latex');
        ylabel('$\bar{R}$', 'interpreter', 'latex');
        fig = gcf();
        set(findall(fig, '-property', 'FontSize'), 'FontSize', 16);
    end
    legend(str, 'location', 'northwest');
    print('-depsc2', sprintf('throughput_d_%s.eps', symmetry{k1}));
end
