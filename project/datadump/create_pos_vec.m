clear all;close all;

temp = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\tof-pos1-control.txt');

i = size(temp);
 
xall = temp(1:i/3,:);
yall = temp(i/3+1:2/3*i,:);
zall = temp(2/3*i+1:end,:);

clear temp;

n = 4;

x = xall(176*n+1:176*(n+1), :);
y = yall(176*n+1:176*(n+1), :);
z = zall(176*n+1:176*(n+1), :);

pos = [];
threshold = 0;

for i = 1:176
    for j = 1:176   
        if j > 144
            pos(i, j, :) = zeros(1,1,3);
        else
            if z(i,j) <= threshold
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

x0 = 0;
y0 = 0;
alpha = 1;
beta = 1;
s = 1;

param = [x0, y0, alpha, beta, s];

[x0n, an, phin, rn, d, sigmah, conv, Vx0n, Van, uphin, urn, ... 
GNlog, a, R0, R] = lscone(temp, [0, 0,0.5]', [0,0,1]', 5*pi/180, 0.125, 0.1, 0.1);


% [f] = fgcylinder(param, pos_vec);



%% start constructing the cylinder from the given parameters.


[X, Y, Z] = cylinder([rn*1, rn*sin(phin)], 125);

X = X-x0n(1);
Y = Y-x0n(2);
Z = (Z+x0n(3))./(1+x0n(3));

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

start = 1;
surfl(pos(:,:,3), pos(:,:,1), pos(:,:,2));
hold on
surf(Z(start:end,:), X(start:end,:), Y(start:end,:));
axis equal
hold off


