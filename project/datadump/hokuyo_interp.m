%% Script interpreting urg-range data.

urg = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\urg-pos21-control.txt');
ranges = hokuyo_parse_range(urg,1,size(urg)); % parses the ranges and angles in rad and meters

%% start interpreting the data.

%look for different shapes, arcs, lines, 




%% plot the data
     polar(ranges(1,:), ranges(61,:));
     title('Plot of the URG Laser Range finder');
     