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
        
        %% Output data Whenever recognized
        recognized_node_struct; % A struct containing the data needed for the world object to draw the node.
        registered_anomalies_struct; % A struct containing details about the detected anomaly.
        
        %% Continous output data
        velocities; % size 3, 1 
        dist_to_walls; % size 2, 1 
        
        verden; % binding til verdenen over.
%     end
    
    %properties(GetAccess = private, SetAccess = private)
        %% Internal properties
        pipeline_profiles; % The profiles to match the sensor data to
    
        %% The sensor readings.
        LRF_data; % input: angles and ranges 2xN
        LRF_paramsy; % structs for holding data asociated with line fit
        LRF_paramsx; 
        LRF_height; % The height of the measurement plane.
        
%         FittedStructures_2D; % Structures for gathering 2d structures
%         FittedStructures_3D; % structures for gathering 3d Structures
        
        ToF_data; % input: point cloud, x,y,z Mx3
        ToF_params; % struct for the cylinder fits
        a0; % assumed direction of the pipe. 
        interval; %tof cylinder fit interval
        
        Stereo_data; % input: x,y,z of features
    
    end
    
    methods
        %% Constructor
        
        function object = sensorinterpreter(args)
           
            if nargin < 1 % default constuctor
                object.velocities = zeros(3,1);
                object.dist_to_walls = zeros(2,1);
                
                
                object.LRF_paramsy = struct('x0_urg', [], 'a_urg' ,[],...
                    'd_urg' , [], 'normd_urg', [],'x_urg' , [],'y_urg' , [],...
                    'histrange', []);
                
                object.LRF_paramsx = struct('x0_urg', [], 'a_urg' ,[],...
                    'd_urg' , [], 'normd_urg', [],'x_urg' , [],'y_urg' , [],...
                    'histrange', []);
            
                object.ToF_params = struct('x0k', [], 'ank',  [], 'rk', [],...
                    'dk', [], 'length_d', [], 'a_parmk', [], 'x0', []);
                            
                object.LRF_height = 0.1; % 10 cm above ground.
                
            elseif nargin == 2
                
                
            end
        end
        
        %% Asociating sensors to the object.
        
        function this = setLRFData(this, LRF_data)
            if (size(LRF_data, 1) ~= 2) || isempty(LRF_data)
                error('The LRF-data should be on the form 2xM')
            else
                this.LRF_data = LRF_data;
            end
        end
        
        function this = setToFData(this, ToF_data)
            if (size(ToF_data, 2) ~= 3) || isempty(ToF_data) 
                error('The LRF-data should be on the form Mx3')
            else
                this.ToF_data = ToF_data; 
            end
        end
        
        
        %% Internal Calculation Function
        
        function [radius, closestRadius, indexMin] = fuseSensors(this, threshold)
            % find pralell lines, start with lines assumed along the pipe.
            directions = this.LRF_paramsy.a_urg;
            
            equalToLine1 = zeros(size(directions,1),1);
            
            %should start with line nearest to origin.
            
            %compare the first with the others
            for i = 2:size(directions, 1)
                if compareValue(directions(i,1), directions(1,1), threshold) == 0
                    if compareValue(directions(i,2), directions(1,2), threshold) == 0
                        disp('line 1 and line %i are within the threshold')
                        equalToLine1(i) = 1;
                    else
                        disp('Not equal');
                    end
                end
            end
            
            %then find the ones that are closes togheter
            
            distanceToLine1 = zeros(size(directions,1),1);
            distanceToLine1(1) = Inf;
            for i = 2:size(directions, 1)
                if equalToLine1(i) == 1
                    %calculat the distance from centeroids at x=0
                    distanceToLine1(i) = norm([0, this.LRF_paramsy.x0_urg(i,2)]-...
                        [0, this.LRF_paramsy.x0_urg(1,2)]);
                end
            end
            
            %find the smalles distance assume this is at the center of the
            %pipe. 
            [minDist, indexMin] = min(distanceToLine1);
            if minDist == 0
                disp('No paralell lines found');
            else
                disp('Found paralell lines');
                radius = this.LRF_height/2 + (minDist^2)/(8*this.LRF_height);
                indexMin
                distanceToLine1
                
                closestRadius = inf;
                
                %compare the distance to the calculated radiuses.
                for i = 1:size(this.ToF_params.rk,1)
                    switch compareValue(this.ToF_params.rk(i), radius, threshold/10)
                        case 0
                            disp('Found a radius whitihn 10 % of threshold of radius');
                            
                            closestRadius = this.ToF_params.rk(i);
                        case -1
                            disp('Found a radius smaller than radius');
                            if abs(radius - this.ToF_params.rk(i)) < 0.03
                                closestRadius = this.ToF_params.rk(i);
                            end
                        case 1
                            disp('Found a radius larger than radius')
                            if abs(this.ToF_params.rk(i) - radius) < 0.03
                                closestRadius = this.ToF_params.rk(i);
                            end
                        otherwise
                            disp('Unkonwn error')
                    end
                end
                
                % Add the cylinder to the back of the ToF_params.
                
                % find direction
                
               
            end
            
                        
        end
        
        
        
        function type = matchPipeProfile(this)
            
        end
        
        
        %% find lines in 2d data
        function this = find2Dlines(this, number_points_x, number_points_y,...
                 histogram_interval_x, histogram_interval_y)
            
            if nargin ~= 5
                error('Too few arguments. Usage: find2DLines(num_point_x, num_points_x, histx_int, hitsy_int')
            else
                % start interpreting the data.
                %transform to cartesian coordinates
                [urgx, urgy] = pol2cart(this.LRF_data(1,:), this.LRF_data(2, :));
                
                % sort on x descending order
                sorted = sortrows([urgx', urgy'], -1);
                
                temp = sorted;
                temp(~any(sorted,2),:)=[]; %% remove trivial points (0,0)
                
                %form histogram ranges and calculate histograms
                this.LRF_paramsy.histrange = (-4.6:histogram_interval_y:4.7)';
                this.LRF_paramsx.histrange = (-4.6:histogram_interval_x:4.7)';
                
                [nx] = hist(temp(:,1), this.LRF_paramsx.histrange);
                [ny] = hist(temp(:,2), this.LRF_paramsy.histrange);
                
                for k = 1:size(this.LRF_paramsy.histrange)
                    
                    threshold = this.LRF_paramsy.histrange(k);
                    % horizontal lines. from the parallell to the y-axis
                    
                    data = [];
                    if ny(k) > number_points_y
                        for i = 1:size(temp(:,2))
                            if (temp(i,2) > threshold-histogram_interval_y) && ...
                                    (temp(i,2) < threshold+histogram_interval_y)
                                data = [data; temp(i,:)];
                            end
                        end
                    end
                    
                    %look for different shapes, arcs, lines, parallell to the y-axis
                    if (~isempty(data)) && (size(data, 1) > 2)
                        [x0_urgt, a_urgt, d_urgt, normd_urgt] = ls2dline(data);
                        
                        this.LRF_paramsy.x0_urg = [this.LRF_paramsy.x0_urg; x0_urgt'];
                        this.LRF_paramsy.a_urg = [this.LRF_paramsy.a_urg; a_urgt'];
                        this.LRF_paramsy.d_urg = [this.LRF_paramsy.d_urg; zeros(50,1); d_urgt]; %%adding zeros to see where the new line starts
                        this.LRF_paramsy.normd_urg =[this.LRF_paramsy.normd_urg; normd_urgt];
                        
                        %calculate the line
                        
                        %scaling
                        startline = -x0_urgt(1)+ min(data(:,1));
                        t = sqrt((max(data(:,1))-min(data(:,1)))^2 + (max(data(:,2))-min(data(:,2)))^2) ;
                        
                        x_urgt_s = (x0_urgt(1) + ((a_urgt(1)*startline)));
                        y_urgt_s = (x0_urgt(2) + ((a_urgt(2)*startline)));
                        
                        x_urgt_l = (x0_urgt(1) + (a_urgt(1)*t));
                        y_urgt_l = (x0_urgt(2) + (a_urgt(2)*t));
                        
                        
                        % assign start and stop points to the y-struct
                        this.LRF_paramsy.x_urg = [this.LRF_paramsy.x_urg; [x_urgt_s, x_urgt_l]];
                        this.LRF_paramsy.y_urg = [this.LRF_paramsy.y_urg; [y_urgt_s, y_urgt_l]];
                    end
                end
                
                % look along the x-axis
                for k = 1:size(this.LRF_paramsx.histrange)
                    threshold = this.LRF_paramsx.histrange(k);
                    datax = [];
                    if nx(k) > number_points_x
                        for i = 1:size(temp(:,2))
                            if (temp(i,1) > threshold-histogram_interval_x) && ...
                                    (temp(i,1) < threshold+histogram_interval_x)
                                
                                datax = [datax; temp(i,:)];
                            end
                        end
                    end
                    if (~isempty(datax)) && (size(datax, 1) > 2)
                        [x0_urgtx, a_urgtx, d_urgtx, normd_urgtx] = ls2dline(datax);
                        this.LRF_paramsx.x0_urg = [this.LRF_paramsx.x0_urg; x0_urgtx'];
                        this.LRF_paramsx.a_urg = [this.LRF_paramsx.a_urg; a_urgtx'];
                        this.LRF_paramsx.d_urg = [this.LRF_paramsx.d_urg; zeros(50,1); d_urgtx];
                        this.LRF_paramsx.normd_urg =[this.LRF_paramsx.normd_urg; normd_urgtx];
                        
                        %calculate the line
                        
                        %scaling
                        startline = -x0_urgtx(2)+ min(datax(:,2));
                        t = sqrt((max(datax(:,1))-min(datax(:,1)))^2 + (max(datax(:,2))-min(datax(:,2)))^2) ;
                        
                        x_urgt_s = (x0_urgtx(1) + ((a_urgtx(1)*startline)));
                        y_urgt_s = (x0_urgtx(2) + ((a_urgtx(2)*startline)));
                        
                        x_urgt_l = (x0_urgtx(1) + (a_urgtx(1)*t));
                        y_urgt_l = (x0_urgtx(2) + (a_urgtx(2)*t));
                        
                        this.LRF_paramsx.x_urg = [this.LRF_paramsx.x_urg; [x_urgt_s, x_urgt_l]];
                        this.LRF_paramsx.y_urg = [this.LRF_paramsx.y_urg; [y_urgt_s, y_urgt_l]];
                    end
                end
            end
        end
        
        
        function this = find3Dcylinders(this, interval, a0)

            this.a0 = a0;
            this.interval = interval;
            
            % sort rows and remove trivial points
            this.ToF_data= sortrows(this.ToF_data, 3); % sort the vector on z value.
            temp = this.ToF_data;
            temp(~any(this.ToF_data,2),:)=[]; %% remove trivial points
            this.ToF_data = temp;
            
            % Calculate the interval
            startz = this.ToF_data(1,3);
            stopz = this.ToF_data(size(this.ToF_data, 1), 3);
            
            pieces = ceil((stopz-startz)/interval); % round upwards toward nearest integer to ensure all values
            
            
            for k = 1:pieces
                %% Select data from total selection
                
                temp = [];
                for i = 1:size(this.ToF_data,1) % might be optimized because of the sorted array
                    if (this.ToF_data(i, 3) >= interval*(k-1)) && (this.ToF_data(i, 3) <= interval*k)
                        temp = [temp; this.ToF_data(i, :)];
                    end
                end
                
                
                
                %% Start the surface fit
                
                this.ToF_params.x0 = [this.ToF_params.x0; 0,0,interval*(k-1)];
                
                % [x0n, an, phin, rn, d, sigmah, conv, Vx0n, Van, uphin, urn, ...
                % GNlog, a, R0, R] = lscone(temp, x0, a0, angle, radius, 0.1, 0.1);
                
                if (isempty(temp) ) || (size(temp,1) < 5)
                    warning('Too few points')
                else
                    % Start cylinder fit using gauss-newton
                    [x0n, an, rn, d, sigmah, conv, Vx0n, Van, urn, GNlog, a] = ...
                        lscylinder(temp, (this.ToF_params.x0(k, :))', a0,...
                        this.verden.pipe_diameter/2, .001, .001);
                    
                    this.ToF_params.x0k = [this.ToF_params.x0k; x0n'];
                    this.ToF_params.ank = [this.ToF_params.ank; an'];
                    this.ToF_params.rk = [this.ToF_params.rk; rn];
                    this.ToF_params.dk = [this.ToF_params.dk; d];
                    this.ToF_params.length_d = [this.ToF_params.length_d; size(d,1)];
                    this.ToF_params.a_parmk = [this.ToF_params.a_parmk; a'];
                end
                
            end
        end
        
        
        
        %% Output function
        
        % This shows what the view should be like with the previous sensor
        % readings. This should open a plot with the the given view.
        function [rot_axis, rot_angle] = showSynthesizedView(this)
            
            rot_axis = zeros(3, size(this.ToF_params.x0k, 1));
            rot_angle = zeros(size(this.ToF_params.x0, 1), 1);
            
            figure;
            set(gcf, 'Renderer', 'opengl');
            test = plot3(this.ToF_data(:,3), this.ToF_data(:,1), this.ToF_data(:,2), '.');
            hold on;
            for k = 1:size(this.ToF_params.x0k, 1)
                
                if this.ToF_params.rk(k) > 2*(this.verden.pipe_diameter)/2
                    warning('Radius errenous. Skipping...');
                else
                    %% start constructing the cylinder from the given parameters.
                    [X, Y, Z] = cylinder(this.ToF_params.rk(k)*ones(2,1), 125); % plot the cone.
                    
                    
                    % find rotation axis and transformation matrix
                    rot_axis(:, k) = cross(this.a0,this.ToF_params.ank(k,:)');
                    if norm(rot_axis(:,k)) > eps
                        rot_angle(k) = asin(norm(rot_axis(:,k)));
                        if(dot(this.a0, this.ToF_params.ank(k,:)')< 0)
                            rot_angle(k) = pi-rot_angle(k);
                        end
                    else
                        rot_axis(:,k) = this.a0;
                        rot_angle(k) = 0;
                    end
                    
                    Sz = makehgtform('scale',[1;1;this.interval] );
                    Txyz = makehgtform('translate', this.ToF_params.x0k(k,:));
                    Rxyz = makehgtform('axisrotate', rot_axis(:,k), rot_angle(k));
                    Tz = makehgtform('translate', [0;0;-(this.interval/2)]);
                    
                    tf = Txyz*Rxyz*Sz;
                    
                    Xny = zeros(2, size(X,2));
                    Yny = zeros(2, size(Y,2));
                    Zny = zeros(2, size(Z,2));
                    % Transform cylinder to right scale and position
                    for i = 1:126
                        for j = 1:2
                            Xny(j, i) = (tf(1, 1:4)*[X(j,i); Y(j,i); Z(j,i); 1]);
                            Yny(j, i) = (tf(2, 1:4)*[X(j,i); Y(j,i); Z(j,i); 1]);
                            Zny(j, i) = (tf(3, 1:4)*[X(j,i); Y(j,i); Z(j,i); 1]);
                        end
                    end
               
                    h(k) = surface(Zny./tf(4,4), Xny./tf(4,4), Yny./tf(4,4));
                    
                    
                end
                
            end
            
            hold off;
            axis equal;
            xlabel('Depth');
            ylabel('Camera X-direction');
            zlabel('Camera Y-direction');
            grid on
            
            figure;
            plot(this.ToF_params.dk);
            title('Distance of points from the fitted cylinder');
            hold on
            plot([this.ToF_params.length_d; size(this.ToF_params.dk,1)],...
                zeros(1, size(this.ToF_params.length_d,1)+1), '+r', 'MarkerSize', 10);
            hold off;
            
%             figure;
%             plot(this.ToF_params.dk, this.ToF_data(:,3)); % distribution of the points along the z-axis
%             xlabel('Length from point on cylinder [m]');
%             ylabel('Detpth into the pipe, Z-axis [m]');
%             grid on
%             
        end
        
        % this creates the real view from the sensors at the given moment.
        function [] = plot2Dlines(this)
            
            [x, y ]= pol2cart(this.LRF_data(1,:), this.LRF_data(2,:));
            figure;
            subplot(2, 2, 1:2);
            plot(x, y, 'b.');
            hold on;
            grid on;
            xlabel('Depth into the pipe Z-axis [m]');
            ylabel('X-axis relative to ToF-camera [m]');
            title('URG Laser Range Finder');
            
            for i = 1:size(this.LRF_paramsy.x_urg,1)
                line([this.LRF_paramsy.x_urg(i,1);this.LRF_paramsy.x_urg(i,2)],...
                    [this.LRF_paramsy.y_urg(i,1);this.LRF_paramsy.y_urg(i,2)], 'Color', 'magenta', ...
                    'LineWidth', 1.5);
                %plot centroid
                plot(this.LRF_paramsy.x0_urg(i,1), this.LRF_paramsy.x0_urg(i,2), 'k*');
            end
            
            for i = 1:size(this.LRF_paramsx.x_urg,1)
                line([this.LRF_paramsx.x_urg(i,1);this.LRF_paramsx.x_urg(i,2)],...
                    [this.LRF_paramsx.y_urg(i,1);this.LRF_paramsx.y_urg(i,2)], 'Color', 'red', ...
                    'LineWidth', 1.5);
                %plot centroid
                plot(this.LRF_paramsx.x0_urg(i,1), this.LRF_paramsx.x0_urg(i,2), 'g*');
            end
            hold off;
            
            
            subplot(2, 2, 3);
            plot(this.LRF_paramsy.d_urg);
            title('Error from the fitted lines Horizontal');
            grid on;
            
            subplot(2, 2, 4);
            plot(this.LRF_paramsx.d_urg);
            title('Error from the fitted lines Vertical');
            grid on;
            
            
            figure;
            subplot(1, 2, 1)
            hist(x, this.LRF_paramsx.histrange);
            title('Histogram of Vertical pixels');
            xlabel('Position depth into the pipe (camera z-axis) [m]');
            
            subplot(1, 2, 2);
            hist(y, this.LRF_paramsy.histrange);
            title('Histogram of Horizontal pixels');
            xlabel('Position lateral position into the pipe(camera x-axis) [m]');
            
            
        end
        
        
        %% Other functions
        
        function this = setWorld(this, world)
           this.verden = world; 
        end
             
        
    end
    
end