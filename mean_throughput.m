#!/usr/bin/octave -qf
arg_list = argv();
datfile = arg_list{1};
var_name = arg_list{2};
x = str2num(arg_list{3});
policy = arg_list{4};
symmetry = arg_list{5};
R = load(datfile);
r = mean(R);
f = fopen(sprintf('throughput_%s_%s_%s.dat', var_name, symmetry, policy), 'a');
fprintf(f, '%d %f\n', x, r);
fclose(f);
