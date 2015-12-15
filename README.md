## Cinder Experiments
A collection of experiments, samples and other bits of code.

***WORK IN PROGRESS - Currently updating everything to Cinder 0.9***

##### [PBR Basics](/PBRBasics/src/PBRBasicsApp.cpp)
This sample show the basics of a physically based shading workflow. Mainly adapted from disney and epic papers on the subject. PBR without textures is not particularly interesting, but it's a good introduction.

![Image](/Images/MO6ufSrgB0.gif)

##### [PBR Image Based Lighting](/PBRImageBasedLighting/src/PBRImageBasedLightingApp.cpp)
Image Based Lighting Diffuse and Specular reflections. Uses Cubemaps created in [CmftStudio](https://github.com/dariomanesku/cmftStudio). This sample uses a full approximation as described on [this Unreal Engine blog post](https://www.unrealengine.com/blog/physically-based-shading-on-mobile).

![Image](/Images/r8dYbVGGWg.gif)

##### [PBR Texturing Basics](/PBRTexturingBasics/src/PBRTexturingBasicsApp.cpp)
Basic use of textures in a physically based shading workflow.

![Image](/Images/U8jbuV7Ujv.gif)

##### [Exponential Shadow Mapping](/ExponentialShadowMap/src/ExponentialShadowMapApp.cpp)
![Image](/Images/OPwIkI154Y.gif)

##### [Cascaded Shadow Mapping](/CascadedShadowMapping/src/CascadedShadowMappingApp.cpp)
![Image](/Images/8zxankRZpX.gif)
![Image](/Images/fOR3N6Pvff.gif)

##### [Viewport Array](/ViewportArray/src/ViewportArrayApp.cpp)
Small sample showing the use of ```glViewportArrayv``` and ```gl_ViewportIndex``` to render to multiple viewports.

![Image](/Images/D3aQZUpUP4.gif)

##### [Wireframe Geometry Shader](/WireframeGeometryShader/src/WireframeGeometryShaderApp.cpp)
Geometry and fragment shader for solid wireframe rendering. Mostly adapted from [Florian Boesch great post on barycentric coordinates](http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/).

![Image](/Images/YDbBnGu8UQ.gif)



##### License
Copyright (c) 2015, Simon Geilfus - All rights reserved.
This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and
the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
