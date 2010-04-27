clear all;close all;

temp = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\tof-pos1-control.txt');

 i = size(temp);
 
 xall = temp(1:i/3,:);
 yall = temp(i/3+1:2/3*i,:);
 zall = temp(2/3*i+1:end,:);
 
for n=1:size(xall)/176

x = xall(176*n+1:176*(n+1), :);
y = yall(176*n+1:176*(n+1), :);
z = zall(176*n+1:176*(n+1), :);

pos = [];

for i = 1:176
    for j = 1:176
        if j > 144
            pos(i, j, :) = zeros(1,1,3);
        else
            pos(i,j,1) = x(i,j);
            pos(i,j,2) = y(i,j);
            pos(i,j,3) = z(i,j);
        end
    end
end
axis('equal')
xlabel('Into the pipe');
ylabel('x');
zlabel('y');
grid on;
surf(pos(:,:,3), pos(:,:,1), pos(:,:, 2))
drawnow;
end
%draw field of view

alpha = 47.5*pi/180;
beta = 39.6*pi/180;

% close all;

%disp_camera_fov([0,0,0], [0,0,1], alpha, beta);
hold off;
