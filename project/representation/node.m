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
%           orientation: The orientation of the node, relative to NED
% List of methods:
%           
classdef node
    properties
        type;
        number;
        orientation;
        prev_node;
        discovered;
        diameter;
        dist_prev_node;
        anomalies;
        color;
%     end
    
%     properties(GetAccess = protected, SetAccess = protected) % Drawing properties
        number_of_edges; % The number of edges connecting to the node
        angles_of_edges; % array with the angles of the connected edges. Relative to NED
        x; % x position of the node when drawing
        y; % y position of the node when drawing
    end
    
    
    methods
        
        %% constructor
        function [object] = node(number, orientation, prev_node, diameter, dist_prev_node)
            if nargin == 0 
                % do nothing, because the felds are empty.
            else
                if ~isempty(number)
                    object.number = number;
                else
                    object.number = [];
                end
                if ~isempty(orientation)
                    object.orientation = orientation;
                else
                    object.orientation = [];
                end
                if ~isempty(diameter)
                    object.diameter = diameter;
                else
                    object.diameter = [];
                end
                if ~isempty(prev_node)
                    object.prev_node = prev_node;
                else
                    object.prev_node = [];
                end
                if ~isempty(dist_prev_node)
                    object.dist_prev_node = dist_prev_node;
                else
                    object.dist_prev_node = [];
                end
            end 
            object.discovered = datevec(datestr(now, 0));
        end
        
        
        
        %% Adding anomalies which have been observed during the travel
        
        % TODO need some error check to see that the anomaly is put into the 
        % table correctly.
        function [object] = addAnomaly(object, anomaly)
            object.anomalies = [object.anomalies; anomaly];
        end
        
        %% Set functions

        function [object] = setPostition(object, x, y)
            object.x = x;
            object.y = y;
        end
       
        %% display functions
        
        function [object] = draw_at_position(object, pos_x, pos_y) % Draws the node at pos x, y using fill
            orientation = deg2rad(object.orientation);
            
            r = [cos(orientation), -sin(orientation);
                 sin(orientation), cos(orientation)];

            posb = r'*[pos_x; pos_y]; % transform to body coords
            
            x = [posb(1)-1; posb(1)-1; posb(1)+1; posb(1)+1];
            y = [posb(2)-1; posb(2)+1; posb(2)+1; posb(2)-1];
            
            % need to rotate the box with regard to the orientation
            % transform to global coords
            pos1 = r*[x(1); y(1)];
            pos2 = r*[x(2); y(2)];
            pos3 = r*[x(3); y(3)];
            pos4 = r*[x(4); y(4)];
            
            x = [pos1(1), pos2(1), pos3(1), pos4(1)];
            y = [pos1(2), pos2(2), pos3(2), pos4(2)];
            
            hold on; % 
            fill(x, y, object.color);
            hold off;
            object.x = pos_x;
            object.y = pos_y;
        end
        
        function [] = draw_edges(object)
            r = 10; %set defualt lenght of edges
            
            for i = 1:object.number_of_edges
                angle = deg2rad(object.angles_of_edges(i) + object.orientation);
                
                x = r*cos(angle) + object.x;
                y = r*sin(angle) + object.y;
             
                hold on;
                plot([object.x, x], [object.y, y]);
                hold off;
            end
        
        end
        
        
        
        
        
    end 
end

