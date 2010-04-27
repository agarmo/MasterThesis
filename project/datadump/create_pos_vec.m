

pos_vec = [];

for i=1:176
    for j = 1:144
        pos_vec(i*j, :) = pos(i,j,:);
        
    end
end

temp = pos_vec;
temp(~any(pos_vec,2),:)=[]; %% remove zeros

x0 = 0;
y0 = 0;
alpha = 1;
beta = 1;
s = 1;

param = [x0, y0, alpha, beta, s];

[x0n, an, rn, d, sigmah, conv, Vx0n, Van, urn, GNlog, a, R0, R] = ...
    lscylinder(temp, [0, 0,0]', [0,0,1]', 0.25,1,1);


% [f] = fgcylinder(param, pos_vec);



%% start constructing the cylinder from the given parameters.


x = -1:0.01:1;
y = -1:0.01:1;
z = -1:0.01:1;

xa = (x./an(1)).^2 -rn^2;
yn = (y./an(2)).^2 -rn^2;
zn = (z./an(3)).^2 -rn^2;

plot3(xa,yn,zn)

