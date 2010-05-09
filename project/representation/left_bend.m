%% Class defs of Left-bend pipe

classdef left_bend < super_node
    properties
    end
    methods
        function [t] = left_bend(number, orientation, prev_node, diameter, dist_prev_node)
            
            %% Default constuctor
            if nargin == 0
                args = {};
            else
                if ~isempty(number)
                    args{1} = number;
                end
                if ~isempty(orientation)
                    args{2} = orientation;
                end
                if ~isempty(prev_node)
                    args{3} = prev_node;
                end
                if ~isempty(diameter)
                    args{4} = diameter;
                end
                if ~isempty(dist_prev_node)
                    args{5} = dist_prev_node;
                end
            end
            %% calling super constructor.
            t = t@super_node(args{:});
            
            %% class specific
            
            t.color = 'black';
            t.type = 'Left Bend';           
            
            t.number_of_edges = 2;
            t.angles_of_edges = [0, 270];
         
            
        end
        function [lhs] = super_node(rhs)
            %% converting function
            %transforming to a struct first, then running the super class
            %consturctor on the sturct
            
            lhs.number = rhs.number;
            lhs.type = rhs.type;
            lhs.orientation = rhs.orientation;
            lhs.prev_node = rhs.prev_node;
            lhs.discovered = rhs.discovered;
            lhs.diameter = rhs.diameter;
            lhs.dist_prev_node = rhs.dist_prev_node;
            lhs.anomalies = rhs.anomalies;
            lhs.color = rhs.color;
            lhs.number_of_edges = rhs.number_of_edges;
            lhs.angles_of_edges = rhs.angles_of_edges;
            lhs.position = rhs.position;
            
            lhs = super_node(lhs);            
        end

    end
end

