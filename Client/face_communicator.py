import serial
from enum import Enum, unique

class face_communicator:
	predefined_faces_limit=1000
	port="/dev/ttyUSB0"
	
	@unique
	class expression(Enum):
		HAPPY		=1
		SAD			=2
		WINK_LEFT	=3
		WINK_RIGHT	=4
		SLEEP		=5
		SURPRISED	=6
	
	
	
	def __init__(self, port="/dev/ttyUSB0"):
		self.port=port
		
	def send_face(self, identifier):
		string_to_send="face "
		if not identifier:
			raise TypeError("Identifier cannot be empty!")
			
			
			
		if isinstance(identifier, str):
			string_to_send=string_to_send+identifier
		
		elif isinstance(identifier, self.expression):
			string_to_send=string_to_send+"%d" %identifier.value
			
		else:
			if identifier>65535:
				raise TypeError("Identifier is too huge: should be less than 65536!")
				
			if identifier>self.predefined_faces_limit:
				string_to_send=string_to_send+"0x%X" %identifier
			else:
				string_to_send=string_to_send+"%d" %identifier
		
		self.send_string_to_serial(string_to_send)
			

	def send_string_to_serial(self,string):
		ser=serial.Serial(self.port,9600)
		ser.write((string+"\r").encode())
		x = ser.readline()
		print(x)    
		#print (string)


fc=face_communicator()


fc.send_face(100);
fc.send_face(10500);
fc.send_face("0.0");
fc.send_face(face_communicator.expression.HAPPY);
fc.send_face(face_communicator.expression.SLEEP);
#face_communicator("pooooort").send_face(1222424242);

