%% lateral calibration of the SR3000

[dev] = sr_open(); % open the connection

k = 1;

w = -1;

while(1)
           
    if w == 1 % keypress
        
        disp('Saving picture')
        
        %convert to double
        img_temp = (double(img)+1)/255;
        
        img_temp = histeq(uint8(img_temp));
        
        figure(2)
        imshow(img_temp);
        
        imwrite(img_temp, sprintf('%s%d.bmp', 'amp',k), 'bmp');
        
        
        k = k+1
    end
    %if w == 0
    %    disp('Mouse clicked, exiting')
    %    break;
    %end
    
    sr_acquire(dev);
    img = sr_getimage(dev,1);
    figure(1);
    image(img',  'cdatamapping', 'scaled');
    drawnow
    
    w = waitforbuttonpress;
    
    
end

disp('Closing connection to dev')
sr_close(dev);
