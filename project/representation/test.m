%% test script
close all;
clear all;
% create the nodes

verden = world(10);


t1 = t_junction(2, 32, 1, 10, 10);
t2 = left_bend(3, 180, 2, 10, 10);
t3 = t_junction(4, 90, 3, 10, 10);


verden = verden.addNode(t1, [-10;2]);
verden = verden.addNode(t2, [20; 0]);
verden = verden.addNode(t3, [20; 20]);

% t1.draw_edges();
% t2.draw_edges();
% t3.draw_edges();


verden.draw();