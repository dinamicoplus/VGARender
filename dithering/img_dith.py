# Marcos Brito 2020
# Trabajo Fin Grado ETSIT UMA

from PIL import Image, ImageDraw
import argparse, json
import numpy as np

def resize_wo_deform(img,size):
    """Resizes Image to fit into the size specified.
    
    Arguments:
        img -- Image to resize
        size -- A tuple containing the desired size in pixels
    Return:
        Processed image
    """
    new_img = Image.new('RGB',size)
    ratio = img.size[0]/img.size[1]

    img = img.resize((int(size[1]*ratio),size[1]),Image.ANTIALIAS) if ratio < size[0]/size[1] else img.resize((size[0],int(size[0]/ratio)),Image.ANTIALIAS)
    left_corner = (int(size[0]/2-img.size[0]/2),0) if ratio < size[0]/size[1] else (0,int(size[1]/2-img.size[1]/2))
    
    new_img.paste(img,left_corner)
    return new_img
    
def posterize(img, bits):
    """Posterizes Image to the desired bit depth.
    
    Arguments:
        img -- Image to posterize
        bits -- Bit depth
    Return:
        Processed image
    """
    return Image.eval(img,lambda a: round(a/2**(9-bits))*2**(9-bits))
    
def fs_dithering(img,bits):
    """Applies Floyd–Steinberg dithering algorithm to Image at the desired bit depth.
    
    Arguments:
        img -- Image to process
        bits -- Bit depth
    Return:
        Processed image
    """
    new_img = Image.new('RGB',img.size)
    new_img.paste(img)
    pix = new_img.load()
    for y in range(0,img.size[1]):
        for x in range(0,img.size[0]):
            oldpixel = pix[x,y]
            newpixel = tuple(round(el /2**(9-bits))*2**(9-bits) for el in oldpixel)
            pix[x,y] = newpixel
            e = tuple(el1 - el2 for el1, el2 in zip(oldpixel,newpixel))
            try:
                pix[x+1,y  ]=tuple(round(el1 + el2*7/16) for el1, el2 in zip(pix[x+1,y  ],e))
                pix[x-1,y+1]=tuple(round(el1 + el2*3/16) for el1, el2 in zip(pix[x-1,y+1],e))
                pix[x  ,y+1]=tuple(round(el1 + el2*5/16) for el1, el2 in zip(pix[x  ,y+1],e))
                pix[x+1,y+1]=tuple(round(el1 + el2*1/16) for el1, el2 in zip(pix[x+1,y+1],e))
            except:
                ()   
    
    return new_img

def img_into_byte2Dlist(img):
    """Creates a 2D list (dimensions of the image) from Image. 
    Each pixel is expected to be a 1-bit RGB:
    R - (0,255)  G - (0,255)  B - (0,255)
    With that in mind, we assign 1bit per channel and store it
    in a byte with a look up table.
    
    Arguments:
        img -- Image to process
    Return:
        Processed list
    """
    bp = {
        b'\x00\x00\x00': 0,
        b'\xff\x00\x00': 1,
        b'\x00\xff\x00': 2,
        b'\xff\xff\x00': 3,
        b'\x00\x00\xff': 4,
        b'\xff\x00\xff': 5,
        b'\x00\xff\xff': 6,
        b'\xff\xff\xff': 7,
        }
    b = [bp[img.tobytes()[e:e+3]] for e in range(0,len(img.tobytes()),3)]

    output_b = []
    [output_b.append(list(e)) for e in np.reshape(b,(img.height,img.width))]
    
    return output_b
       
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process dithering to picture')
    parser.add_argument('input', help='input picture')
    parser.add_argument('-o', default='output.bmp', help='output picture')
    parser.add_argument('-width', default=150, help='width of the pic')
    parser.add_argument('-height', default=96, help='heigth of the pic')
    parser.add_argument('-bits', default=1, help='bit depth of the pic')
    parser.add_argument('-header', default='pixels.h', help='output header')

    # Process the arguments
    args = parser.parse_args()
    w = int(args.width)
    bit_depth = args.bits
    h = int(args.height)
    in_img = args.input
    header_file = args.header
    out_img = args.o
    
    # Process the image. Resize, Floyd–Steinberg dithering or Posterize
    img = Image.open(in_img)
    img = resize_wo_deform(img,(w,h))
    #img = posterize(img, bit_depth)
    img = fs_dithering(img,bit_depth)

    # Convert image into C byte array
    b_list = img_into_byte2Dlist(img)
    out = str(b_list).replace('[','{').replace(']','}').replace('},','},\n')
    out = "// To change. This approach is crap\n\nunsigned char pix[PIX_H][PIX_W] = " + out + ";"

    # Save the array in a header file
    f = open(header_file,'w')
    f.write(out)
    f.close()
    
    # Save the processed image
    img.save(out_img)