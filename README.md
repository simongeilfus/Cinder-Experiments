## Cinder Experiments
A collection of experiments, samples and other bits of code.

##### [Cascaded Shadow Mapping](/CascadedShadowMapping/src/CascadedShadowMappingApp.cpp)
Cascaded Shadow Mapping is a common method to get high resolution shadows near the viewer. This sample shows the very basic way of using this technique by splitting the frustum into different shadow maps. CSM has its own issues but usually provides better shadow resolution near the viewer and lower resolutions far away.  
![Image](/Images/CascadedShadowMapping0.jpg)
![Image](/Images/CascadedShadowMapping1.jpg)

Here's a few references :  
https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/
http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
https://software.intel.com/en-us/articles/sample-distribution-shadow-maps
http://blogs.aerys.in/jeanmarc-leroux/2015/01/21/exponential-cascaded-shadow-mapping-with-webgl/
https://github.com/NVIDIAGameWorks/OpenGLSamples/blob/master/samples/gl4-maxwell/CascadedShadowMapping/CascadedShadowMappingRenderer.cpp

##### [Color Grading](/ColorGrading/src/ColorGradingApp.cpp)
This sample shows a really easy way to add proper color grading to your apps. The trick is to store a 3d color lookup table and to use it to filter the output of a fragment shader. The nice thing about it is that you can use Photoshop or any other editing tool to create the right look and then replicate the exact same grading at a really low cost (the cost of one extra texture sample per fragment).  

Press 'e' top open photoshop and live edit the color grading. When the file is saved in photoshop, the app automatically reloads the color grading.  
![Image](/Images/ColorGrading.jpg)

##### [Exponential Shadow Mapping](/ExponentialShadowMap/src/ExponentialShadowMapApp.cpp)
![Image](/Images/ExponentialShadowMap.jpg)

A few interesting links :  
http://advancedgraphics.marries.nl/presentationslides/13_exponential_shadow_maps.pdf
http://www.sunandblackcat.com/tipFullView.php?l=eng&topicid=35
http://nolimitsdesigns.com/tag/exponential-shadow-map/
http://web4.cs.ucl.ac.uk/staff/j.kautz/publications/esm_gi08.pdf
https://pixelstoomany.wordpress.com/2008/06/12/a-conceptually-simpler-way-to-derive-exponential-shadow-maps-sample-code/
http://www.olhovsky.com/2011/07/exponential-shadow-map-mFiltering-in-hlsl/

##### [Parallax Corrected Cubemap](/ParallaxCorrectedCubemap/src/ParallaxCorrectedCubemapApp.cpp)
![Image](/Images/ParallaxCorrectedCubemap.jpg)

##### [PBR Basics](/PBRBasics/src/PBRBasicsApp.cpp)
This sample show the basics of a physically based shading workflow. Mainly adapted from disney and epic papers on the subject. PBR without textures is not particularly interesting, but it's a good introduction.

![Image](/Images/PBRBasics.jpg)

##### [PBR Image Based Lighting](/PBRImageBasedLighting/src/PBRImageBasedLightingApp.cpp)
Image Based Lighting Diffuse and Specular reflections. Uses Cubemaps created in [CmftStudio](https://github.com/dariomanesku/cmftStudio). This sample uses a full approximation as described on [this Unreal Engine blog post](https://www.unrealengine.com/blog/physically-based-shading-on-mobile).

![Image](/Images/PBRImageBasedLighting0.jpg)
![Image](/Images/PBRImageBasedLighting1.jpg)

##### [PBR Texturing Basics](/PBRTexturingBasics/src/PBRTexturingBasicsApp.cpp)
Basic use of textures in a physically based shading workflow.

![Image](/Images/PBRTexturingBasics0.jpg)
![Image](/Images/PBRTexturingBasics1.jpg)

##### [Tessellated Noise](/TessellatedNoise/src/TessellatedNoiseApp.cpp)
![Image](/Images/TessellatedNoise.jpg)

##### [Tessellation Shader](/TessellationShader/src/TessellationShaderApp.cpp)
![Image](/Images/TessellationShader.jpg)

##### [Viewport Array](/ViewportArray/src/ViewportArrayApp.cpp)
Small sample showing the use of ```glViewportArrayv``` and ```gl_ViewportIndex``` to render to multiple viewports.

![Image](/Images/ViewportArray.jpg)

##### [Wireframe Geometry Shader](/WireframeGeometryShader/src/WireframeGeometryShaderApp.cpp)
Geometry and fragment shader for solid wireframe rendering. Mostly adapted from [Florian Boesch great post on barycentric coordinates](http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/).

![Image](/Images/WireframeGeometryShader.jpg)



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
