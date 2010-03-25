%% Class defs of T-junction

classdef t_junction < node
    properties    
    end
    methods
        function [t] = setType(obj, nodetype)
            obj.type = nodetype;
            t = obj;
        end
    end
end