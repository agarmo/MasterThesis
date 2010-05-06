clear all; close all;

%% Create world
verden = world(.25);

oversetter = sensorinterpreter();

oversetter = oversetter.setWorld(verden);

%% load data and average
%% urg
disp('Loading urg file...')
urg = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\urg-pos1-control.txt');
disp('Loading urg file... Done!')
ranges = hokuyo_parse_range(urg,1,size(urg)); % parses the ranges and angles in rad and meters

%average urg
start = 2;
stop = 101;
for i = start+1:stop
    ranges(2,:) = ranges(2,:) + ranges(i,:);
end
ranges(2,:) = ranges(2,:)./(stop-(start-1));

oversetter = oversetter.setLRFData(ranges(1:2,:));

clear ranges;


%% tof
disp('Loading ToF file...')
temp = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\tof-pos1-control.txt');
disp('Loading ToF file... Done!')
i = size(temp);
 
xall = temp(1:i/3,:);
yall = temp(i/3+1:2/3*i,:);
zall = temp(2/3*i+1:end,:);

clear temp;

% Start the filtering.
start = 10;
stop = 100;
interval = stop-start;

x = zeros(176,144);
y = zeros(176,144);
z = zeros(176,144);

for n = start:stop

    %input filter here simple averaging
    x = x+xall(176*n+1:176*(n+1), :);
    y = y+yall(176*n+1:176*(n+1), :);
    z = z+zall(176*n+1:176*(n+1), :);
    
end

x = x./interval;
y = y./interval;
z = z./interval;

clear xall;
clear yall;
clear zall;

pos_vec = [];
for i=1:176
    for j = 1:144
        pos_vec(i*j, :) = [x(i,j), y(i,j), z(i,j)];
    end
end
pos_vec = sortrows(pos_vec, 3); % sort the vector on z value.

temp = pos_vec;
temp(~any(pos_vec,2),:)=[]; %% remove trivial points

pos_vec = temp;
clear temp;
clear x;
clear y;
clear z;

oversetter = oversetter.setToFData(pos_vec);

clear pos_vec;
