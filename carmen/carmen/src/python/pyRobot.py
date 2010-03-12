# file: runme.py
# This file illustrates the cross language polymorphism using directors.
import pyCarmen
from threading import Thread
from time import sleep
from sys import exit
from scipy import *

#You need to override the callback function in order
#to get the functionality of this class
class pyMessageHandler(pyCarmen.MessageHandler, Thread):
	def __init__(self):
		pyCarmen.carmen_ipc_initialize(1, ["python"])
		pyCarmen.MessageHandler.__init__(self)

		Thread.__init__(self)
		self.laser = None

	def run(self):
		self.connect()

	def run_cb(self, my_type, msg_functor):
		msg = eval("self."+msg_functor)
		self.callback(my_type, msg)

	def callback(self, the_type, msg):
		print the_type
		#print dir(msg)
		pass

	def connect(self):
		while(1):
			pyCarmen.carmen_ipc_sleep(0.05)

		#pyCarmen.carmen_ipc_dispatch()

	def stop(self):
		pyCarmen.carmen_ipc_disconnect()
		
	def __del__(self):
		print "PyCallback.__del__()"
		pyCarmen.MessageHandler.__del__(self)
		

class Robot(pyMessageHandler):
	def __init__(self):
		self.name = ""
		pyMessageHandler.__init__(self)
		
	def callback(self, the_type, msg):
		print the_type
		
		#if(the_type == "global_pose"):
		#     print "global_pose", msg.globalpos.x, msg.globalpos.y
		pass

	def set_pose(self, x, y, theta):
		pt = pyCarmen.carmen_point_t()
		pt.x = x
		pt.y = y
		pt.theta = theta
		pyCarmen.carmen_simulator_set_truepose(pt)
		
	def set_goal(self, x, y):
		pyCarmen.carmen_navigator_set_goal(x, y)
		
	def set_goal_name(self, name):
		pyCarmen.carmen_navigator_set_goal_place(name)
		
	def command_velocity(self, tv, rv):
		pyCarmen.carmen_robot_velocity_command(tv, rv)
		
	def command_vector(self, distance, theta):
		pyCarmen.carmen_robot_move_along_vector(distance, theta)
		
	def command_go(self):
		pyCarmen.carmen_navigator_go()
		
	def command_stop(self):
		pyCarmen.carmen_navigator_stop()
		
	#test this 
	def command_arm(self, joint_angles):
		pyCarmen.carmen_arm_command(len(joint_angles), joint_angles)
				



