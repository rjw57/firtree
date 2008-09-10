from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'separable gaussian blur with cropping'

    def expected_hash(self):
        return '0d83587dd8dcf1863055e5fa956ee932'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        sigma = 10.0

        smallLena = Image.CreateFromImageWithTransform(
            lena,
            AffineTransform.Scaling(0.5, 0.5))

        lenaExtent = smallLena.GetExtent()

        blurKernel = Kernel.CreateFromSource('''
            kernel vec4 blurKernel(sampler src, vec2 direction, float sigma)
            {
                vec4 outputColour = vec4(0,0,0,0);
                for(float delta = -2.0; delta <= 2.0; delta += 1.0/sigma)
                {
                    float weight = exp(-0.5*delta*delta);
                    outputColour += weight * sample(src, 
                        samplerTransform(src, destCoord() + sigma * delta * direction));
                }
                
                float oneOverRootTwoPi = 1.0 / 2.5066;
                outputColour *= oneOverRootTwoPi / sigma;

                return outputColour;
            }
        ''')

        xBlur = Image.CreateFromKernel(blurKernel,
            ExtentProvider.CreateStandardExtentProvider(None, -2.0*sigma, -2.0*sigma))
        blurKernel.SetValueForKey(smallLena, 'src')
        blurKernel.SetValueForKey(1.0, 0.0, 'direction')
        blurKernel.SetValueForKey(sigma, 'sigma')

        imageAccum = ImageAccumulator.Create(lenaExtent, context)
        imageAccum.RenderImage(xBlur)

        yBlur = Image.CreateFromKernel(blurKernel)
        blurKernel.SetValueForKey(imageAccum.GetImage(), 'src')
        blurKernel.SetValueForKey(0.0, 1.0, 'direction')
        blurKernel.SetValueForKey(sigma, 'sigma')

        helper.write_test_output(yBlur)

# vim:sw=4:ts=4:et
