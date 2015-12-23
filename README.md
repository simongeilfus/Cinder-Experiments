## Cinder Experiments
A collection of experiments, samples and other bits of code.

#### [Cascaded Shadow Mapping](/CascadedShadowMapping/src/CascadedShadowMappingApp.cpp)
Cascaded Shadow Mapping is a common method to get high resolution shadows near the viewer. This sample shows the very basic way of using this technique by splitting the frustum into different shadow maps. CSM has its own issues but usually provides better shadow resolution near the viewer and lower resolutions far away. The sample uses ESM for the shadowing algorithm (see the [ESM sample](/ExponentialShadowMap) for more infos about ESM).  

One easy improvement to this sample is to use the approach shown in the [GpuParrallelReduction sample](/GpuParrallelReduction) to find the minimum and maximum depth of the scene and use those values to better fit what the viewer see from the scene. Other approaches involve, better frustum culling, better splitting scheme or more stable samples distributions.

Some references :  
https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/
http://http.developer.nvidia.com/GPUGems3/gpugems3_ch10.html
https://software.intel.com/en-us/articles/sample-distribution-shadow-maps
http://blogs.aerys.in/jeanmarc-leroux/2015/01/21/exponential-cascaded-shadow-mapping-with-webgl/
https://github.com/NVIDIAGameWorks/OpenGLSamples/blob/master/samples/gl4-maxwell/CascadedShadowMapping/CascadedShadowMappingRenderer.cpp  

![Image](/Images/CascadedShadowMapping0.jpg)
![Image](/Images/CascadedShadowMapping1.jpg)

#### [Color Grading](/ColorGrading/src/ColorGradingApp.cpp)
This sample shows a really easy way to add proper color grading to your apps. The trick is to store a 3d color lookup table and to use it to filter the output of a fragment shader. The nice thing about it is that you can use Photoshop or any other editing tool to create the right look and then replicate the exact same grading at a really low cost (the cost of one extra texture sample per fragment).  

Press 'e' top open photoshop and live edit the color grading. When the file is saved in photoshop, the app automatically reloads the color grading.  

![Image](/Images/ColorGrading.jpg)

#### [Exponential Shadow Mapping](/ExponentialShadowMap/src/ExponentialShadowMapApp.cpp)
Shadow Mapping is a vast subject and every approach comes with their own downsides. Basic shadow mapping have precision, aliasing,shadow acne and peter-panning issues, variance shadow mapping improves this but introduces light bleeding, etc... Exponential shadow mapping is an easy and inexpensive way to get rid of most of the above, but it (of course) comes with its own issues as well. The nice thing is that the shadow map can be inexpensively filtered in screenspace to produce softer shadows. On the other hand the main issue with ESM is that the closer a shadow is to the caster the brighter the shadow will be. Which may look weird in some cases. This is more or less fixed by using an "over-darkening" value but it doesn't work all the time.  

A few interesting links :  
http://advancedgraphics.marries.nl/presentationslides/13_exponential_shadow_maps.pdf
http://www.sunandblackcat.com/tipFullView.php?l=eng&topicid=35
http://nolimitsdesigns.com/tag/exponential-shadow-map/
http://web4.cs.ucl.ac.uk/staff/j.kautz/publications/esm_gi08.pdf
https://pixelstoomany.wordpress.com/2008/06/12/a-conceptually-simpler-way-to-derive-exponential-shadow-maps-sample-code/
http://www.olhovsky.com/2011/07/exponential-shadow-map-mFiltering-in-hlsl/

![Image](/Images/ExponentialShadowMap.jpg)

#### [Gpu Parrallel Reduction](/GpuParrallelReduction/src/GpuParrallelReductionApp.cpp)
Not a particularly exciting sample but a usefull technique. It can be use to gatter the average brightness of a scene and improve tonemapping, or to get the minimum and maximal depth and improve shadow mapping algorithm, etc... 

The sample simply show how to use the different mipmap level of a texture to progressively reduce its size until its reasonable to copy it back to the cpu and read the results.

#### [Parallax Corrected Cubemap](/ParallaxCorrectedCubemap/src/ParallaxCorrectedCubemapApp.cpp)
Cubemap environment mapping is the most straightforward way to add reflection to a scene. Usually cubemaps reflections represent infinitely far away reflections. This sample shows how to correct the texture lookup to have proper local reflections. This is not shown in this sample but this can also be used to fake small local light sources as well. The sample uses lightmapping to keep the code simple.

Here's some interesting links on the subject :  
https://seblagarde.wordpress.com/2012/11/28/siggraph-2012-talk/
https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
http://c0de517e.blogspot.be/2015/03/being-more-wrong-parallax-corrected.html
http://www.clicktorelease.com/blog/making-of-cruciform
http://gpupro.blogspot.be/2013/02/gpu-pro-4-practical-planar-reflections.html
http://learnopengl.com/#!Advanced-OpenGL/Cubemaps
http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf

![Image](/Images/ParallaxCorrectedCubemap.jpg)

#### [PBR Basics](/PBRBasics/src/PBRBasicsApp.cpp)
This sample show the basics of a physically based shading workflow. Mainly adapted from disney and epic papers on the subject. PBR without textures is not particularly interesting, but it's a good introduction.

![Image](/Images/PBRBasics.jpg)

#### [PBR Image Based Lighting](/PBRImageBasedLighting/src/PBRImageBasedLightingApp.cpp)
Image Based Lighting Diffuse and Specular reflections. Uses Cubemaps created in [CmftStudio](https://github.com/dariomanesku/cmftStudio). This sample uses a full approximation as described on [this Unreal Engine blog post](https://www.unrealengine.com/blog/physically-based-shading-on-mobile).

![Image](/Images/PBRImageBasedLighting0.jpg)
![Image](/Images/PBRImageBasedLighting1.jpg)

#### [PBR Texturing Basics](/PBRTexturingBasics/src/PBRTexturingBasicsApp.cpp)
Basic use of textures in a physically based shading workflow.

![Image](/Images/PBRTexturingBasics0.jpg)
![Image](/Images/PBRTexturingBasics1.jpg)

#### [Tessellated Noise](/TessellatedNoise/src/TessellatedNoiseApp.cpp)
Useless update to the [Tessellation Shader sample](/TessellationShader/). Just playing around with noise sums in the vertex shader. Shading is absolutely wrong.  

![Image](/Images/TessellatedNoise.jpg)

#### [Tessellation Shader](/TessellationShader/src/TessellationShaderApp.cpp)
Small sample implementing Philip Rideout article on ["Triangle Tessellation with OpenGL 4.0"](http://prideout.net/blog/?p=48).  

![Image](/Images/TessellationShader.jpg)

#### [Viewport Array](/ViewportArray/src/ViewportArrayApp.cpp)
Small sample showing the use of ```glViewportArrayv``` and ```gl_ViewportIndex``` to render to multiple viewports. ```glViewportArrayv``` is a nice way to specifies a list of viewports that can be later used in the geometry shader. By setting gl_ViewportIndex in the geometry shader you can re-direct your drawing calls to a specific viewport. Used along arrays of projections and view matrices it really ease the setup of a multiple viewport / 3d editor like view.  

![Image](/Images/ViewportArray.jpg)

#### [Wireframe Geometry Shader](/WireframeGeometryShader/src/WireframeGeometryShaderApp.cpp)
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
