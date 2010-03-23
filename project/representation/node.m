%% Class definition for the genreal node in the world representation. 
% The plan is to make this class a super class and let the different types
% of nodes inherit form this one.
% Description of the properties:
%           type: the type of the node.
%           number: is the number in the node, usually describing the
%                   number in which it is discovered
%           prev_node: is the link to the previous visited node.
%           discovered: is the timestamp when the node was recognized
%           diameter: the estimated diameter of the pipe
%           dist_prev_node: is the estimated distance to the previous node.
%           anomalies: A list of anomalies discovered between this node and
%                      the previous one,
% List of methods:
%           
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

