close all; clear all;

[templ, tempr] = stereo_dump(50);

for i = 1:50
    
    temp = ((i-1)*480)+1
    
    subplot(1, 2,1); image(templ(temp:(480*i),:,:))
    subplot(1, 2,2); image(tempr(temp:(480*i),:,:))

end