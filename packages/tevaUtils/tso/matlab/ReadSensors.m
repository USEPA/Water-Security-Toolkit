function [ sensors ] = ReadSensors( filename )
%Read in one or more sensor placements from a file
% 
%  INPUT
%    filename - The name of a file that contains one or more sensor placements.
%
%  OUTPUT
%    sensors - An array of structs that contain sensor placements:
%       sensors(i).id        - The ID of the i-th sensor placement
%       sensors(i).locations - The locations of the i-th sensor placement
%
%  NOTE: The sensor placement file is assumed to have the formate specified for
%        TEVA-SP

% Error checking
if nargin == 0
    error('ReadSensors: Empty filename');
end

% Open the sensor file
INPUT = fopen(filename,'r');
if INPUT == -1
    error('MATLAB:noSuchFile','ReadSensors: File "%s" not found', filename);
end

% Read the file one line at a time, ignoring lines that begin with '#'
i=1;
while 1
    line = fgetl(INPUT);
    if ~ischar(line)
        break
    end
    if line(1) ~= '#'
        %sdisp(line);
        sp = textscan(line,'%d');
        sp = sp{1}';
        sensors(i).id = sp(1);
        sensors(i).locations = sp(3:sp(2)+2);
        i = i+1;
    end
end

% Close the file
fclose(INPUT);