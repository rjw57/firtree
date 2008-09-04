from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'separable gaussian blur with prior processing'

    def expected_hash(self):
        return '09edc9a416f95ff746c9b214ee748496'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        sigma = 10.0

        smallLena = Image.CreateFromImageWithTransform(
            lena,
            AffineTransform.Scaling(0.5, 0.5))

        invKernel = Kernel.CreateFromSource('''
            kernel vec4 invKernel(sampler src)
            {
               vec4 srcCol = unpremultiply(sample(src, samplerCoord(src)));
               return premultiply(vec4(1.0 - srcCol.rgb, srcCol.a));
            }
        ''')
        smallInvLena = Image.CreateFromKernel(invKernel)
        invKernel.SetValueForKey(smallLena, 'src')

        lenaExtent = smallInvLena.GetExtent()
        lenaExtent = Rect2D.Inset(lenaExtent, -2.0*sigma, -2.0*sigma)

        blurKernel = Kernel.CreateFromSource('''
            kernel vec4 blurKernel(sampler src, vec2 direction, float sigma)
            {
                vec4 outputColour = vec4(0,0,0,0);
                float weightSum = 0.0;
                for(float delta = -2.0; delta <= 2.0; delta += 1.0/sigma)
                {
                    float weight = exp(-delta*delta);
                    weightSum += weight;
                    outputColour += weight * sample(src, 
                        samplerTransform(src, destCoord() + sigma * delta * direction));
                }
                
                outputColour /= weightSum;

                return outputColour;
            }
        ''')

        xBlur = Image.CreateFromKernel(blurKernel,
            ExtentProvider.CreateStandardExtentProvider(None, -2.0*sigma, -2.0*sigma))
        blurKernel.SetValueForKey(smallInvLena, 'src')
        blurKernel.SetValueForKey(1.0, 0.0, 'direction')
        blurKernel.SetValueForKey(sigma, 'sigma')

        imageAccum = ImageAccumulator.Create(lenaExtent)
        imageAccum.RenderImage(xBlur)

        yBlur = Image.CreateFromKernel(blurKernel)
        blurKernel.SetValueForKey(imageAccum.GetImage(), 'src')
        blurKernel.SetValueForKey(0.0, 1.0, 'direction')
        blurKernel.SetValueForKey(sigma, 'sigma')

        helper.write_test_output(yBlur)

# vim:sw=4:ts=4:et
