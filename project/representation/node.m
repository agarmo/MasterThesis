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
        type;
        number;
        prev_node;
        discovered;
        diameter;
        dist_prev_node;
        anomalies;
        color;
    end
    methods
        
        function [] = draw(object, pos_x, pos_y) % Draws the node at pos x, y using fill
            x = [pos_x-1; pos_x-1; pos_x+1; pos_x+1];
            y = [pos_y-1; pos_y+1; pos_y+1; pos_y-1];
            hold on; % 
            fill(x, y, object.color);
            hold off;
        end
        % need some error check to see that the anomaly is put into the 
        % table correctly.
        function [object] = addAnomaly(object, anomaly)            
            object.anomalies = [object.anomalies; anomaly];
        end
    end 
end


