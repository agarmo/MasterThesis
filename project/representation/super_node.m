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
classdef super_node
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
        position;
    end
    
    
    methods
        
        %% constructor
        function [object] = super_node(number, orientation, prev_node, diameter, dist_prev_node)
            if nargin == 0 
                % do nothing, because the felds are empty.
            elseif nargin == 1 % maybe just a struct?
                if isstruct(number)
                    object.number = number.number;
                    object.orientation = number.orientation;
                    object.prev_node = number.prev_node;
                    object.diameter = number.diameter;
                    object.discovered = number.discovered;
                    object.dist_prev_node = number.dist_prev_node;
                    object.type = number.type;
                    object.anomalies = number.anomalies;
                    object.color = number.color;
                    object.number_of_edges = number.number_of_edges;
                    object.position = number.position;
                    object.angles_of_edges = number.angles_of_edges;
                end
                    
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
            if isempty(object.discovered)
                object.discovered = datevec(datestr(now, 0));
            end
        end
        
        
        
        %% Adding anomalies which have been observed during the travel
        
        % TODO need some error check to see that the anomaly is put into the 
        % table correctly.
        function [object] = addAnomaly(object, anomaly)
            object.anomalies = [object.anomalies; anomaly];
        end
        
        %% Set functions

        function [object] = setPostition(object, pos)
            
            if nargin == 1
                object.position = pos;
            else
                disp('Too few arguments');
            end
            
        end
       
        %% display functions
        
        function [object] = draw_at_position(object, pos) % Draws the node at pos x, y using fill
            orientation = deg2rad(object.orientation);
            
            r = [cos(orientation), -sin(orientation);
                 sin(orientation), cos(orientation)];
        
            % Need to check that it is a column vector
            if size(pos) == 2 
                posb = r'*pos'; % transform to body coords
            else
                posb = r'*pos;
            end
            
            
            % need to rotate the box with regard to the orientation
            % transform to global coords
            pos1 = r*(posb + [-1; -1]);
            pos2 = r*(posb + [-1; 1]);
            pos3 = r*(posb + [1; 1]) ;
            pos4 = r*(posb + [1; -1]);
            
            x = [pos1(1), pos2(1), pos3(1), pos4(1)];
            y = [pos1(2), pos2(2), pos3(2), pos4(2)];
            
            hold on; % 
            fill(x, y, object.color);
            hold off;
            
            object.position = pos;
        end
        
        function [] = draw_edges(object)
            r = 1; %set defualt lenght of edges
            
            for i = 1:object.number_of_edges
                angle = deg2rad(object.angles_of_edges(i) + object.orientation);
                
                x = r*cos(angle) + object.position(1);
                y = r*sin(angle) + object.position(2);
             
                hold on;
                plot([object.position(1), x], [object.position(2), y], 'white', 'LineWidth', 3);
                hold off;
            end
        
        end
        
        
        
        function [object] = setPosition(object, position)
            object.position = position;
        end

        
        
        
    end 
end

