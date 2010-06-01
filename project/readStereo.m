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

temp3 = sortrows(pos, 3);
pos = temp3;

temp2 = pos;
temp2(~any(pos,2),:)=[]; %% remove trivial points



scatter3(temp2(1:1:end,3), temp2(1:1:end,1), temp2(1:1:end,2), '.')
grid on
xlabel('Depth');
ylabel('X');
zlabel('Y');
axis equal

% mesh(zall(1:10:480,:), xall(1:10:480,:), yall(1:480,:))