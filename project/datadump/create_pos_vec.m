
tof_low_pass; %filter the input using averagin. 


pos = [];
threshold = .5; % Threshold for how far away there should be data.

for i = 1:176
    for j = 1:176   
        if j > 144
            pos(i, j, :) = zeros(1,1,3);
        else
            pos(i,j,1) = x(i,j);
            pos(i,j,2) = y(i,j);
            pos(i,j,3) = z(i,j);
%             if (z(i,j) <= threshold) || (z(i,j) >= .6) % sort out the right distances
%                 pos(i,j,1) = 0;
%                 pos(i,j,2) = 0;
%                 pos(i,j,3) = 0;
%             end
        end
    end
end

pos_vec = [];
k = 1;
for i=1:176
    for j = 1:144
        if (pos(i,j,3) > threshold*k) || (pos(i,j,3) < threshold*(k+1))
            pos_vec(i*j, :) = pos(i,j,:);
        elseif pos(i,j,3)> threshold*(k+1)
            k = k+1;
            
            
        
    end
end

pos_vec = sortrows(pos_vec, 3); % sort the vector on z value.

 temp = pos_vec;
 temp(~any(pos_vec,2),:)=[]; %% remove trivial points

%% Start the surface fit

x0 = [0,0,threshold]';
a0 = [0,0,1]';
angle = 0;
radius = 0.125;

% [x0n, an, phin, rn, d, sigmah, conv, Vx0n, Van, uphin, urn, ... 
% GNlog, a, R0, R] = lscone(temp, x0, a0, angle, radius, 0.1, 0.1);

[x0n, an, rn, d, sigmah, conv, Vx0n, Van, urn, GNlog, a, R0, R] = ...
        lscylinder(temp, x0, a0, radius, .001, .001);



%% start constructing the cylinder from the given parameters.
[X, Y, Z] = cylinder(rn*ones(2,1), 125); % plot the cone.

%find the rotation matrices

%U = rot3z(an);

% ax = axes('XLim',[-0.5 0.5],'YLim',[-0.5 0.5],...
% 	'ZLim',[0.0 .5]);
% tf = hgtransform('Parent', ax);

rot_axis = cross([0;0;1],an);

if norm(rot_axis) > eps
    rot_angle = asin(norm(rot_axis));
    if(dot([0;0;1], an)< 0)
        rot_angle = pi-rot_angle;
    end
else
    rot_axis = [0;0;1];
    rot_angle = 0;
end

tf = makehgtform('translate', x0n, 'axisrotate', rot_axis, rot_angle, ...
     'scale', [1; 1;(max(temp(:,3))-min(temp(:,3)))]);

for i = 1:126
    for j = 1:2
        X(j, i) = tf(1, 1:3)*[X(j,i); Y(j,i); Z(j,i)] + tf(1,4);
        Y(j, i) = tf(2, 1:3)*[X(j,i); Y(j,i); Z(j,i)] + tf(2,4);
        Z(j, i) = tf(3, 1:3)*[X(j,i); Y(j,i); Z(j,i)] + tf(3,4);
    end
end
 
%normalize the cyliner length


%% Start plotting the data.
close all;

figure;
plot(d, temp(:,3)); % distribution of the points along the z-axis
xlabel('Length from point on cylinder [m]');
ylabel('Detpth into the pipe, Z-axis [m]');
grid on

figure;
set(gcf, 'Renderer', 'opengl');
start = 1;
%surfl(pos(:,:,3), pos(:,:,1), pos(:,:,2));
test = plot3(temp(:,3), temp(:,1), temp(:,2), '.');
hold on
test2 = surface(Z, X, Y);



hold off    
axis equal;
xlabel('Depth');
ylabel('Camera X-direction');
zlabel('Camera Y-direction');
grid on

figure;
plot(d);
title('Distance of points from the fitted cylinder');

