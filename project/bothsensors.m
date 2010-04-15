%% script for getting both sensors
clear all; close all;

ToF = [];

%open both sensors
if isempty(ToF) || (ToF == -1)
    [ToF] = sr_open();
end

hokuyo_matlab('open', 'COM5', 115200);

%angle to be plotted against
theta = (270/768)*(pi/180).*(1:768);

k=[];
set(gcf,'keypress','k=get(gcf,''currentchar'');');

threeD = zeros(176,176, 3);

%main loop
while 1
    
    tic;
    [h1] = hokuyo_matlab('getReading');
    toc
    
    sr_acquire(ToF);
    [res, x, y, z] = sr_coordtrf(ToF);
    tic;
    ampimg = sr_getimage(ToF,1);
    toc;
    h1 = [h1; zeros((768-726), 1)]; %% fill the array with zeros when outside the FOV
    
%     for i = 1:176
%         for j = 1:176
%             if j > 144
%                 threeD(i, j, 1) = 0;
%                 threeD(i, j, 2) = 0;
%                 threeD(i, j, 3) = 0;
%             else
%                 threeD(i, j, 1) = x(i,j);
%                 threeD(i, j, 2) = y(i,j);
%                 threeD(i, j, 3) = z(i,j);
%             end
%         end
%     end
    
    
    figure(1);
    subplot(2, 2, 1), image(ampimg, 'cdatamapping', 'scaled');
    subplot(2, 2, 2), polar(theta', h1);
    subplot(2, 1, 2), mesh(double(z));
    
  if ~isempty(k)
    if strcmp(k,'s');
        break; 
    end;
    if strcmp(k,'p');
        pause; k=[];
    end;
  end;
     
end

sr_close(ToF);
hokuyo_matlab('close');



