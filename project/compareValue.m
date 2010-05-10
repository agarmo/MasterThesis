% determines if the source value is whihtin some threshold of value. 

function result = compareValue(source, value, threshold)

    if (source >= value-threshold) && (source <= value+threshold)
        result = 0;
        return
    elseif (source >= value+threshold)
        result = 1;
        return
    elseif (source <= value-threshold)
        result = -1;
        return;
    end

end