%% Script interpreting urg-range data.
close all; clear all;

urg = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\urg-pos1-control.txt');
ranges = hokuyo_parse_range(urg,1,size(urg)); % parses the ranges and angles in rad and meters

%% start interpreting the data.

%transform to cartesian coordinates
[urgx, urgy] = pol2cart(ranges(1,:), ranges(5, :));

%sort out special lines
for i = 1:1024
    if urgy(i) < 0.5
        urgx(i) = 0;
        urgy(i) = 0;
    end
end

 temp = [urgx', urgy'];
 temp(~any([urgx', urgy'],2),:)=[]; %% remove trivial points


%look for different shapes, arcs, lines, 

[x0_urg, a_urg, d_urg, normd_urg] = ls2dline(temp);

%calculate the line
t = 0.7:-0.01:-1.4;

x_urg = x0_urg(1) + a_urg(1).*t;
y_urg = x0_urg(2) + a_urg(2).*t;


%% plot the data
figure;
polar(ranges(1,:), ranges(61,:), '.');
title('Plot of the URG Laser Range finder');


figure;
plot(urgx, urgy, '.');
grid on;
xlabel('Depth into the pipe Z-axis [m]');
ylabel('X-axis relative to ToF-camera [m]');
title('URG Laser Range Finder');
hold on;
plot(x_urg, y_urg);
hold off;
