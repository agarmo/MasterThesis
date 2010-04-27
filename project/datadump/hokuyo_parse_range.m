function [ranges] = hokuyo_parse_range(vargin, start, iterations)

    theta = (360/1024)*(pi/180).*(1:1024);

    ranges = [];    
    
    for i = start:iterations
        ranges = [ranges; [theta; vargin(i, :)]];
    end    
    
    polar(ranges(1,:), ranges(2,:));
    title('Plot of the URG Laser Range finder');
    
    

end