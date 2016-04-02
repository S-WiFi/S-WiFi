function plot_queue_sample_path(input, N_client)

[m,n] = size(input);
T = m/N_client;
queue_length = zeros(N_client, T);
time = 1:1:T;
for i=1:N_client
    id = i:N_client:m;
    queue_length(i,:) = input(id);
end
createfigure(time, queue_length);   
    
end
