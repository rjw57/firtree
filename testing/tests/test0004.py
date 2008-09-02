from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'simple image transform'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        lenaTransform = AffineTransform.Scaling(0.5)
        lenaTransImage = Image.CreateFromImageWithTransform(lena, lenaTransform)
        helper.write_test_output(lenaTransImage)

# vim:sw=4:ts=4:et
