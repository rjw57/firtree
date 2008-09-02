from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'simple image crop'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        lenaTransImage = Image.CreateFromImageCroppedTo(lena, Rect2D(100,100,256,256))
        helper.write_test_output(lenaTransImage)

# vim:sw=4:ts=4:et
