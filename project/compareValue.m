% determines if the source value is whihtin some threshold of value. 

function result = compareValue(radius, value, threshold)

    if (radius >= value-threshold) && (radius <= value+threshold)
        result = 0;
        return
    elseif radius > (value+threshold)
        result = 1;
        return
    elseif radius < (value-threshold)
        result = -1;
        return;
    else
        result = inf;
    end

end