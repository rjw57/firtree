from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'simple image crop'

    def expected_hash(self):
        return '4b3204f92e05bcc8a3e34fabc331ccea'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        lenaTransImage = Image.CreateFromImageCroppedTo(lena, Rect2D(100,100,256,256))
        helper.write_test_output(lenaTransImage)

# vim:sw=4:ts=4:et
