%% World class, containing all the nodes. 
% This is the container class for all the nodes stored in the world
% 


classdef world
    properties %(SetAccess = private) % ?
    %% private attributes?
        nodes; % the nodes that are discovered. 
        pipe_diameter; % The diameter of the pipeworld
    end
    
    properties
    %% public attributes?
    
    end
    
    methods

        %%  Constuctor
        function [object] = world(diameter)
            
            object.nodes = []; % create an empty list.
            
            if nargin == 1
                start = start_node(0, diameter); % create a start node
                object.pipe_diameter = diameter;
            else
                start = start_node(0, 10);
                object.pipe_diameter = 10;
            end
            
%             object.nodes = [start];
%             object.nodes(1) = object.nodes(1).draw_at_position([0;0]);
%                                     
            object = object.addNode(start, [0; 0]);
        end
        
        %% Othre functions
        % add node function for adding nodes to the world
        % 
        % When adding nodes, they need to be of the same type. Then it
        % should be easiest to convert them to the super class, node to
        % fit all the nodes into the same array.
        
        function [object] = addNode(object, node, position)
            if nargin < 2
                disp('Too few arguments');
            else
                if ~strcmp(class(node), 'super_node')
                    % convert the node to super node and put into array
                    
                    node = node.super_node(); %convert the node to the super class node
                    
                    object.nodes = [object.nodes; node]; % add the node to the list
                    %object.nodes(end) = object.nodes(end).draw_at_position(position);
                else
                    % is of type node.
                    
                    object.nodes = [object.nodes; node]; % add the node to the list
                    %object.nodes(end) = object.nodes(end).draw_at_position(position);
                end
            end
        end
        
    end
end
