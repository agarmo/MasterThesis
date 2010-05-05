
%tof_low_pass; %filter the input using averagin. 

%% parameters
close all;

interval = 0.5;
radius = 0.125; % radius of the pipe
a0 = [0,0,1]'; % allways assume that the direction is along the z-direction



%% put the coordinates into a mesh array for surface
% pos = [];
% 
% for i = 1:176
%     for j = 1:176   
%         if j > 144
%             pos(i, j, :) = zeros(1,1,3);
%         else
%             pos(i,j,1) = x(i,j);
%             pos(i,j,2) = y(i,j);
%             pos(i,j,3) = z(i,j);
%         end
%     end
% end

%% Put the coordinates into a list of 3d points and sort it on z-value
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
 
%% Calculate the interval

startz = pos_vec(1,3);
stopz = pos_vec(size(pos_vec, 1), 3);

pieces = ceil((stopz-startz)/interval); % round upwards toward nearest integer to ensure all values

x0k = [];
ank = [];
rk = [];
dk = [];
length_d = [];
a_parmk = [];

for k = 1:pieces
    %% Select data from total selection
    
    temp = [];
    for i = 1:size(pos_vec,1) % might be optimized because of the sorted array
        if (pos_vec(i, 3) >= interval*(k-1)) && (pos_vec(i, 3) <= interval*k)
            temp = [temp; pos_vec(i, :)];
        end
    end
    
    
    
    %% Start the surface fit
    
    x0 = [0,0,interval*(k-1)]';
    angle = 0; % not used with cylinderfit
    
    % [x0n, an, phin, rn, d, sigmah, conv, Vx0n, Van, uphin, urn, ...
    % GNlog, a, R0, R] = lscone(temp, x0, a0, angle, radius, 0.1, 0.1);
    
    if (isempty(temp) ) || (size(temp,1) < 5) 
        disp('Too few points')
    else
        
        [x0n, an, rn, d, sigmah, conv, Vx0n, Van, urn, GNlog, a, R0, R] = ...
            lscylinder(temp, x0, a0, radius, .001, .001);
        
        x0k = [x0k; x0n'];
        ank = [ank; an'];
        rk = [rk; rn];
        dk = [dk; d];
        length_d = [length_d; size(d,1)];
        a_parmk = [a_parmk; a'];
    end

end

rot_axis = zeros(3, size(x0k, 1));
rot_angle = zeros(size(x0, 1), 1);

figure;
set(gcf, 'Renderer', 'opengl');
test = plot3(pos_vec(:,3), pos_vec(:,1), pos_vec(:,2), '.');
hold on;
for k = 1:size(x0k, 1)
    
    if rk(k) > 2*radius
        disp('Radius errenous. Skipping...');
    else
        %% start constructing the cylinder from the given parameters.
        [X, Y, Z] = cylinder(rk(k)*ones(2,1), 125); % plot the cone.
        
        
        %% find rotation axis and transformation matrix
        rot_axis(:, k) = cross(a0,ank(k,:)');
        if norm(rot_axis(:,k)) > eps
            rot_angle(k) = asin(norm(rot_axis(:,k)));
            if(dot(a0, ank(k,:)')< 0)
                rot_angle(k) = pi-rot_angle(k);
            end
        else
            rot_axis(:,k) = a0;
            rot_angle(k) = 0;
        end
                
        Sz = makehgtform('scale', [1;1;interval]);
        Txyz = makehgtform('translate', x0k(k,:));
        Rxyz = makehgtform('axisrotate', rot_axis(:,k), rot_angle(k));
        
        tf = Txyz*Rxyz*Sz;
        
        
        %% Transform cylinder to right scale and position
        for i = 1:size(X,2)
            for j = 1:2
                X(j, i) = tf(1, 1:3)*[X(j,i); Y(j,i); Z(j,i)] + tf(1,4);
                Y(j, i) = tf(2, 1:3)*[X(j,i); Y(j,i); Z(j,i)] + tf(2,4);
                Z(j, i) = tf(3, 1:3)*[X(j,i); Y(j,i); Z(j,i)] + tf(3,4);
            end
        end
        
        
        h(k) = surface(Z, X, Y);

    end

end

hold off;
axis equal;
xlabel('Depth');
ylabel('Camera X-direction');
zlabel('Camera Y-direction');
grid on


%% Start plotting othre data


figure;
plot(dk);
title('Distance of points from the fitted cylinder');
hold on
plot([length_d; size(dk,1)], zeros(1, size(length_d,1)+1), '+r', 'MarkerSize', 10);
hold off;

figure;
plot(dk, pos_vec(:,3)); % distribution of the points along the z-axis
xlabel('Length from point on cylinder [m]');
ylabel('Detpth into the pipe, Z-axis [m]');
grid on


