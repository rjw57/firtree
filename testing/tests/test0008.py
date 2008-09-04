from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'complex cropping'

    def expected_hash(self):
        return '75154096942bd9aff5734614bd055b14'

    def run_test(self, context, renderer, helper):
        fog = helper.load_image('fog.png')
        lena = helper.load_image('lena.png')
        kernel = Kernel.CreateFromSource('''
        kernel vec4 testKernel(sampler a, sampler b) 
        {
            vec4 aCol = sample(a, samplerCoord(a));
            vec4 bCol = sample(b, samplerCoord(b));
            return aCol + bCol * (1.0 - aCol.a);
        }
        ''')
        outimage = Image.CreateFromKernel(kernel);

        outTrans = AffineTransform.RotationByDegrees(30)

        kernel.SetValueForKey(fog, 'a')
        kernel.SetValueForKey(lena, 'b')

        outtransimage = Image.CreateFromImageWithTransform(outimage, outTrans)
        outtransimage2 = Image.CreateFromImageCroppedTo(outtransimage, Rect2D(100,100,256,256))
        helper.write_test_output(outtransimage2)

# vim:sw=4:ts=4:et
