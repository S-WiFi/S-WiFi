#!/usr/bin/octave -qf
arg_list = argv();
T = str2num(arg_list{1});
datfile = arg_list{2};
R = load(datfile);
r = mean(R);
f = fopen('throughput_T.dat', 'a');
fprintf(f, '%d %f\n', T, r);
fclose(f);
