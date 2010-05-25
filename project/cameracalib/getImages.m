%set up the video sources.
clear all;
close all;

stereo(1) = videoinput('winvideo',1)
stereo(2) = videoinput('winvideo', 2)


disp('Start imagecapture?')

w = -1;

preview(stereo(1));
preview(stereo(2));

k = 1;
while(1)
    
    
    if w == 1 % keypress
        
        disp('Saving picture')
        left = getsnapshot(stereo(1));
        right = getsnapshot(stereo(2));
        
        imwrite(left, sprintf('%s%d.bmp', 'left',k), 'bmp');
        imwrite(right, sprintf('%s%d.bmp', 'rigth',k), 'bmp');
        k = k+1
    end
    if w == 0
        disp('Mouse clicked, exiting')
        break;
    end
    
    
    w = waitforbuttonpress;
    
    
end
closepreview(stereo(1));
closepreview(stereo(2));


