%Sample script that uses the hokuyoAPI interface to Hokuyo C++ driver
%Sensor must be upgraded to SCIP2.0 format

%Compile the API with the mexHokuyoReaderAPI script.
%Note: in order to shutdown the device cleanly, type "clear hokuyoAPI" or "clear all"

%Aleksandr Kushleyev
%University of Pennsylvania, 2008

%{
General info(with Hokuyo URG-04LX):
The sensor provides three types of data:
-range (0-5500 mm)
-intensity (reflectivity)     0-10000 or 0-40000 (for some reason older sensors have different range)
-AGC (Automatic Gain Control) 0-1000 (somewhat inverse proportional (but nonlinear..) relationship to reflectivity of the surface)

%}

%provide the device name and baud rate 
%(if you use USB connection, then the baud rate setting does not matter
% - the device will work at full speed regardless of the setting)

%dev='/dev/cu.usbmodem0000101D1';
dev='/dev/ttyACM1';
baud=115200;      %does not matter for USB

if ~hokuyoReaderAPI('open',dev,baud)
    return;
end

%{
maximum number of points for hokuyo URG 04-LX is 769
however, if you request range, reflectivity, and agc data,
(all three at the same time), only 1/3 of points will be returned -
every 2 out of 3 points will be thrown out to maintain the same
total number of points - see hokuyo protocol...
%}

n_points=769;
%n_points=1081;   %1081 is max for URG 30LX

%may need to play with resolution - the one provided in the manual produces distorted (bent) walls.
%just try changing it manually
res=0.356;   %URG-04LX
%res=0.25;   %URG-30LX

%the vector of angles
angles=((0:res:(n_points-1)*res)-135)'*pi/180;

%setup figures
figure(1);
clf(gcf);
hXY=plot(0,0,'b.');
axis([-5500 5500 -5500 5500]);

figure(2);
clf(gcf);
hR=plot(0,0,'b.');hold on;
hRef=plot(0,0,'g.'); 
hAGC=plot(0,0,'r.'); hold off;
axis([0 1200 0 10000]);
drawnow;


disp('press any key to start reading data');
pause;

%all possible command modes
cmd='range';                        %both URG-04LX and URG-30LX
%cmd='top_urg_range+intensity';     %only for URG-30LX

%the commands below are not supporoted by URG-30LX
%cmd='range+intensityAv+AGCAv';
%cmd='range+intensity0+AGC0';
%cmd='range+intensity1+AGC1'; 
%cmd='range+intensityAv';
%cmd='range+intensity0';
%cmd='range+intensity1';
%cmd='intensityAv';
%cmd='intensity0';
%cmd='intensity1';
%cmd='AGC0';
%cmd='AGC1';

start=0;            %start step of the scan
stop=n_points-1;    %end step of the scan
encoding=3;         %2 or 3 character data encoding (data coming from sensor to the driver)

%only for range measurements (04LX), and both range and intensity (30LX), 
%you can specify how many points to skip:
% skip=1: return all points, skip=2: return every other point,etc
skip=1;

hokuyoReaderAPI('setScanSettings',cmd,start,stop,skip,encoding);

while(1)
  r=[];
  agc=[];
  ref=[];
  
  %tic
     
  
  %below are examples of accessing all the possible combinations of data that the manual describes
  [r ref agc]=hokuyoReaderAPI('getScan');
 
  %toc
  
  %size(r)

  if ~isempty(r)
    %only plot the x,y coordinates if 'r' is the only thing you requested
    %otherwise, you have to calculate the angles differently, because
    %points get thrown out - this is just a quick demo..
    if size(r)==size(angles)
      x=r.*cos(angles);
      y=r.*sin(angles);
      set(hXY,'xdata',x,'ydata',y);
    else
      %disp('warning: the size of r does not match!!!!!!!!!!!!');
    end  
    
    %plot the radii
    set(hR,'xdata',1:length(r),'ydata',r);
  end

  if ~isempty(ref)
    %plot the reflectivity data
    set(hRef,'xdata',1:length(ref),'ydata',ref);
  end

  if ~isempty(agc)
    %plot the AGC (automatic gain correction) data
    set(hAGC,'xdata',1:length(agc),'ydata',agc);
  end
  drawnow;
end
