%% Class defs of T-junction

classdef t_junction < node
    properties    
    end
    methods
        %Default constuctor
        function [t] = t_junction(t)
            t.color = 'green';
            t.type = 'T Junction';
%             t.diameter = constant_pipe_diameter;
        end
         %constructor which is run when a new node is recognized
%         function [t] = t_junction(t, time, prev_node)
%             t.color = 'green';
%             t.prev_node = prev_node;
%             t.type = 'T Junction';
%             t.timestamp = time;
%         end
        
        function [t] = setType(obj, nodetype)
            obj.type = nodetype;
            t = obj;
        end
    end
end