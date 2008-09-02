// FIRTREE
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License version 2 for more details.
//
// You should have received a copy of the GNU General Public License version 2
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

%module Firtree
%{
#include <firtree/blob.h>
#include <firtree/image.h>
#include <firtree/main.h>
#include <firtree/math.h>
#include <firtree/kernel.h>
#include <firtree/glsl-runtime.h>

using namespace Firtree;

%}

#ifdef SWIGPYTHON

%apply unsigned int { uint32_t }
%apply int { int32_t }

// %feature("ref")   Firtree::ReferenceCounted "printf(\"Ref: %p\\n\", $this);"
// %feature("unref") Firtree::ReferenceCounted "printf(\"Unref: %p\\n\", $this); $this->Release();"

%feature("ref")   Firtree::ReferenceCounted ""
%feature("unref") Firtree::ReferenceCounted "$this->Release();"

%newobject Firtree::Blob::Create;
%newobject Firtree::Blob::CreateWithLength;
%newobject Firtree::Blob::CreateFromBuffer;
%newobject Firtree::Blob::CreateFromBlob;

%newobject Firtree::SamplerParameter::CreateFromImage;

%newobject Firtree::Kernel::CreateFromSource;

%newobject Firtree::Image::CreateFromImage;
%newobject Firtree::Image::CreateFromBitmapData;
%newobject Firtree::Image::CreateFromFile;
%newobject Firtree::Image::CreateFromImageWithTransform;
%newobject Firtree::Image::CreateFromImageCroppedTo;
%newobject Firtree::Image::CreateFromKernel;
%newobject Firtree::Image::CreateFromImageProvider;

%newobject Firtree::AccumulationImage::Create;

%newobject Firtree::BitmapImageRep::Create;
%newobject Firtree::BitmapImageRep::CreateFromBitmapImageRep;

%newobject Firtree::Parameter::Create;

%newobject Firtree::AffineTransform::Identity;
%newobject Firtree::AffineTransform::FromTransformStruct;
%newobject Firtree::AffineTransform::Scaling;
%newobject Firtree::AffineTransform::RotationByDegrees;
%newobject Firtree::AffineTransform::RotationByRadians;
%newobject Firtree::AffineTransform::Translation;

%newobject Firtree::OpenGLContext::CreateNullContext;
%newobject Firtree::OpenGLContext::CreateOffScreenContext;

%newobject Firtree::GLRenderer::Create;
%newobject Firtree::GLRenderer::CreateBitmapImageRepFromImage;

%newobject Firtree::ExtentProvider::CreateStandardExtentProvider;

%newobject Firtree::RectComputeTransform;

/* Some cleverness to allow us to wire up python callables
 * as an extent provider */

%{
    class PythonExtentProvider : public Firtree::ExtentProvider
    {
        public:
            PythonExtentProvider(PyObject* pyfunc)
                :   Firtree::ExtentProvider()
                ,   m_Function(pyfunc)
            {
                Py_INCREF(m_Function);
            }

            virtual ~PythonExtentProvider()
            {
                Py_DECREF(m_Function);
            }

            virtual Rect2D ComputeExtentForKernel(Kernel* kernel)
            {
                PyObject *arglist, *pyKernel;
                PyObject *result;

                pyKernel = 
                    SWIG_NewPointerObj(SWIG_as_voidptr(kernel), 
                            SWIGTYPE_p_Firtree__Kernel, 0);
                arglist = Py_BuildValue("(O)", pyKernel);

                result = PyEval_CallObject(m_Function, arglist);

                Py_XDECREF(arglist);

                Rect2D resultRect = Firtree::Rect2D();

                if(result)
                {
                    Rect2D *r;
                    int ok = SWIG_ConvertPtr(result, (void**)(&r), 
                            SWIGTYPE_p_Firtree__Rect2D, SWIG_POINTER_EXCEPTION);
                    if(ok == 0)
                    {
                        resultRect = *r;
                    }
                }

                Py_XDECREF(result);

                return resultRect;
            }

        private:
            PyObject*       m_Function;
    };
%}

/* Grab a Python function object as a Python object. */
%typemap(in) PyObject *pyfunc {
  if (!PyCallable_Check($input)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return NULL;
  }
  $1 = $input;
}

/* Attach a new method to Image to use a Python callable
 * as an extent provider */
%extend Firtree::Image {
    static Image* CreateFromKernel(Firtree::Kernel* k, PyObject* pyfunc)
    {
        ExtentProvider* ep = new PythonExtentProvider(pyfunc);
        Image* retVal = Firtree::Image::CreateFromKernel(k, ep);
        FIRTREE_SAFE_RELEASE(ep);
        return retVal;
    }
}

/* Convert from Python --> C */
%typemap(in) uint32_t {
    $1 = PyInt_AsLong($input);
}

/* Convert from C --> Python */
%typemap(out) uint32_t {
    $result = PyInt_FromLong($1);
}

%exception {
    try {
        $action
    } catch ( Firtree::Exception& e ) {
        std::string message = e.GetFunction();
        message += ": ";
        message += e.GetMessage();
        PyErr_SetString(PyExc_RuntimeError, const_cast<char*>(message.c_str()));
        return NULL;
    }
}
#endif

%include <firtree/main.h>
%include <firtree/math.h>
%include <firtree/image.h>
%include <firtree/kernel.h>
%include <firtree/glsl-runtime.h>

/* So that VIM does the "right thing"
 * vim:cindent:sw=4:ts=4:et
 */
