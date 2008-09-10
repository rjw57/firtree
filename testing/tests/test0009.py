from TestModule import *
from Firtree import *

class Test (TestModule):
    def name(self):
        return 'accumulation image'

    def expected_hash(self):
        return '8093918d5dcb6a4885189b99c4853c08'

    def run_test(self, context, renderer, helper):
        fog = helper.load_image('fog.png')
        lena = helper.load_image('lena.png')

        outputImageAccum = ImageAccumulator.Create(Rect2D(0,0,320,240), context)

        smallLena = Image.CreateFromImageWithTransform(
            lena,
            AffineTransform.Scaling(0.25, 0.25))

        outputImageAccum.RenderImage(smallLena)
        outputImageAccum.RenderImage(Image.CreateFromImageWithTransform(smallLena,
            AffineTransform.Translation(320-128,240-128)))

        kernel = Kernel.CreateFromSource('''
        kernel vec4 invkernel(sampler src)
        {
            vec4 a = sample(src, samplerCoord(src));
            return vec4(1.0 - a.rgb, a.a);
        }
        ''')
        invImage = Image.CreateFromKernel(kernel);
        kernel.SetValueForKey(lena, 'src')

        smallInv = Image.CreateFromImageWithTransform(
            invImage, AffineTransform.Scaling(0.25, 0.25))

        outputImageAccum.RenderImage(Image.CreateFromImageWithTransform(smallInv,
            AffineTransform.Translation(160-64,120-64)))

        kernel = Kernel.CreateFromSource('''
        kernel vec4 radialkernel(float radius, __color blendCol)
        {
            float r = 1.0 - (length(destCoord()) / radius);
            r = max(0.0, r);
            return r * blendCol;
        }
        ''')
        radius = 50.0
        kernel.SetValueForKey(radius, 'radius')
        gradImage = Image.CreateFromKernel(kernel)
        gradImageCrop = Image.CreateFromImageCroppedTo(gradImage,
            Rect2D(-radius,-radius,2*radius,2*radius))

        kernel.SetValueForKey(0.0,1.0,0.0,1.0, 'blendCol')
        outputImageAccum.RenderImage(Image.CreateFromImageWithTransform(gradImageCrop,
            AffineTransform.Translation(160,120)))

        kernel.SetValueForKey(0.0,0.0,1.0,1.0, 'blendCol')
        outputImageAccum.RenderImage(Image.CreateFromImageWithTransform(gradImageCrop,
            AffineTransform.Translation(320,120)))

        kernel.SetValueForKey(1.0,0.0,0.0,1.0, 'blendCol')
        outputImageAccum.RenderImage(Image.CreateFromImageWithTransform(gradImageCrop,
            AffineTransform.Translation(0,120)))

        helper.write_test_output(outputImageAccum.GetImage())

# vim:sw=4:ts=4:et
