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


%main loop
while 1
    
    tic;
    [h1] = hokuyo_matlab('getReading');
    toc
    
    sr_acquire(ToF);
    %[res, x, y, z] = sr_coordtrf(ToF);
    tic;
    ampimg = sr_getimage(ToF,1);
    toc;
    h1 = [h1; zeros((768-726), 1)]; %% fill the array with zeros when outside the FOV
    
    figure(1);
    subplot(1, 2, 1), image(ampimg, 'cdatamapping', 'scaled');
    subplot(1, 2, 2), polar(theta', h1);
    
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



