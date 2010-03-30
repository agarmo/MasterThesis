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
        
        function [object] = addNode(object, node, position)        
            % When adding nodes, they need to be of the same type. Then it
            % should be easiest to convert them to the super class, node to
            % fit all the nodes into the same array.
            if nargin < 3
                disp('Too few arguments');
            else
                if ~strcmp(class(node), 'super_node')
                    % convert the node to super node and put into array
                    
                    node = node.super_node(); %convert the node to the super class node
                    node = node.setPosition(position);
                    
                    object.nodes = [object.nodes; node]; % add the node to the list
                    %object.nodes(end) = object.nodes(end).draw_at_position(position);
                else
                    % is of type node.
                    node = node.setPosition(position);
                    object.nodes = [object.nodes; node]; % add the node to the list
                    %object.nodes(end) = object.nodes(end).draw_at_position(position);
                end
            end
        end
              
                
        function [] = draw(object)
            
            for i = 1:length(object.nodes)
                object.nodes(i) = object.nodes(i).draw_at_position(object.nodes(i).position);
                object.nodes(i).draw_edges(); % indicate what kind of junction it is.
            end
        end
        
        
        function [] = draw_edges(object)
            % algorithm: 1. find which nodes is connected to who.
            %            2. draw the edges between them, with according
            %            distances
            %            3. Draw the edges which are not connected to any
            %            4. finished
            % extra: Need to handle faults and errors, check if the
            % distance is correct. Check if there really is an edge at the
            % exiting at the bearing allowed by the node in question.
            
            for i = 1:length(object.nodes) % Assumes that the nodes array are sorted 
           
                if i ~= length(object.nodes)
                    distance = object.nodes(i+1).dist_prev_node;
                    
                    % draw the edges
                    
                    hold on
                    plot([object.nodes(i).position(1), object.nodes(i+1).position(1)],...
                        [object.nodes(i).position(2), object.nodes(i+1).position(2)]);
                    hold off;
                                        
                else
                    distance = 0; %no noeds after this one.
                    
                    disp('Last node in array');
                end
                
                
                
            end
            
            
        end
        
        
    end
end
