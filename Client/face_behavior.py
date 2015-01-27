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
