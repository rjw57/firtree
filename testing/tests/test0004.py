from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'simple image transform'

    def expected_hash(self):
        return '2ea0f1e539fa1c272667f721f08bf8cb'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        lenaTransform = AffineTransform.Scaling(0.5)
        lenaTransImage = Image.CreateFromImageWithTransform(lena, lenaTransform)
        helper.write_test_output(lenaTransImage)

# vim:sw=4:ts=4:et
