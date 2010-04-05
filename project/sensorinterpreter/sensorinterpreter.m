%% Sensor Interpreter
%
% This is the class object which is resposible for recognizing the sensor
% output. 
% Output form this class is:    - Node Type
%                               - Position and Orientation
%                               - Distance traveled
%                               - Velocity
%                               - Distance to walls (?)
%                               - Register anomalies and irregualrities


classdef sensorinterpreter 
   
    properties(GetAccess = public, SetAccess = protected)
        %% The sensor readings.
        LR_data; % input: sample number and range in mm
        ToF_data; % input: point cloud, x,y,z and intensity
        Stereo_data; % input: x,y,z of features
    
        %% Output data Whenever recognized
        recognized_node_struct; % A struct containing the data needed for the world object to draw the node.
        registered_anomalies_struct; % A struct containing details about the detected anomaly.
        
        %% Continous output data
        velocities; % size 3, 1 
        dist_to_walls; % size 2, 1 
        
        verden;
    end
    
    properties(GetAccess = private, SetAccess = private)
        %% Internal properties
        pipeline_profiles; % The profiles to match the sensor data to
        
    end
    
    methods
        %% Constructor
        
        function object = sensorinterpreter(args)
           
            if nargin < 1 % default constuctor
                object.velocities = zeros(3,1);
                object.dist_to_walls = zeros(2,1);
                
                
            elseif nargin == 2
                
                
            end
            
            
        end
        
        
        
        %% Internal Calculation Function
        
        function [] = fuseSensors(object, LRF, ToF)
            
        end
        
        
        function type = matchPipeProfile(object)
            
        end
        
        % This function should try to fit a cylinder to the acquired data
        % from the sensors, to match the pipe. Anomalies are large
        % deviations from this ideal cylinder.
        function [radius, certainty] = curveFitCylinder(object, LRF, ToF)
            
        end
        
        
        function [radius, certanty] = curveFitCone(object, LRF, ToF)
            
        end
        
        
        
        %% Output function
        
        % This shows what the view should be like with the previous sensor
        % readings. This should open a plot with the the given view.
        function [] = showSynthesizedView(object)
            
            % create a cylinder with default radius.
            if nargin > 1
                [X, Y, Z] = cylinder(sqrt(object.verden.pipe_diameter));
                figure(1);
                cylinder(sqrt(object.verden.pipe_diameter), 64)
%                 axes('Projection', 'perspective');
            else
                [X, Y, Z] = cylinder(sqrt(10));
                figure(1)
                cylinder(sqrt(10), 64)
%                 axes('Projection', 'perspective');
                
            end
            
            % Set the view from the current position.
            
                        
        end
        
        % this creates the real view from the sensors at the given moment.
        function [] = showRealView(object)
            
            
        end
        
        
        %% Other functions
        
        function verden = setWorld(object, world)
           verden = world; 
        end
        
        
    end
    
end