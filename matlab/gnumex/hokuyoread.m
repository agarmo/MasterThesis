clear all; close all;

[h1] = hokuyo_matlab_win('open', 'COM4', 115200);

theta = (270/768)*(pi/180).*(1:768);
h1 = [h1; zeros(768-726, 1)];

polar(theta', h1);

while(1)
    tic;
    [h1] = hokuyo_matlab_win('open', 'COM4', 115200);
    toc
    h1 = [h1; zeros(768-726, 1)];
    theta = (270/768)*(pi/180).*(1:768);
    pause(.5)
    polar(theta', h1);
    axis manual;
    
end

