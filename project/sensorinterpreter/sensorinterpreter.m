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
        LR_data;
        ToF_data;
        Stereo_data;
    
        %% Output data Whenever recognized
        recognized_node_struct; % A struct containing the data needed for the world object to draw the node.
        registered_anomalies_struct; % A struct containing details about the detected anomaly.
        
        %% Continous output data
        velocities; % size 3, 1 
        dist_to_walls; % size 2, 1 
    end
    
    properties(GetAccess = private, SetAccess = private)
        %% Internal properties
        
        
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
        
        
        
        %% Output function
        
        
        
        %% Other functions
        
        
        
        
    end
    
end