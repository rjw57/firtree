from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'separable gaussian blur with post processing'

    def expected_hash(self):
        return '48ed27e597400e77762145fbe355a04e'

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

        lenaExtent = smallLena.GetExtent()
        lenaExtent = Rect2D.Inset(lenaExtent, -2.0*sigma, -2.0*sigma)

        lenaSize = lenaExtent.Size
        accumImage = AccumulationImage.Create(int(lenaSize.Width), 
            int(lenaSize.Height), context)

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
        blurKernel.SetValueForKey(smallLena, 'src')
        blurKernel.SetValueForKey(1.0, 0.0, 'direction')
        blurKernel.SetValueForKey(sigma, 'sigma')

        accumRenderer = accumImage.GetRenderer()
        accumRenderer.RenderWithOrigin(xBlur, Point2D(2.0*sigma, 2.0*sigma))

        yBlur = Image.CreateFromKernel(blurKernel)
        blurKernel.SetValueForKey(accumImage, 'src')
        blurKernel.SetValueForKey(0.0, 1.0, 'direction')
        blurKernel.SetValueForKey(sigma, 'sigma')

        invKernel.SetValueForKey(yBlur, 'src')
        smallInvLena = Image.CreateFromKernel(invKernel)

        helper.write_test_output(smallInvLena)

# vim:sw=4:ts=4:et
