clear all;

leftcam = videoinput('winvideo', 1);
rightcam = videoinput('winvideo', 2);

leftframe = getsnapshot(leftcam);
rightframe = getsnapshot(rightcam);

figure(1);
image(leftframe);
figure(2);
image(rightframe);
