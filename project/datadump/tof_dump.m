%% ToF camera dump script

function [xall, yall, zall] = tof_dump(filename, iterations)

    [dev] = sr_open(); % open the connection

    xall = [];
    yall = [];
    zall = [];
    amplitude = [];
    
    tic;
    for i = 0:iterations; % the main loop

        sr_acquire(dev); % start the acquiring process
        
        [res, x, y, z] = sr_coordtrf(dev);
        ampimg = sr_getimage(ToF,1);

        %stor the coordinates in the overall array
        xall = [xall; x];
        yall = [yall; y];
        zall = [zall; z];
        amplitude = [amplitude; ampimg];

    end
    sr_close(dev);
    toc
    
    temp = [xall; yall; zall];

    disp('Writing to file...')
    csvwrite(filename, temp );
    
    s = sprintf('amp%s', filename);
    csvwrite(s, amplitude);
    
    
end
