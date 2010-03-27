%% Class defs for the start node

classdef start_node < node
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
                end
                if ~isempty(diameter)
                    args{4} = diameter;
                end
                
            end
            
            %% calling super constructor.
            t = t@node(args{:});
            
            %% class specific
            
            t.color = 'yellow';
            t.type = 'Start Node';
            
            t.number_of_edges = 1;
            t.angles_of_edges = 180; %standard  
        end
       
        function [t] = setType(obj, nodetype)
            obj.type = nodetype;
            t = obj;
        end
    end
end