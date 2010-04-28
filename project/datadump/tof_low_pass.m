%% Low pass filter for ToF-data

%% Load in data from files
temp = csvread('C:\Documents and Settings\anderga\My Documents\MATLAB\tof-pos1-control.txt');

i = size(temp);
 
xall = temp(1:i/3,:);
yall = temp(i/3+1:2/3*i,:);
zall = temp(2/3*i+1:end,:);

clear temp;



for n = 1:size(xall)

    %input filter here
    
    
    x = xall(176*n+1:176*(n+1), :);
    y = yall(176*n+1:176*(n+1), :);
    z = zall(176*n+1:176*(n+1), :);

end


