%% Class defs for the start node

classdef start_node < super_node
    properties
    end
    methods
        
        function [t] = start_node(orientation, diameter)
            
            %% Default constuctor
            if nargin == 0
                args = {};
            else
                args{1} = 1;
                args{3} = [];
                args{5} = [];
                if ~isempty(orientation)
                    args{2} = orientation;
                else 
                    args{2} = [];
                end
                if ~isempty(diameter)
                    args{4} = diameter;
                else
                    args{4} = [];
                end
                
            end
            
            %% calling super constructor.
            t = t@super_node(args{:});
            
            %% class specific
            
            t.color = 'yellow';
            t.type = 'Start Node';
            
            t.number_of_edges = 1;
            t.angles_of_edges = 180; %standard  
        end
        
        function [lhs] = super_node(rhs)
            %% converting function
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
        
 
       
        function [t] = setType(obj, nodetype)
            obj.type = nodetype;
            t = obj;
        end
    end
end