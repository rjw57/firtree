from TestModule import *

class Test (TestModule):
    def name(self):
        return 'simple image loader'

    def run_test(self, context, renderer, helper):
        lena = helper.load_image('lena.png')
        helper.write_test_output(lena)

# vim:sw=4:ts=4:et
