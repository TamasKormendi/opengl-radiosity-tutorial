# Chapter 0 - Introduction

Hi everyone and welcome to my radiosity tutorial with C++ and OpenGL 4.5! If you're here you most likely know what radiosity is and how many different implementation strategies exist for it.

This tutorial is going to present a GPU-accelerated way of implementing progressive radiosity. Progressive radiosity was first presented in a paper by Cohen et al. in SIGGRAPH 1988, titled "A Progressive Refinement Approach to Fast Radiosity Image Generation". That paper is slightly archaic for a modern solution, however, so the two main inspirations for the approach you'll see here are Chapter 39 from GPU Gems 2 and the radiosity section from the 2006 Eurographics tutorial by Szirmay-Kalos et al. titled "GPUGI: Global Illumination Effects on the GPU".

The purpose of this tutorial is to teach you how to *implement* a radiosity renderer and thus I won't cover general radiosity theory in too much detail. If you're interested in that too I'd encourage you to read the 3 articles I mentioned previously (or take a look at my dissertation, that topic is covered in detail plus the "References" section has many, many more resources). In a post-release bonus chapter I'll also show you how a small, basic GUI is implemented using wxWidgets and how it is connected to the renderer.

Before I begin, two more things. First and foremost, my deepest thanks to Joey de Vries of https://learnopengl.com. Without his OpenGL tutorials this project would not have been possible. The basic architecture of this renderer also follows the renderer presented on his site, so if you want a complete overview of the project (not just the radiosity implementation) then do take a look at his site. I'd also like to thank my supervisor, Dr Richard Overill.

Second, the code here is not meant to be production-grade. It is meant to give you an idea how to implement a working radiosity renderer that makes decent use of system resources, so the coding style is more tailored towards understandability rather than production-grade practices. With the knowledge gained here you should be able to improve on it or write your own radiosity renderer from scratch.

Without further ado, let's begin by taking a look at the general plan.