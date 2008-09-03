import sys
import os
import glob
import traceback
import md5
import sys
import cgi

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

def sumfile(fobj):
    '''Returns an md5 hash for an object with read() method.'''
    m = md5.new()
    while True:
        d = fobj.read(8096)
        if not d:
            break
        m.update(d)
    return m.hexdigest()


def md5sum(fname):
    '''Returns an md5 hash for file fname, or stdin if fname is "-".'''
    if fname == '-':
        ret = sumfile(sys.stdin)
    else:
        try:
            f = file(fname, 'rb')
        except:
            return 'Failed to open file'
        ret = sumfile(f)
        f.close()
    return ret

class TestHelper:
    def __init__(self, renderer, test_name):
        self._renderer = renderer
        self._test_name = test_name
        self._checksum = None

        outfilename = '%s.png' % self._test_name
        self._outputFileName = os.path.join(outputDir, outfilename)

        if(self.output_exists()):
            # Remove the output if it already exists
            os.remove(self._outputFileName)

    def output_exists(self):
        return os.path.exists(self._outputFileName) and os.path.isfile(self._outputFileName)

    def load_image(self, filename):
        if(os.path.isfile(os.path.join(imageDir, filename))):
            return Image.CreateFromFile(os.path.join(imageDir, filename))

        if(os.path.isfile(filename)):
            return Image.CreateFromFile(filename)

        return None
    
    def checksum(self):
        return self._checksum

    def write_test_output(self, image):
        self._renderer.WriteImageToFile(image, self._outputFileName)
        if(self.output_exists()):
            self._checksum = md5sum(self._outputFileName )

def run_test(test_name, context, renderer, output):
    # Attempt to import the test module
    testmod = __import__(test_name, globals(),  locals(), [], -1)

    test = testmod.Test()

    print('Running test %s (%s)...' % (testid, test.name()))

    test.init_test()

    context.Begin()
    helper = TestHelper(renderer, test_name)
    testPassed = False
    exceptionStr = ''

    try:
        test.run_test(context, renderer, helper)

        print('  - output hash:   %32s' % helper.checksum())
        print('  - expected hash: %32s' % test.expected_hash())

        if(helper.checksum() == test.expected_hash()):
            testPassed = True
            print('    - PASSED')
        else:
            print('    - FAILED')

    except:
        exceptionStr = traceback.format_exc()
        print("Raised exception:\n%s" % exceptionStr)

    if(testPassed):
        output.write('''
        <div class="testPassed">
        <div class="testTitle">%s - %s</div>
        <div class="testOutput"><img src="output/%s.png" alt="Output" /></div>
        </div>
        ''' % (testid, test.name(), testid))
    else:
        outputStr = ''
        if(helper.output_exists()):
            outputStr = ('<div class="testOutput"><img src="output/%s.png" ' +
                'alt="Output" /></div>') % testid
        output.write('''
        <div class="testFailed">
        <div class="testTitle">%s - %s</div>
        %s
        <div class="testFailString">%s</div>
        </div>
        ''' % (testid, test.name(), outputStr, cgi.escape(exceptionStr)))

    helper = None
    context.End()

    test.finalise_test()

    return testPassed

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

    testCount = 0
    passedCount = 0

    for testname in tests:
        testid = os.path.splitext(os.path.basename(testname))[0]
        status = run_test(testid, context, renderer, outputFile)
        testCount += 1
        if(status):
            passedCount += 1

    outputFile.write('</body></html>')
    outputFile.close()

    print('%i/%i tests passed' % (passedCount, testCount))

# vim:sw=4:ts=4:et
