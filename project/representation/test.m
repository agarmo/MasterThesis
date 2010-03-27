%% test script
clear all;
clear;
close all;
% create the nodes

t1 = t_junction(0, 0, [], 10, 10);
t2 = left_bend(1, 180, 0, 10, 10);
t3 = t_junction(2, 90, 1, 10, 10);


t1 = t1.draw_at_position(0,0);
t2 = t2.draw_at_position(20, 0);
t3 = t3.draw_at_position(20, 20);

t1.draw_edges();
t2.draw_edges();
t3.draw_edges();


