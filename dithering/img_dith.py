# Marcos Brito 2020
# Trabajo Fin Grado ETSIT UMA

from PIL import Image
import argparse, json
import numpy as np

def find_color(oldpixel):
	return tuple(round(el /2**(9-args.b))*2**(9-args.b) for el in oldpixel)
    
def mod_pix(x,y):
    oldpixel = pix[x,y]
    newpixel = find_color(oldpixel)
    pix[x,y] = newpixel
    e = tuple(el1 - el2 for el1, el2 in zip(oldpixel,newpixel))
    try:
        pix[x+1,y  ]=tuple(round(el1 + el2*7/16) for el1, el2 in zip(pix[x+1,y  ],e))
        pix[x-1,y+1]=tuple(round(el1 + el2*3/16) for el1, el2 in zip(pix[x-1,y+1],e))
        pix[x  ,y+1]=tuple(round(el1 + el2*5/16) for el1, el2 in zip(pix[x  ,y+1],e))
        pix[x+1,y+1]=tuple(round(el1 + el2*1/16) for el1, el2 in zip(pix[x+1,y+1],e))
    except:
        ()   
        
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process dithering to picture')
    parser.add_argument('input', help='input picture')
    parser.add_argument('-o', default='output.bmp', help='output picture')
    parser.add_argument('-w', default=300, help='width of the pic')
    parser.add_argument('-b', default=1, help='bit depth of the pic')
    parser.add_argument('-l', default='output.log', help='output picture')

    args = parser.parse_args()
    bw = int(args.w)
    img = Image.open(args.input)
    ratio = (bw/float(img.size[0]))
    h = int((float(img.size[1])*float(ratio)))
    img = img.resize((bw,h), Image.ANTIALIAS)
    
    ideal_h = int(bw*3/4)
    blank = ideal_h-h
    b1 = round(blank/2)
    b2 = blank-b1

    pix = img.load()
    [mod_pix(k,j) for j in range(0,img.size[1]) for k in range(0,img.size[0])]
    bp = {
        b'\x00\x00\x00': 0,#b'\x00', 
        b'\xff\x00\x00': 1,#b'\x01', 
        b'\x00\xff\x00': 2,#b'\x02', 
        b'\xff\xff\x00': 3,#b'\x03',
        b'\x00\x00\xff': 4,#b'\x04',        
        b'\xff\x00\xff': 5,#b'\x05', 
        b'\x00\xff\xff': 6,#b'\x06', 
        b'\xff\xff\xff': 7,#b'\x07'
        }
    b = [bp[img.tobytes()[e:e+3]] for e in range(0,len(img.tobytes()),3)]
    b_list = [[0]*bw for c in range(0,b1)]
    [b_list.append(list(e)) for e in np.reshape(b,(h,bw))]
    [b_list.append([0]*bw) for c in range(0,b2)]
    out = str(b_list).replace('[','{').replace(']','}').replace('},','},\n')
    out = "// To change. This approach is crap\n\nunsigned char pix[PIX_H][PIX_W] = " + out + ";"
    #out = str([list(e) for e in np.reshape(b,(h,bw))]).replace('[','{').replace(']','}')
    #print(len(b))
    f = open(args.l,'w')
    #json.dump(b,f)
    f.write(out)
    f.close()
    
    img.save(args.o)