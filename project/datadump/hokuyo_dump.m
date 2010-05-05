%% script for dumping hokuyo data to disk
function [temp] = hokuyo_dump(file, com, baud, iterations)

    hokuyo_matlab('open', com, baud); % open connection
    
    temp = [];
    tic;
    for i=1:iterations
        
        [h1] = hokuyo_matlab('getReading');
        
        h1 = [zeros(128,1);h1; zeros((768-726)+128, 1)];
        
        temp = [temp; h1'];
                
    end
    hokuyo_matlab('close');
    toc
    
    disp('Writing to file...');
    csvwrite(file, temp);
    
    
end
