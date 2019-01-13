# Progressive radiosity tutorial with OpenGL

## What is this?
Radiosity is a global illumination algorithm - like ray tracing or path tracing but unlike the latter two it is hard (if not impossible) to find a modern tutorial for it. At least it used to e, up until now! This repository contains the tutorial series that describes how progressive radiosity can be implemented with GPU acceleration. You can also find the source code of the GPU-accelerated radiosity renderer I developed for my 3rd year project at King's College London, which forms the basis of the tutorials. It also contains a few test scenes and my dissertation that accompanied my code.

Overall, the tutorials are meant to be a more detailed and in-depth version of the implementation section of the dissertation, with code samples and images, where relevant. The tutorials are written with an intermediate-to-advanced OpenGL audience audience in mind so do brush up on the basics before starting the series!

## Coding style
The code was not meant to be production-grade, it was written with readability and understandability in mind. It was also my first OpenGL/graphics programming project so do let me know if you see any blatant violations of common sense. The code targets C++17 and OpenGL 4.5.

## Dependencies
GLFW 3.2.1: http://www.glfw.org/download.html

GLEW 2.1.0: http://glew.sourceforge.net/

wxWidgets 3.10: https://www.wxwidgets.org/

Assimp 4.0.1 http://assimp.org/index.html

GLM 0.9.8.5 https://glm.g-truc.net/0.9.8/index.html

stb_image.h https://github.com/nothings/stb/blob/master/stb_image.h

## Controls
* WASD and mouse: navigation.
* ESC: quit program.
* U: switch on ambient light.
* I: switch off ambient light.
* Left mouse button: place light source at current location.
* E: initiate preprocess step. Can only be done once. It must be done after lamps are placed and once it is initiated no more lamps should be placed.
* Right mouse button: start radiosity iteration.
* Q: stop radiosity iteration.
* R: reload every currently loaded shader program.

## Licence information
Unless explicitly stated otherwise, all the material in the repository is licensed under the terms of CC BY-NC 4.0: https://creativecommons.org/licenses/by-nc/4.0/legalcode 

The base architecture of the renderer is based on the tutorials presented on https://learnopengl.com/, written by Joey de Vries (https://twitter.com/JoeyDeVriez), licensed under the terms of CC BY-NC 4.0: https://creativecommons.org/licenses/by-nc/4.0/legalcode. Many thanks for the tutorials!

The test scenes are based on the Cornell box (http://www.graphics.cornell.edu/online/box/), by the Cornell University.