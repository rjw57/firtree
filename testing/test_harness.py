import sys
import os
import glob
import traceback

# Find the script path.
scriptDir = os.path.abspath(os.path.dirname(sys.argv[0]))

# Walk down one subdir
rootDir = os.path.dirname(scriptDir)

# Add path to the python bindings
importDir = os.path.join(rootDir, 'bindings', 'python')

# Append this path to our search path & import the firtree bindings.
sys.path.insert(0, importDir)

# Create path to the example images
imageDir = os.path.join(rootDir, 'examples')

# Create path to the testing
testingDir = os.path.join(rootDir, 'testing')

# Create path to the tests
testDir = os.path.join(testingDir, 'tests')
sys.path.append(testDir)

# Create path to the test outputs
outputDir = os.path.join(testingDir, 'output')

from Firtree import *

class TestHelper:
    def __init__(self, renderer, test_name):
        self._renderer = renderer
        self._test_name = test_name

    def load_image(self, filename):
        if(os.path.isfile(os.path.join(imageDir, filename))):
            return Image.CreateFromFile(os.path.join(imageDir, filename))

        if(os.path.isfile(filename)):
            return Image.CreateFromFile(filename)

        return None

    def write_test_output(self, image):
        outfilename = '%s.png' % self._test_name
        self._renderer.WriteImageToFile(image, os.path.join(outputDir, outfilename))

def run_test(test_name, context, renderer, output):
    # Attempt to import the test module
    testmod = __import__(test_name, globals(),  locals(), [], -1)

    test = testmod.Test()

    print('Running test %s (%s)...' % (testid, test.name()))

    test.init_test()

    context.Begin()
    helper = TestHelper(renderer, test_name)
    try:
        test.run_test(context, renderer, helper)
        output.write('''
        <div class="testPassed">
        <div class="testTitle">%s - %s</div>
        <div class="testExemplar"><img src="exemplar/%s.png" alt="Exemplar" /></div>
        <div class="testOutput"><img src="output/%s.png" alt="Output" /></div>
        </div>
        ''' % (testid, test.name(), testid, testid))
    except:
        print('Exception in test:')
        traceback.print_exc()
        output.write('''
        <div class="testFailed">
        <div class="testTitle">%s - %s</div>
        </div>
        ''' % (testid, test.name()))
    helper = None
    context.End()

    test.finalise_test()

if(__name__ == '__main__'):
    tests = glob.glob(os.path.join(testDir, 'test*.py'))
    tests.sort()

    outputFile = open(os.path.join(testingDir, 'results.html'), 'w')
    outputFile.write('''<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

    <html xmlns="http://www.w3.org/1999/xhtml">
    <head>
    <meta name="generator" content="Firtree test harness" />
    <title>Firtree test results</title>
    <style type="text/css">
    html, body { background-color: black; }
    .testTitle { font-size: 120%; font-weight: bold; }
    .testPassed, .testFailed { padding: 1em; }
    .testPassed {
      background-color: #aaffaa; margin: 0.3em; 
      border-left: 1px solid #ccffcc;
      border-top: 1px solid #ccffcc;
      border-bottom: 1px solid #88ff88;
      border-right: 1px solid #88ff88;
    }
    .testFailed { 
      background-color: #ffaaaa; margin: 0.3em; 
      border-left: 1px solid #ffcccc;
      border-top: 1px solid #ffcccc;
      border-bottom: 1px solid #ff8888;
      border-right: 1px solid #ff8888;
    }

    .testExemplar, .testOutput { display: inline; }
    </style>
    </head><body>
    ''')

    context = OpenGLContext.CreateOffScreenContext(16, 16)
    renderer = GLRenderer.Create(context)

    if(len(sys.argv) > 1):
        tests = sys.argv[1:]

    for testname in tests:
        testid = os.path.splitext(os.path.basename(testname))[0]
        run_test(testid, context, renderer, outputFile)

    outputFile.write('</body></html>')
    outputFile.close()

# vim:sw=4:ts=4:et
