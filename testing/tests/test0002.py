from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'simple image compositing'

    def expected_hash(self):
        return 'a48707cdbf3edd8d3a9da37ef13913cd'

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

        kernel.SetValueForKey(fog, 'a')
        kernel.SetValueForKey(lena, 'b')

        helper.write_test_output(outimage)

# vim:sw=4:ts=4:et
