%% Class defs of T-junction

classdef t_junction < node
    properties    
    end
    methods
        
        function [t] = t_junction(number, prev_node, diameter, dist_prev_node)
            
            %% Default constuctor
            if nargin == 0
                args = {};
            else
                if ~isempty(number)
                    args{1} = number;
                end
                if ~isempty(prev_node)
                    args{2} = prev_node;
                end
                if ~isempty(diameter)
                    args{3} = diameter;
                end
                if ~isempty(dist_prev_node)
                    args{4} = dist_prev_node;
                end
            end
            
            %% calling super constructor.
            t = t@node(args{:});
            
            %% class specific
            
            t.color = 'green';
            t.type = 'T Junction';          
        end
       
        function [t] = setType(obj, nodetype)
            obj.type = nodetype;
            t = obj;
        end
    end
end