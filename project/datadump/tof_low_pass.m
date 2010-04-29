%% Low pass filter for ToF-data
clear all; close all;

%% Load in data from files
temp = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\tof-pos1-longpipe.txt');

i = size(temp);
 
xall = temp(1:i/3,:);
yall = temp(i/3+1:2/3*i,:);
zall = temp(2/3*i+1:end,:);

clear temp;

%% Start the filtering.
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
