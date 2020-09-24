function [ ] = SummarizeSensors( sensors )
%Summarize the frequency of sensor placements at various locations
% TODO

% An array that will contain the counts for sensor placements
Count = [];

% Iterate through the list of sensors
nsp = numel(sensors);
for i = 1:nsp
    %disp(sensors(i).locations)
    nsensors = numel(sensors(i).locations);
    for j = 1:nsensors
        if sensors(i).locations(j) > length(Count)
            Count(sensors(i).locations(j)) = 1;
        else
            Count(sensors(i).locations(j)) = Count(sensors(i).locations(j)) + 1;
        end
    end
end

Count = Count/nsp;
for i = 1:length(Count)
    if Count(i) ~= 0
        disp(sprintf('%d\t%f',i,Count(i)));
    end
end