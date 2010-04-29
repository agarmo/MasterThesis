
tof_low_pass;


pos = [];
threshold = .5; % Threshold for how far away there should be data.

for i = 1:176
    for j = 1:176   
        if j > 144
            pos(i, j, :) = zeros(1,1,3);
        else
            if (z(i,j) <= threshold) || (z(i,j) >= 1.2)
                x(i,j) = 0;
                y(i,j) = 0;
                z(i,j) = 0;
            end
            pos(i,j,1) = x(i,j);
            pos(i,j,2) = y(i,j);
            pos(i,j,3) = z(i,j);
        end
    end
end

pos_vec = [];

for i=1:176
    for j = 1:144
        pos_vec(i*j, :) = pos(i,j,:);
        
    end
end

 temp = pos_vec;
 temp(~any(pos_vec,2),:)=[]; %% remove zeros

%% Start the surface fit

x0 = [0,0,0.5]';
a0 = [0,0,1]';
angle = 0;
radius = 0.125;

% [x0n, an, phin, rn, d, sigmah, conv, Vx0n, Van, uphin, urn, ... 
% GNlog, a, R0, R] = lscone(temp, x0, a0, angle, radius, 0.1, 0.1);

[x0n, an, rn, d, sigmah, conv, Vx0n, Van, urn, GNlog, a, R0, R] = ...
        lscylinder(temp, x0, a0, radius, 100, 100);



%% start constructing the cylinder from the given parameters.
[X, Y, Z] = cylinder([rn*1], 125); % plot the cone.

X = X + x0n(1);
Y = Y + x0n(2);
Z = Z*max(pos_vec(:,3));

Rx = rotox(acos(an(2)));
Ry = rotoy(acos(an(3)));
Rz = rotoz(acos(an(1)));

U = rot3z(an);

for i = 1:126
    for j =1:2
        X(j,i) = U(1,:)*[X(j,i);Y(j,i);Z(j,i)];
        Y(j,i) = U(2,:)*[X(j,i);Y(j,i);Z(j,i)];
        Z(j,i) = U(3,:)*[X(j,i);Y(j,i);Z(j,i)];
    end
end

%% Start plotting the data.

figure;
plot(pos_vec(:,3)); % distribution of the points along the z-axis

figure;
start = 1;
%surfl(pos(:,:,3), pos(:,:,1), pos(:,:,2));
plot3(pos(:,:,3), pos(:,:,1), pos(:,:,2), '.');
hold on
surf(Z(start:end,:), X(start:end,:), Y(start:end,:));
hold off    
axis equal
grid on



