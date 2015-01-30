#!/usr/bin/env python2

from Tkinter import *
from ttk import Frame

leds = [
    [(2,1),(4,1),(12,1),(14,1)],
    [(0,0),(16,0)],
    [(6,0),(10,0)],
    [(6,2),(10,2)],
    [(0,2),(16,2)],
    [(2,5),(4,5)],
    [(12,5),(14,5)],
    [(1,7),(11,7)],
    [(3,7),(13,7)],
    [(5,7),(15,7)],
    [(2,12),(14,12)],
    [(4,13),(12,13)],
    [(6,13),(8,13),(10,13)],
    [(2,14),(14,14)],
    [(5,15),(11,15)],
    [(7,16),(9,16)],
]

class FaceEditor(Frame):
    def __init__(self, parent):
        Frame.__init__(self, parent)
        self.parent = parent

        self.parent.title("Face Editor")
        self.pack(fill=BOTH, expand=1)
        self.face = Frame(self)

        self.state = [IntVar() for _ in range(16)]
        
        button_config = [('#99cc99', '#00ff00')]*5 + \
                        [('#cccc99', '#ffff00')]*5 + \
                        [('#cc9999', '#ff0000')]*6
        button_config_base = dict(indicatoron=False, width=15, height=15,
            image=BitmapImage(data="#define im_width 1\n#define im_height 1\nstatic char im_bits[] = { 0x00 };"))
        for key, (background, selectcolor) in enumerate(button_config):
            button_config[key] = dict(background=background, selectcolor=selectcolor)
            button_config[key].update(button_config_base)
        
        self.buttons = []
        for i in range(16):
            for x, y in leds[i]:
                b = Checkbutton(self.face, variable=self.state[i], command=self.recompute, onvalue=1, offvalue=0)
                b.configure(**button_config[i])
                self.buttons.append(b)
                b.place(x=x*10, y=y*10)
        
        self.face.pack(fill=BOTH, expand=1)

        self.binvalue = Text(self, width=16, height=1, wrap=NONE)
        self.binvalue.configure(inactiveselectbackground=self.binvalue.cget("bg"), relief=FLAT)
        self.binvalue.pack(side=RIGHT)
        self.hexvalue = Text(self, width=4, height=1, wrap=NONE)
        self.hexvalue.configure(inactiveselectbackground=self.hexvalue.cget("bg"), relief=FLAT)
        self.hexvalue.pack(side=LEFT)
        
        self.recompute()

    def recompute(self):
        value = 0
        for i, var in enumerate(self.state):
            value += 2**i if var.get()==1 else 0
        binvalue = bin(value)[2:].zfill(16)
        hexvalue = hex(value)[2:].zfill(4)
        self.value = value
        self.binvalue.delete(1.0, END)
        self.binvalue.insert(END, binvalue)
        self.hexvalue.delete(1.0, END)
        self.hexvalue.insert(END, hexvalue)
        print binvalue, hexvalue

def run():
    root = Tk()
    root.geometry("180x210")
    app = FaceEditor(root)
    root.mainloop()

if __name__ == '__main__':
    run()
