class TestModule:
    '''TestModule is the base class from which all tests should be derrived.'''

    def name(self):
        '''return a human readable test name'''
        return 'Untitled test'

    def init_test(self):
        '''perform one-time initialisation of test (before Firtree context is active).'''
        pass

    def finalise_test(self):
        '''perform one-time finalisation of test (after Firtree context is active).'''
        pass

    def expected_hash(self):
        '''return the expected MD5 hash of the output file.'''
        return None

    def run_test(self, context, renderer, helper):
        '''run this test, context is a Firtree OpenGLContext one can used
           for rendering. This method is called between Begin() and End().'''
        pass

# vim:sw=4:ts=4:et
