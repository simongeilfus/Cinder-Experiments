#### [Parallax Corrected Cubemap](/src/ParallaxCorrectedCubemapApp.cpp)
Cubemap environment mapping is the most straightforward way to add reflection to a scene. Usually cubemaps reflections represent infinitely far away reflections. This sample shows how to correct the texture lookup to have proper local reflections. This is not shown in this sample but this can also be used to fake small local light sources as well. The sample uses lightmapping to keep the code simple.

Here's some interesting links on the subject :  
https://seblagarde.wordpress.com/2012/11/28/siggraph-2012-talk/
https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
http://c0de517e.blogspot.be/2015/03/being-more-wrong-parallax-corrected.html
http://www.clicktorelease.com/blog/making-of-cruciform
http://gpupro.blogspot.be/2013/02/gpu-pro-4-practical-planar-reflections.html
http://learnopengl.com/#!Advanced-OpenGL/Cubemaps
http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf

![Image](../Images/ParallaxCorrectedCubemap.jpg)

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
