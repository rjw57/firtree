from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'simple image colour inverter'

    def expected_hash(self):
        return '78f835913ab6328ceb5b67ee5fce7878'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        kernel = Kernel.CreateFromSource('''
        kernel vec4 testKernel(sampler src) 
        {
            vec4 srcCol = sample(src, samplerCoord(src));
            vec4 invSrcCol = vec4(1,1,1,1) - srcCol;
            return vec4(invSrcCol.rgb, srcCol.a);
        }
        ''')
        outimage = Image.CreateFromKernel(kernel);

        kernel.SetValueForKey(lena, 'src')

        helper.write_test_output(outimage)

# vim:sw=4:ts=4:et
