from TestModule import *
from Firtree import *

from lib0015.PageCurlTransition import *

class Test (TestModule):
    def name(self):
        return 'page curl example'

    def expected_hash(self):
        return 'd7ce310d5024bab34eafcaf46578f478'

    def run_test(self, context, renderer, helper):
        trans = PageCurlTransition()

        statues = helper.load_image('statues.jpg')
        painting = helper.load_image('painting.jpg')

        trans.set_front_image(painting)
        trans.set_back_image(statues)
        trans.set_radius(100.0)
        trans.set_progress(0.5)

        helper.write_test_output(trans.get_output())

# vim:sw=4:ts=4:et
