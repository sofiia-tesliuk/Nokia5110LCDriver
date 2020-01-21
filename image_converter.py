from PIL import Image



input_filename = ""
output_filename = ""


img = Image.open(input_filename).convert('LA')

LCD_WIDTH = 84
LCD_HEIGHT = 48

width, height = img.size

new_height = LCD_HEIGHT
new_width = int(height * new_height / width)

img = img.resize((new_width, new_height))

old_size = img.size
new_size = (LCD_WIDTH, LCD_HEIGHT)
new_im = Image.new(mode="L", size=new_size, color=(0))
new_im.paste(img, ((new_size[0] - old_size[0]) // 2,
                              (new_size[1] - old_size[1]) // 2))
new_im.show()

lst = []
cnl = [[] for i in range(LCD_HEIGHT)]
for seg_i in range(6):
    for j in range(LCD_WIDTH):
        my_byte = ''
        for i in range(seg_i * 8, (seg_i + 1) * 8):
            cnl[i].append('X' if new_im.getpixel((j, i)) > 127 else ' ')
            my_byte += str(int(new_im.getpixel((j, i)) > 127))
        my_byte = my_byte[::-1]
        lst.append(int(my_byte, 2))

with open(output_filename, 'a') as file:
    file.write('\n'.join([str(x) for x in lst]))

for row in cnl:
    print(''.join(row))
