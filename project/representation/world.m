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
        function [object] = world(object)
            
            object.nodes = []; % create an empty list.
            
            start = start_node(0, 10); % create a start node
            
            object.nodes = [start];
            object.nodes(1) = object.nodes(1).draw_at_position(0,0);
                                    
        end
        
        %% Othre functions
        % add node function for adding nodes to the world
        
        function [object] = addNode(object, node, x, y)
            
            object.nodes = [object.nodes; node]; % add the node to the list
            object.nodes(end) = object.nodes(end).draw_at_position(x, y);
            
        end
        
    end
end
