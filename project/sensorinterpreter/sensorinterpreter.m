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
        
        
        
        %% Output function
        
        
        
        %% Other functions
        
        
        
        
    end
    
end