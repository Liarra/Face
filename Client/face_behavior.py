import expression

class face_behavior:
	table={}
	###{face, {[0.5, face],[0.5, otherface]}}
	def add_state(self, face, transitions):
		#check if transition probabilites are ok
		self.table[face]=transitions
		
	def print_table(self):
		for face in self.table.keys():
			for otherface in self.table[face].keys():
				probability=self.table[face][otherface]
				print ("%s--%.2f->%s   " % (face, probability, otherface))
			
			print ("\n")

fb=face_behavior()
fb.add_state(1,{1:0.5, 2:0.5})
fb.add_state(2,{1:0.5, 2:0.5})
fb.add_state(3,{1:0.5, 2:0.25, 3:0.25})

fb.print_table()

b=face_behavior()
b.add_state(expression.HAPPY, {expression.HAPPY:0.8, expression.WINK_LEFT:0.1, expression.WINK_RIGHT:0.1})
b.add_state(expression.WINK_LEFT, {expression.HAPPY:1})
b.add_state(expression.WINK_RIGHT, {expression.HAPPY:1})

wink_behavior=b
