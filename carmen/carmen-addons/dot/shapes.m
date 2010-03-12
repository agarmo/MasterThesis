%%%%%%%%%%%%%%%%%%%%%%%%
% get mean of n shapes %
%%%%%%%%%%%%%%%%%%%%%%%%

clear

n = input('enter number of shapes: ')
p = input('enter number of vertices per shape: ')

axis([-1 1 -1 1])
plot(0,0,'gx');

for j = 1:n,
    in = []
    % get shape
    for k = 1:p
        in(:,k) = ginput(1)
        int = in
        int(:,k+1) = int(:,1)
        int = int'
        plot(int(:,1), int(:,2))
        hold on
        axis([-1 1 -1 1])
        plot(0,0,'gx');
        hold off
    end
    
    in = in'
    
    % get preshape
    in = in - repmat(mean(in), [p 1])
    scale = sqrt(sum(sum(in'.*in')'))
    preshape = in / scale

    complex_contour = preshape(:,1) + i*preshape(:,2)
    
    p2 = p
    midpoint_iterations = 0
    while 1
        midpoint_iterations = midpoint_iterations + 1
        new_complex_contour = []
        for k=1:p2-1,
            midpoint_x = (real(complex_contour(k)) + real(complex_contour(k+1))) / 2
            midpoint_y = (imag(complex_contour(k)) + imag(complex_contour(k+1))) / 2
            midpoint = midpoint_x + i*midpoint_y
            new_complex_contour(2*k-1) = complex_contour(k)
            new_complex_contour(2*k) = midpoint
        end
        midpoint_x = (real(complex_contour(p2)) + real(complex_contour(1))) / 2
        midpoint_y = (imag(complex_contour(p2)) + imag(complex_contour(1))) / 2
        midpoint = midpoint_x + i*midpoint_y
        new_complex_contour(2*p2-1) = complex_contour(p2)
        new_complex_contour(2*p2) = midpoint
        complex_contour = new_complex_contour
        p2 = length(complex_contour)
        if p2 > 100
           break 
        end
    end
    
    scale = sqrt(complex_contour*complex_contour')
    complex_contour = complex_contour / scale

    plot(complex_contour)
    hold on
    axis([-1 1 -1 1])
    plot(0,0,'gx');
    hold off

    
    % get complex coords of preshape
    %x = preshape
    %x = x(:,1) + i*x(:,2)

    %X(:,j) = x
    
    X(:,j) = complex_contour
    
end

p = p2

input('Hit enter to compute mean shape:')

x1 = X(:,1)
computing = 1
while computing == 1,
    for j = 1:n,
        
        x2 = X(:,j)
        
        % get complex conjugate inner product
        c = x2'*x1

        % get procrustes distance
        d = acos(abs(c))

        % get angle from x2 to fit onto x1
        theta = atan2(imag(c),real(c))

        % fit x2 to x1
        c_theta = cos(theta) + i * sin(theta)
        x2 = c_theta * x2
        
        Y(:,j) = x2
    end

    x1 = sum(Y.').'/n
    X = Y
    Z = [Y,x1]
    Z = Z.'
    Z(:,p+1) = Z(:,1)
    Z = Z.'
    
    % plot shapes
    plot(Z)
    axis([-1 1 -1 1])
    hold on
    plot(0,0,'gx');
    hold off

    computing = input('Type 1 to continue: ')
end

mean_shape = x1



%%%%%%%%%%%%%%%%%%%%%
% tangent space PCA %
%%%%%%%%%%%%%%%%%%%%%

% X is the matrix with fitted pre-shapes as columns
% compute Xt--the tangent space matrix of projected pre-shapes

for k = 1:p,
    vec_mean_shape(:,2*k-1) = real(mean_shape(k))
    vec_mean_shape(:,2*k) = imag(mean_shape(k))
    vec_X(2*k-1,:) = real(X(k,:))
    vec_X(2*k,:) = imag(X(k,:))
end

Xt = (eye(2*p) - vec_mean_shape*vec_mean_shape')*vec_X

Xt = Xt'
Xt_cov = cov(Xt)
[V D] = eig(Xt_cov)
eigenvectors = V
eigenvalues = diag(D)

Z = []

% compute the effects of the first three PCs
for j = 1:3
    Z(:,j) = vec_mean_shape' + 3*sqrt(eigenvalues(2*p-j+1))*V(:,2*p-j+1)
    for k = 1:p,
        ivec_Z(k,j) = Z(2*k-1,j) + i*Z(2*k,j)
    end
    scale = sqrt(ivec_Z(:,j)'*ivec_Z(:,j))
    ivec_Z(:,j) = ivec_Z(:,j) / scale
end

'first eigenvalue:'
eigenvalues(2*p)
input('hit enter to plot the effects of the first eigenvector:')

Z = []
Z(:,1) = ivec_Z(:,1)
Z(:,2) = mean_shape
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off

'second eigenvalue:'
eigenvalues(2*p-1)
input('hit enter to plot the effects of the second eigenvector:')

Z = []
Z(:,1) = ivec_Z(:,2)
Z(:,2) = mean_shape
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off

'third eigenvalue:'
eigenvalues(2*p-2)
input('hit enter to plot the effects of the third eigenvector:')

Z = []
Z(:,1) = ivec_Z(:,3)
Z(:,2) = mean_shape
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off

' first three eigenvalues:'
[eigenvalues(2*p) eigenvalues(2*p-1) eigenvalues(2*p-2)]
input('hit enter to plot the effects of the first three eigenvectors:')

Z = []
Z(:,1) = ivec_Z(:,1)
Z(:,2) = ivec_Z(:,2)
Z(:,3) = ivec_Z(:,3)
Z(:,4) = mean_shape
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off




%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% get partial shape and use MLE to fill in missing points %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

axis([-1 1 -1 1])
plot(0,0,'gx');
X = []
Y = []
Z = []

in = []

p2 = input('enter number of points in partial shape: ')

% get partial shape (first p2 points)
for k = 1:p2
    in(:,k) = ginput(1)
    int = in
    int(:,k+1) = int(:,1)
    int = int'
    plot(int(:,1), int(:,2))
    hold on
    axis([-1 1 -1 1])
    plot(0,0,'gx');
    hold off
end

in = in'

% get partial preshape
in = in - repmat(mean(in), [p2 1])
scale = sqrt(sum(sum(in'.*in')'))
preshape = in / scale

complex_contour = preshape(:,1) + i*preshape(:,2)

p3 = p2
for k=1:midpoint_iterations,
    new_complex_contour = []
    for k=1:p3-1,
        midpoint_x = (real(complex_contour(k)) + real(complex_contour(k+1))) / 2
        midpoint_y = (imag(complex_contour(k)) + imag(complex_contour(k+1))) / 2
        midpoint = midpoint_x + i*midpoint_y
        new_complex_contour(2*k-1) = complex_contour(k)
        new_complex_contour(2*k) = midpoint
    end
    %midpoint_x = (real(complex_contour(p3)) + real(complex_contour(1))) / 2
    %midpoint_y = (imag(complex_contour(p3)) + imag(complex_contour(1))) / 2
    %midpoint = midpoint_x + i*midpoint_y
    new_complex_contour(2*p3-1) = complex_contour(p3)
    %new_complex_contour(2*p3) = midpoint
    complex_contour = new_complex_contour
    p3 = length(complex_contour)
    %if p3 > 100
    %   break 
    %end
end

complex_contour = complex_contour - repmat(mean(complex_contour), [1 p3])
scale = sqrt(complex_contour*complex_contour')
complex_contour = complex_contour / scale

plot(complex_contour)
hold on
axis([-1 1 -1 1])
plot(0,0,'gx');
hold off

x = complex_contour.'
p2 = p3

% get complex coords of partial preshape
%x = preshape
%x = x(:,1) + i*x(:,2)

input('Hit enter to compute partial procrustes fit to mean:')

% get partial mean preshape
x1 = mean_shape(1:p2)
x1 = x1 - repmat(mean(x1), [p2 1])
scale = sqrt(x1'*x1)
x1 = x1 / scale

% now x = new partial preshape, x1 = mean partial preshape
% and we want to fit x to x1:        

x2 = x

% get complex conjugate inner product
c = x2'*x1

% get procrustes distance
d = acos(abs(c))

% get angle from x2 to fit onto x1
theta = atan2(imag(c),real(c))

% fit x2 to x1
c_theta = cos(theta) + i * sin(theta)
x2 = c_theta * x2

Z = [x2,x1]
Z = Z.'
Z(:,p2+1) = Z(:,1)
Z = Z.'

% plot fit of partial shape to partial mean shape
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off

input('Hit enter to compute MLE estimate of missing points:')

% first, rescale and translate partial shape x2 by offsets from full mean shape
x2 = scale * x2
offset = mean(mean_shape) - mean(mean_shape(1:p2))
x2 = x2 - offset

% plot shifted and scaled partial shape
x1 = mean_shape(1:p2)
Z = [x2,x1]
Z = Z.'
Z(:,p2+1) = Z(:,1)
Z = Z.'
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off

input('Hit enter to place missing points:')

% next, find the missing point
%c = -(1+i)*real(mean_shape(p)'*(x2'*mean_shape(1:p-1))) - (1-i)*real(mean_shape(p)*(mean_shape(1:p-1)'*x2))
%c = mean_shape(1:p-1)'*x2
%x2r = c / (2*mean_shape(p)*mean_shape(p)')
%x2(p) =  c / (2*mean_shape(p)')
for j=p2+1:p
    x2(j) = mean_shape(j)
end

% and plot it
x1 = mean_shape
Z = [x2,x1]
Z = Z.'
Z(:,p+1) = Z(:,1)
Z = Z.'
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off

input('Hit enter to fit full shape:')

% get complex conjugate inner product
c = x2'*x1

% get procrustes distance
d = acos(abs(c))

% get angle from x2 to fit onto x1
theta = atan2(imag(c),real(c))

% fit x2 to x1
c_theta = cos(theta) + i * sin(theta)
x2 = c_theta * x2

% and plot it
x1 = mean_shape
Z = [x2,x1]
Z = Z.'
Z(:,p+1) = Z(:,1)
Z = Z.'
plot(Z)
axis([-1 1 -1 1])
hold on
plot(0,0,'gx');
hold off



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% reconstructing full contours from partial views %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

clear

input('hit enter to input shape contour in COUNTER-CLOCKWISE order only!!! :')

axis([-1 1 -1 1])
plot(0,0,'gx');

% first get full contour from which to simulate partial views
p = 0
while 1
    [x y b] = ginput(1)
    if b ~= 1
        break
    end
    p = p+1    
    in(:,p) = [x y]
    int = in
    int(:,p+1) = int(:,1)
    int = int'
    plot(int(:,1), int(:,2))
    hold on
    axis([-1 1 -1 1])
    plot(0,0,'gx');
    hold off
end

input('got preshape, hit enter to rasterize:')

in = in'

% get preshape
in = in - repmat(mean(in), [p 1])
scale = sqrt(sum(sum(in'.*in')'))
preshape = in / scale

complex_contour = preshape(:,1) + i*preshape(:,2)

p2 = p
while 1
    new_complex_contour = []
    for k=1:p2-1,
        midpoint_x = (real(complex_contour(k)) + real(complex_contour(k+1))) / 2
        midpoint_y = (imag(complex_contour(k)) + imag(complex_contour(k+1))) / 2
        midpoint = midpoint_x + i*midpoint_y
        new_complex_contour(2*k-1) = complex_contour(k)
        new_complex_contour(2*k) = midpoint
    end
    midpoint_x = (real(complex_contour(p2)) + real(complex_contour(1))) / 2
    midpoint_y = (imag(complex_contour(p2)) + imag(complex_contour(1))) / 2
    midpoint = midpoint_x + i*midpoint_y
    new_complex_contour(2*p2-1) = complex_contour(p2)
    new_complex_contour(2*p2) = midpoint
    complex_contour = new_complex_contour
    p2 = length(complex_contour)
    if p2 > 100
       break 
    end
end

plot(complex_contour)
hold on
axis([-1 1 -1 1])
plot(0,0,'gx');
hold off


% next, get some partial views
n = 1
while 1
    partial_contour = []
    [x y] = ginput(1)
    [x2 y2] = ginput(1)
    % simulate readings with laser at (x,y) looking towards (x2,y2)

    % put contour in robot's coordinate frame
    complex_contour2 = complex_contour - repmat(x + i*y, [1 p2])
    robot_theta = atan2(y2-y, x2-x)
    complex_contour2 = complex_contour2 * (cos(robot_theta) - i*sin(robot_theta))

    full_contour_angles = atan2(imag(complex_contour2), real(complex_contour2))
    [sorted_angles angle_indices] = sort(full_contour_angles, 2, 'descend')

    kmin = 0
    kmax = 0
    max_angle = -1000
    min_angle = 1000
    for k=1:p2,
        t = sorted_angles(k)
        if abs(t) > pi/2
            continue
        end
        if t > max_angle
            max_angle = t
            kmax = k
        elseif t < min_angle
            min_angle = t
            kmin = k
        end
    end

    if kmin*kmax == 0
        'error: robot cant see object contour'
        n = n-1
        break
    end

    occlusion_mask = repmat(0, [1 p2])
    partial_contour(1) = complex_contour(angle_indices(kmax))
    pclen = 1
    next_index = angle_indices(kmax) + 1
    if (kmax < p2)
        for k=kmax+1:p2,
            if (sorted_angles(k) < -pi/2)
                break
            end
            if angle_indices(k) == next_index
                pclen = pclen + 1
                partial_contour(pclen) = complex_contour(angle_indices(k))
                next_index = next_index + 1
                while next_index <= p2 && occlusion_mask(next_index)
                    next_index = next_index + 1
                end
            else
                occlusion_mask(angle_indices(k)) = 1
                while next_index <= p2 && occlusion_mask(next_index)
                    next_index = next_index + 1
                end
            end
        end
    end

    plot(partial_contour)
    hold on
    axis([-1 1 -1 1])
    plot(0,0,'gx');
    hold off

    input('hit enter to continue')
        
    n = n+1
end




%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Building evidence grids of shape contours %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

res = input('enter the evidence grid resolution: ')
sigma = input('enter the laser stdev: ')

S(:,1) = real(mean_shape);
S(:,2) = imag(mean_shape);

% find bounds of egrid
x_min = S(1,1);
x_max = S(1,1);
y_min = S(1,2);
y_max = S(1,2);
for j = 1:length(S),
    if S(j,1) > x_max
        x_max = S(j,1);
    elseif S(j,1) < x_min
        x_min = S(j,1);
    end
    if S(j,2) > y_max
        y_max = S(j,2);
    elseif S(j,2) < y_min
        y_min = S(j,2);
    end    
end

x_min = x_min - 4*sigma;
x_max = x_max + 4*sigma;
y_min = y_min - 4*sigma;
y_max = y_max + 4*sigma;

x_size = floor((x_max - x_min) / res)
y_size = floor((y_max - y_min) / res)

% initialize evidence grid
E = repmat(-10, [y_size x_size]);

% compute log likelihood mask
x_mask_size = floor(4*sigma/res);
if mod(x_mask_size,2) == 0
    x_mask_size = x_mask_size+1;
end
y_mask_size = floor(4*sigma/res);
if mod(y_mask_size,2) == 0
    y_mask_size = y_mask_size+1;
end
x_mask_center = ceil(x_mask_size/2);
y_mask_center = ceil(y_mask_size/2);
for j = 1:y_mask_size,
    for k = 1:x_mask_size,
        dx = (k - x_mask_center)*res;
        dy = (j - y_mask_center)*res;
        if dx*dx+dy*dy > x_mask_size*x_mask_size*res*res/4
            mask(j,k) = -10
        else
            mask(j,k) = -(dx*dx+dy*dy)/(2*sigma*sigma);
        end
    end
end

for j = 1:length(S),
    x = floor((S(j,1) - x_min) / res);
    y = floor((S(j,2) - y_min) / res);
    if x <= 0 || x > x_size || y <= 0 || y > y_size
        'error: contour point out of bounds'
        continue
    end
    mask_x0 = max(1, x + x_mask_center - x_mask_size);
    mask_x1 = min(x_size, x - x_mask_center + x_mask_size);
    mask_y0 = max(1, y + y_mask_center - y_mask_size);
    mask_y1 = min(y_size, y - y_mask_center + y_mask_size);
    x_offset = x_mask_center - x;
    y_offset = y_mask_center - y;
    for y1 = mask_y0:mask_y1,
        for x1 = mask_x0:mask_x1,
            if E(y1,x1) < mask(y1+y_offset, x1+x_offset)
                E(y1,x1) = mask(y1+y_offset, x1+x_offset);
                pcolor(E)
                hold on;
                plot(x, y, 'gx');
                hold off;
                %mask(y1+y_offset, x1+x_offset)
                input(':')
            end
        end
    end
end

S(:,1) = (S(:,1) - x_min) / res;
S(:,2) = (S(:,2) - y_min) / res;

pcolor(E)



%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% scan-matching partial contours %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

hold on;

in = []
int = []

p = 0;
while 1
    [x y b] = ginput(1);
    if b ~= 1
        break
    end
    p = p+1;
    in(:,p) = [x y];
    int = in;
    int(:,p+1) = int(:,1);
    int = int';
    plot(int(:,1), int(:,2), 'yo');
end

hold off;

partial_contour = in';
pc_centroid = mean(partial_contour);

input('got partial contour, hit enter to scan match: ');

log_prob = 0;
for j = 1:length(partial_contour),
    x = floor(partial_contour(j,1));
    y = floor(partial_contour(j,2));
    log_prob = log_prob + E(y,x);
end

best_prob = log_prob;
best_dx = 0;
best_dy = 0;
best_dt = 0;

% compute shift bounds
x_min = partial_contour(1,1);
x_max = partial_contour(1,1);
y_min = partial_contour(1,2);
y_max = partial_contour(1,2);
for j = 1:length(partial_contour),
    if partial_contour(j,1) > x_max
        x_max = partial_contour(j,1);
    elseif partial_contour(j,1) < x_min
        x_min = partial_contour(j,1);
    end
    if partial_contour(j,2) > y_max
        y_max = partial_contour(j,2);
    elseif partial_contour(j,2) < y_min
        y_min = partial_contour(j,2);
    end
end

dx0 = -x_max;
dx1 = x_size - x_min;
dy0 = -y_max;
dy1 = y_size - y_min;

x_step_size = ceil(sigma/res);
y_step_size = ceil(sigma/res);

theta_step_size = pi/64
dt0 = -pi + theta_step_size
dt1 = pi

%input(':');

pc1 = []
pc2 = []
pc = []

for dt = dt0:theta_step_size:dt1,
    %input(':');
    pc1 = partial_contour - repmat(pc_centroid, [length(partial_contour) 1]);
    pc2(:,1) = cos(dt)*pc1(:,1) - sin(dt)*pc1(:,2);
    pc2(:,2) = sin(dt)*pc1(:,1) + cos(dt)*pc1(:,2);
    pc2 = pc2 + repmat(pc_centroid, [length(partial_contour) 1]);
    %input(':');
    for dx = dx0:x_step_size:dx1,
        %input('...:');
        pc(:,1) = pc2(:,1) + repmat(dx, [length(pc2) 1]);
        for dy = dy0:y_step_size:dy1,
            pc(:,2) = pc2(:,2) + repmat(dy, [length(pc2) 1]);
            log_prob = 0;
            for j = 1:length(pc),
                x = floor(pc(j,1));
                y = floor(pc(j,2));
                if x < 1 || x > x_size || y < 1 || y > y_size
                    log_prob = log_prob - 10;
                else
                    log_prob = log_prob + E(y,x);
                end
            end

            %log_prob
            %best_prob
            
            %pcolor(E);
            %shading interp;
            %hold on;
            %plot(pc(:,1), pc(:,2), 'yo');
            %hold off;
            %input(':');

            if log_prob > best_prob
                best_prob = log_prob
                best_dx = dx;
                best_dy = dy;
                best_dt = dt;
                pcolor(E);
                %shading interp;
                %hold on;
                %plot(pc(:,1), pc(:,2), 'yo');
                %hold off;
                %input(':');
            end
        end
    end
end

pcolor(E);
%shading interp;
hold on;
pc1 = partial_contour - repmat(pc_centroid, [length(partial_contour) 1]);
pc2(:,1) = cos(best_dt)*pc1(:,1) - sin(best_dt)*pc1(:,2);
pc2(:,2) = sin(best_dt)*pc1(:,1) + cos(best_dt)*pc1(:,2);
pc2 = pc2 + repmat(pc_centroid, [length(partial_contour) 1]);
pc(:,1) = pc2(:,1) + repmat(best_dx, [length(pc2) 1]);
pc(:,2) = pc2(:,2) + repmat(best_dy, [length(pc2) 1]);
plot(pc(:,1), pc(:,2), 'yo');
plot(S(:,1), S(:,2), 'gx');
hold off;
