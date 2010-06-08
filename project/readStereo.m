clear all;
close all;


temp = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\stereo\pos1-control.txt');

xall = zeros(480*5, 640);
yall = zeros(480*5, 640);
zall = zeros(480*5, 640);

for i = 1:size(temp,1)
    for j = 1:640
        xall(i,j) = temp(i, (j*3)-2);
        yall(i,j) = temp(i, (j*3)-1);
        zall(i,j) = temp(i, j*3);
        
        if zall(i,j) == 10000
            zall(i,j) = 0;
            xall(i,j) = 0;
            yall(i,j) = 0;
        end
    end
end

pos = zeros(640*480,3);
for i = 1:480
    for j = 1:640
           pos(i*j, :) = [(xall(i*1,j)+xall(i*2,j)+xall(i*3,j)+ xall(i*4,j) + xall(i*5,j))/5,...
               (yall(i*1,j)+yall(i*2,j)+yall(i*3,j)+ yall(i*4,j) + yall(i*5,j))/5,...
               (zall(i*1,j)+zall(i*2,j)+zall(i*3,j)+ zall(i*4,j) + zall(i*5,j))/5];
    end
end
% 
% pos = resultImage3d(:,1:3);
% pos = [pos; resultImage3d(:,4:6)];
% % 
% for i = 1:size(pos,1)
%     if pos(i,3) == 10000
%         pos(i,:) = zeros(3,1);
%     end
% end

clear xall;
clear yall;
clear zall;
clear temp;

pos = sortrows(pos, 3);

temp2 = pos;
temp2(~any(pos,2),:)=[]; %% remove trivial points

clear pos;

% estiamte the cylinder
[x0n, an, rn, d, sigmah, conv, Vx0n, Van, urn, GNlog, a] = ...
                        lscylinder(temp2, [0, 0, -40]', [0,0,1],...
                        0.125, 100, 100);



scatter3(-temp2(1:1:end,3), -temp2(1:1:end,1), -temp2(1:1:end,2), '.')
grid on
xlabel('Depth');
ylabel('X');
zlabel('Y');
axis equal


% 
% [X, Y, Z] = cylinder(rn*ones(2,1), 125); % plot the cone.
%                     
%                     
%                     % find rotation axis and transformation matrix
%                     rot_axis = cross([0,0,1]',an');
%                     if norm(rot_axis) > eps
%                         rot_angle = asin(norm(rot_axis));
%                         if(dot([0,0,1]', an)< 0)
%                             rot_angle = pi-rot_angle;
%                         end
%                     else
%                         rot_axis = [0,0,1]';
%                         rot_angle = 0;
%                     end
%                     
%                     Txyz = makehgtform('translate', x0n);
%                     Rxyz = makehgtform('axisrotate', rot_axis, rot_angle);
%                     Sz = makehgtform('scale', [1,1, abs(min(temp2(:,3)))+abs(max(temp2(:,3)))]);
%                     
%                     
%                     tf = Txyz*Rxyz*Sz;
%                     Xny = zeros(2, size(X,2));
%                     Yny = zeros(2, size(Y,2));
%                     Zny = zeros(2, size(Z,2));
%                     
%                     % Transform cylinder to right scale and position
%                     for i = 1:size(X,2)
%                         for j = 1:2
%                             Xny(j, i) = (tf(1, 1:4)*[X(j,i); Y(j,i); Z(j,i); 1]);
%                             Yny(j, i) = (tf(2, 1:4)*[X(j,i); Y(j,i); Z(j,i); 1]);
%                             Zny(j, i) = (tf(3, 1:4)*[X(j,i); Y(j,i); Z(j,i); 1]);
%                         end
%                         
%                     end
%                     hold on;
%                     surface(Zny./tf(4,4), Xny./tf(4,4), Yny./tf(4,4));
%                     hold off
% 


% mesh(zall(1:10:480,:), xall(1:10:480,:), yall(1:480,:))