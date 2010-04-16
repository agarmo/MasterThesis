%% script for getting both sensors
clear all; close all;

ToF = [];

%open both sensors
if isempty(ToF) || (ToF == -1)
    [ToF] = sr_open();
end

hokuyo_matlab('open', 'COM5', 115200);

%angle to be plotted against
theta = (360/1024)*(pi/180).*(1:1024);

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
    h1 = [zeros(128,1);h1; zeros((768-726)+128, 1)]; %% fill the array with zeros when outside the FOV
   
    figure(1);
    subplot(2, 2, 1), image(ampimg', 'cdatamapping', 'scaled');
    title('ToF-camera Amplitude image');
       
    subplot(2, 2, 2), polar(theta', h1);
    title('Plot of the URG Laser Range finder');
    
    subplot(2, 1, 2), mesh(double(z));
    title('Depth plot from the ToF-camera');
    
    
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



