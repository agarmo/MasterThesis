%% test script
close all;
clear all;
% create the nodes

verden = world(0.25);


t1 = t_junction(2, 90, 1, 10, 10);
t2 = left_bend(3, 0, 2, 10, 10);
t3 = t_junction(4, 0, 3, 10, 10);


verden = verden.addNode(t1, [-10;0]);
verden = verden.addNode(t2, [-10; 10]);
verden = verden.addNode(t3, [20; 10]);

% t1.draw_edges();
% t2.draw_edges();
% t3.draw_edges();


verden.draw();