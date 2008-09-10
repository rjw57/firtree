from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'image compositing + transformation'

    def expected_hash(self):
        return 'e22329eff4f9c8db3f1dd1c774fc3d81'

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
        outTrans.ScaleBy(0.5)

        kernel.SetValueForKey(fog, 'a')
        kernel.SetValueForKey(lena, 'b')

        outtransimage = Image.CreateFromImageWithTransform(outimage, outTrans)
        helper.write_test_output(outtransimage)

# vim:sw=4:ts=4:et
