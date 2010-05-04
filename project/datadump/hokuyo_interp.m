%% Script interpreting urg-range data.
close all; clear all;

urg = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\urg-pos1-irregularB.txt');
ranges = hokuyo_parse_range(urg,1,size(urg)); % parses the ranges and angles in rad and meters

%% start interpreting the data.

%transform to cartesian coordinates
[urgx, urgy] = pol2cart(ranges(1,:), ranges(5, :));

% sort on x
sorted = sortrows([urgx', urgy'], -1);


temp = sorted;
temp(~any(sorted,2),:)=[]; %% remove trivial points (0,0)

histrangey = (-1.6:0.05:0.7)';
histrange = (-1.6:0.1:0.7)';

[nx, binx] = hist(temp(:,1), histrange);
[ny, biny] = hist(temp(:,2), histrangey);

%pick out interesting poitns.

%Allocation for the horizontal lines
x0_urg = [];
a_urg = [];
d_urg = [];
normd_urg =[];
x_urg = [];
y_urg = [];

%Allocation for the vertical lines
x0_urgx = [];
a_urgx = [];
d_urgx = [];
normd_urgx =[];
x_urgx = [];
y_urgx = [];

for k = 1:size(histrangey)
        
    threshold = histrangey(k);
    % horizontal lines. from the parallell to the y-axis
    data = [];
    if ny(k) > 30
        for i = 1:size(temp(:,2))
            if (temp(i,2) > threshold-0.05) && (temp(i,2) < threshold+0.05)
                data = [data; temp(i,:)];
            end
        end
    end
    
    %look for different shapes, arcs, lines, parallell to the y-axis
    if (~isempty(data)) && (size(data, 1) > 2)
        [x0_urgt, a_urgt, d_urgt, normd_urgt] = ls2dline(data);
        x0_urg = [x0_urg; x0_urgt'];
        a_urg = [a_urg; a_urgt'];
        d_urg = [d_urg; d_urgt];
        normd_urg =[normd_urg; normd_urgt];
        
        %calculate the line
        
        %scaling
        startline = -x0_urgt(1)+ min(data(:,1));        
        t = sqrt((max(data(:,1))-min(data(:,1)))^2 + (max(data(:,2))-min(data(:,2)))^2) ;
        
        x_urgt_s = (x0_urgt(1) + ((a_urgt(1)*startline)));
        y_urgt_s = (x0_urgt(2) + ((a_urgt(2)*startline)));
        
        x_urgt_l = (x0_urgt(1) + (a_urgt(1)*t));
        y_urgt_l = (x0_urgt(2) + (a_urgt(2)*t));
        
        x_urg = [x_urg; [x_urgt_s, x_urgt_l]];
        y_urg = [y_urg; [y_urgt_s, y_urgt_l]];
    end
end

% look along the x-axis
for k = 1:size(histrange)
    threshold = histrange(k);
    datax = [];
    if nx(k) > 30
        for i = 1:size(temp(:,2))
            if (temp(i,1) > threshold-0.1) && (temp(i,1) < threshold+0.1)
                datax = [datax; temp(i,:)];
            end
        end
    end    
    if (~isempty(datax)) && (size(datax, 1) > 2)
        [x0_urgtx, a_urgtx, d_urgtx, normd_urgtx] = ls2dline(datax);
        x0_urgx = [x0_urgx; x0_urgtx'];
        a_urgx = [a_urgx; a_urgtx'];
        d_urgx = [d_urgx; d_urgtx];
        normd_urgx =[normd_urgx; normd_urgtx];
        
        %calculate the line
        
        %scaling
        startline = -x0_urgtx(2)+ min(datax(:,2));        
        t = sqrt((max(datax(:,1))-min(datax(:,1)))^2 + (max(datax(:,2))-min(datax(:,2)))^2) ;
        
        x_urgt_s = (x0_urgtx(1) + ((a_urgtx(1)*startline)));
        y_urgt_s = (x0_urgtx(2) + ((a_urgtx(2)*startline)));
        
        x_urgt_l = (x0_urgtx(1) + (a_urgtx(1)*t));
        y_urgt_l = (x0_urgtx(2) + (a_urgtx(2)*t));
        
        x_urgx = [x_urgx; [x_urgt_s, x_urgt_l]];
        y_urgx = [y_urgx; [y_urgt_s, y_urgt_l]];
    end
    
    
end



%% plot the data
figure;
polar(ranges(1,:), ranges(61,:), '.');
title('Plot of the URG Laser Range finder');


figure;
subplot(2, 2, 1:2);
plot(sorted(:,1), sorted(:,2), 'b.');
hold on;
% plot(urgx, urgy, '.');
grid on;
xlabel('Depth into the pipe Z-axis [m]');
ylabel('X-axis relative to ToF-camera [m]');
title('URG Laser Range Finder');

for i = 1:size(x_urg,1)
    line([x_urg(i,1);x_urg(i,2)],[y_urg(i,1);y_urg(i,2)], 'Color', 'magenta', ...
        'LineWidth', 1.5);
    %plot centroid
    plot(x0_urg(i,1), x0_urg(i,2), 'k*');
end

for i = 1:size(x_urgx,1)
    line([x_urgx(i,1);x_urgx(i,2)],[y_urgx(i,1);y_urgx(i,2)], 'Color', 'red', ...
         'LineWidth', 1.5);
    %plot centroid
    plot(x0_urgx(i,1), x0_urgx(i,2), 'g*');
end
hold off;

subplot(2, 2, 3);
plot(d_urg);
title('Error from the fitted lines Horizontal');
grid on;

subplot(2, 2, 4);
plot(d_urgx);
title('Error from the fitted lines Vertical');
grid on;


figure;
subplot(1, 2, 1)
hist(temp(:,1), histrange);
title('Histogram of x-coordinates');

subplot(1, 2, 2);
hist(temp(:,2), histrangey);
title('Histogram of y-coordinates');
