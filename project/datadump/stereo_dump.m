function [tempLeft, tempRight] = stereo_dump(iterations)

    % setting up the video objects
    left_cam = videoinput('winvideo', 1);
    right_cam = videoinput('winvideo', 2);

    set(left_cam, 'SelectedSourceName', 'input1')
    set(right_cam, 'SelectedSourceName', 'input1')

    triggerconfig(left_cam, 'manual');
    triggerconfig(right_cam, 'manual');

    start(left_cam);
    start(right_cam);
    
    pause(1);
    
    tempLeft = [];
    tempRight = [];

    for i=1:iterations

        tic;
        left = getsnapshot(left_cam);
        right = getsnapshot(right_cam);
        toc
            
        tempLeft = [tempLeft; left];
        tempRight = [tempRight; right];

    end

    trigger(left_cam);
    trigger(right_cam);
    
    imwrite(tempLeft, 'left.bmp', 'bmp');
    imwrite(tempRight, 'right.bmp', 'bmp');

end
