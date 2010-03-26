%% Class defs of Left-bend pipe

classdef left_bend < node
    properties
    end
    methods
        function [t] = left_bend(number, prev_node, diameter, dist_prev_node)
            
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
            
            t.color = 'black';
            t.type = 'Left Bend';           
        end
    end
end

