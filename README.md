## Cinder Experiments
Samples, experiments and other bits of code that doesn't deserve a repository on their own.

***WORK IN PROGRESS - Currently updating everything to Cinder 0.9***

##### [PBR Basics](/PBRBasics/src/PBRBasicsApp.cpp)
This sample show the basics of a physically based shading workflow. Mainly adapted from disney and epic papers on the subject. PBR without textures is not particularly interesting, but it's a good introduction.

![Image](https://c1.staticflickr.com/1/582/23359423301_7ac7ecf293_o.gif)

##### [PBR Image Based Lighting](/PBRImageBasedLighting/src/PBRImageBasedLightingApp.cpp)
Image Based Lighting Diffuse and Specular reflections. Use Cubemaps created in [CmftStudio](https://github.com/dariomanesku/cmftStudio). This sample uses a full approximation as described on [this Unreal Engine blog post](https://www.unrealengine.com/blog/physically-based-shading-on-mobile).

![Image](https://c1.staticflickr.com/1/665/23415737176_f9378210a5_o.gif)

##### [PBR Texturing Basics](/PBRTexturingBasics/src/PBRTexturingBasicsApp.cpp)
Basic use of textures in a physically based shading workflow.

##### [Exponential Shadow Mapping](/ExponentialShadowMap/src/ExponentialShadowMapApp.cpp)
##### [Cascaded Shadow Mapping](/CascadedShadowMapping/src/CascadedShadowMappingApp.cpp)
![Image](https://c2.staticflickr.com/6/5791/23073995959_545492c54c_o_d.gif)
![Image](https://c1.staticflickr.com/1/624/22813606734_c3e293dc9f_o.gif)

##### [Viewport Array](/ViewportArray/src/ViewportArrayApp.cpp)
Small sample showing the use of ```glViewportArrayv``` and ```gl_ViewportIndex``` to render to multiple viewports.

![Image](https://c2.staticflickr.com/6/5641/23415803906_880f6eeccb_o.gif)

##### [Wireframe Geometry Shader](/WireframeGeometryShader/src/WireframeGeometryShaderApp.cpp)
Geometry and fragment shader for solid wireframe rendering.

![Image](https://c2.staticflickr.com/6/5726/23415788246_a5513fc938_o.gif)



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
