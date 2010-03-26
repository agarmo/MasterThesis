%% Class defs of right-bend pipe

classdef right_bend < node
    properties
    end
    methods
        function [t] = right_bend(number, orientation, prev_node, diameter, dist_prev_node)
            
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
            t = t@node(args{:});
            
            %% class specific
            
            t.color = 'red';
            t.type = 'Right Bend';
        end
    end
end

