%% This is the class definition for the genreal node in the world
%% representation. 
% The plan is to make this class a super class and let the different types
% of nodes inherit form this one.

classdef node
    properties
        type = 'Super Node';
        number = 0;
        prev_node;
        discovered;
        diameter;
        dist_prev_node;
        anomalies;
    end
    methods
        function isDiscovered(this)
            
        end
    end 
end

