%% Script interpreting urg-range data.
close all; clear all;

urg = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\urg-pos1-control.txt');
ranges = hokuyo_parse_range(urg,1,size(urg)); % parses the ranges and angles in rad and meters

%% start interpreting the data.

%transform to cartesian coordinates
[urgx, urgy] = pol2cart(ranges(1,:), ranges(5, :));

% sort on x
sorted = sortrows([urgx', urgy'], -1);

%sort out special lines
% for i = 1:1024
%     if urgy(i) > 0.0
%         urgx(i) = 0;
%         urgy(i) = 0;
%     end
% end

temp = sorted;
temp(~any(sorted,2),:)=[]; %% remove trivial points (0,0)

histrange = (-1.6:0.05:0.7)';

[nx, binx] = hist(temp(:,1), histrange);
[ny, biny] = hist(temp(:,2), histrange);

%pick out interesting poitns.

%threshold
x0_urg = [];
a_urg = [];
d_urg = [];
normd_urg =[];

x_urg = [];
y_urg = [];

for k = 1:47
        
    threshold = histrange(k);
    
    data = [];
    if ny(k) > 20
        for i = 1:size(temp(:,2))
            if (temp(i,2) > threshold-0.05) && (temp(i,2) < threshold+0.05)
                data = [data; temp(i,:)];
            end
        end
    end
    
    %look for different shapes, arcs, lines,
    if (~isempty(data)) && (size(data, 1) > 2)
        [x0_urgt, a_urgt, d_urgt, normd_urgt] = ls2dline(data);
        x0_urg = [x0_urg; x0_urgt'];
        a_urg = [a_urg; a_urgt'];
        %d_urg = [d_urg; d_urgt'];
        normd_urg =[normd_urg; normd_urgt];
        
        %calculate the line
        
        %scaling is difficutl!!! 
        x_urgt_s = (x0_urgt(1) + ((a_urgt(1)*0)).*min(data(:,1));
        y_urgt_s = (x0_urgt(2) + ((a_urgt(2)*0)).*min(data(:,2));
        
        x_urgt_l = (x0_urgt(1) + (a_urgt(1)*1)).*max(data(:,1));
        y_urgt_l = (x0_urgt(2) + (a_urgt(2)*1)).*max(data(:,2));
        
        x_urg = [x_urg; [x_urgt_s, x_urgt_l]];
        y_urg = [y_urg; [y_urgt_s, y_urgt_l]];
    end

end



%% plot the data
figure;
polar(ranges(1,:), ranges(61,:), '.');
title('Plot of the URG Laser Range finder');


figure;
subplot(2, 1, 1);
plot(sorted(:,1), sorted(:,2), 'r.');
hold on;
% plot(urgx, urgy, '.');
grid on;
xlabel('Depth into the pipe Z-axis [m]');
ylabel('X-axis relative to ToF-camera [m]');
title('URG Laser Range Finder');
for i = 1:size(x_urg,1)
    line([x_urg(i,1);x_urg(i,2)],[y_urg(i,1);y_urg(i,2)]);
end
hold off;

subplot(2, 1, 2);
plot(d_urgt);
title('Error from the fitted line');
grid on;

figure;
subplot(1, 2, 1)
hist(temp(:,1), histrange);
title('Histogram of x-coordinates');

subplot(1, 2, 2);
hist(temp(:,2), histrange);
title('Histogram of y-coordinates');
