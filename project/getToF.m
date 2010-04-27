%% script to get images from ToF-camera

[dev] = sr_open(); % open the connection

while(1)
    sr_acquire(dev);
    img = sr_getimage(dev,1);
    
    figure(1);
    image(img, 'cdatamapping', 'scaled');
    drawnow;
end



