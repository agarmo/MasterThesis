%% ToF camera dump script

function [] = tof_dump(filename)

    [dev] = sr_open(); % open the connection

    sr_acquire(dev); % start the acquiring process

    xall = [];
    yall = [];
    zall = [];

    for i = 0:300; % the main loop

        [res, x, y, z] = sr_coordtrf(dev);

        %stor the coordinates in the overall array
        xall = [xall, x];
        yall = [yall, y];
        zall = [zall, z];



    end

    sr_close(dev);
    
    %open the file
    
    file = fopen(filename, 'w');
    
    fprintf(file, 'Tof datadump\n\n');
    
    
    
        
    fclose(filename);

end
