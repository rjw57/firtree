from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'complex image compositing'

    def expected_hash(self):
        return '8eb8d29b4544657ad6a0cfe635afa2f4'

    def run_test(self, context, renderer, helper):
        fog_orig = helper.load_image('fog.png')
        lena_orig = helper.load_image('lena.png')

        trans1 = AffineTransform.Scaling(1, 1.5)
        fog = Image.CreateFromImageWithTransform(fog_orig, trans1)

        trans2 = AffineTransform.Translation(-256,-256)
        trans2.RotateByDegrees(-10)
        lena = Image.CreateFromImageWithTransform(lena_orig, trans2)

        kernel = Kernel.CreateFromSource('''
        kernel vec4 testKernel(sampler a, sampler b) 
        {
            vec4 aCol = sample(a, samplerCoord(a));
            vec4 bCol = sample(b, samplerCoord(b));
            return aCol + bCol * (1.0 - aCol.a);
        }
        ''')
        outimage = Image.CreateFromKernel(kernel);

        kernel.SetValueForKey(fog, 'a')
        kernel.SetValueForKey(lena, 'b')

        helper.write_test_output(outimage)

# vim:sw=4:ts=4:et
