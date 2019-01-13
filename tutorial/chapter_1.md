# Chapter 1 - Implementation plan

This chapter is meant to give you a general idea about the steps we have to take. If you don't get some of the sections, don't worry, they'll be revisited later with code samples and in more detail.

Once you read a few radiosity papers and start thinking about how you could write an implementation that's suitable for modern systems you are likely to end up with a list of questions to be answered similar to the following:

* How to represent patches?
* How to choose the shooting patch?
* How to find form factors? And if we further break this question down:
  * How to find the geometric term?
  * How to find the visibility term?
* How to distribute energy from the shooting patch to the other visible patches?
* How to find the final surface colours so they can be drawn on the screen?

Let's take a look at all of them in order!

## Patch representation

 You might know that old radiosity papers suggested subdividing big surfaces into smaller patches. In practice this meant that a surface (for example a wall) that only contained a few triangles had more vertices added to it so you could create more, smaller triangles. In this case a patch would be a triangle. Since in traditional radiosity the amount and colour of light is constant over a patch more patches lead to a better quality image. However, we aren't doing this. The previously described method couples the lighting representation and the underlying geometry rather tightly. This is not great since you'd have to render more triangles even if you just want more refined lighting (which can lead to some wasted performance) and there's no easy way to directly map the calculation to the GPU pipeline. It also raises the question about how we could subdivide the original triangles (tessellation? create new vertices manually?) etc. etc. so the way this renderer represents patches is with textures. In this case one texel (texture pixel) is going to be one patch so if we want a more detailed image we can simply change an integer to a higher value. Neat, right?

 These textures are called "lightmaps". This name is used in multiple different contexts (for example, it can be used for offline/pre-computed lighting too) but here these maps are simply going to store the radiosity values that map to a given point in the scene. Every mesh in the scene is going to have individual lightmaps and they are mapped to the mesh according to the UV coordinates of the given mesh. Of course, this approach isn't perfect either. It enforces a constraint where each mesh has to have UV map, which, in addition, can't be overlapping.

 Still, discounting this limitation, lightmaps are a good way to store lighting information since they map to the GPU pipeline well (fragment shaders anyone?) so we can easily update and make use of the lighting values we have.

 ## Choosing the shooting patch

 In theory the best way of choosing the shooting patch is selecting the patch that has the most energy that has not been shot yet (called the "radiance" or "radiant energy" of the patch). This patch contributes the most to the scene and thus if we always select the patch with the highest radiance the scene is going to converge the quickest, right? _In theory._ I tried this in a previous version of my renderer but it was about as fast as a snail in a desert. There is no easy way to map the selection operation to the GPU so I had to loop through all the radiance values on the CPU so even if you optimise this as much as possible it'll still be leagues behind a GPU accelerated solution. So let's relax the rules a bit, shall we? We should find the _mesh_ that has the highest average radiant energy then simply let all of the patches mapping to that mesh shoot.

 This is good because we can get the average radiance of a mesh by creating a mipmap pyramid of its radiance texture then taking the top level of it.

 This assumes that our lightmaps are power of 2 sized, since in this case the top level mipmap is going to have a size of 1x1 and it is going to contain the average RGB value of the whole texture. As you'll see later OpenGL does most of the heavy lifting for us in this case. After this we can use a Z-buffer sort by writing the RGB-coded ID of the mesh into a framebuffer with a depth proportional to its radiance. The end result is going to be the ID of the shooting mesh. More details in later chapters!

 ## Form factor, energy distribution and final values

 These ones might best be illustrated with code samples so I'll just give a short explanation here, with more following in their respective chapters.

### Form factor

 The geometric term of the form factor calculation relies on the distance and relative orientation of two elements (the shooter and the receiver) and also their size. It is a bit problematic to take sizes into account in a general renderer (that doesn't define a unit system) so first we'll just consider relative orientation and distance. I used a modified version of the geometric term calculation found in GPUGI, since the version given there used DirectX/HLSL so had to modify it slightly.

 The visibility term determines if the shooting patch can see a specific receiver or if it is occluded. For an incredible simple solution we can consider `visibility = 1` if it is not occluded and `visibility = 0` if it is. To determine this we can use shadow mapping. So far so simple, right? Now comes the harder part. You might've heard about "hemispheres" and "hemicubes" and whatnot when reading about radiosity. So what do these mean?

 If you cut a sphere in half along one of its axes (you'll get two hemispheres) then lay the flat side of one of the hemispheres on a surface you are going to see what the given point can "see". Radiosity deals with perfect lambertian diffuse surfaces, which means the surface is going to distribute energy evenly over the hemisphere. The hemisphere itself is similar to a camera with a 180° field of view. Not too complicated so far, right? Now the problem is that you cannot easily get OpenGL to rasterise a hemispherical projection. GPU Gems 2 Chapter 39 tries this approach and I also gave it a shot, to some rather disappointing results, i.e. the scene had a massive amount of artifacts. So let's not do this, shall we?

 What we can do, however, is approximating a hemisphere with a hemicube. This is a cube cut in half, with 1 full face and 4 half-faces. The downside is that we'll have to render the scene into a depth buffer 5 times but we'll get some rather nice looking images. I'll only present the hemicube approach but if you're adventurous you can try to get a nice looking hemisphere visibility calculation up and running.

### Energy distriution

As mentioned previously we need some information about the distance and orientation of every patch within the scene. Those of you who are familiar with deferred rendering know about a "G-buffer" (geometry buffer) that stores some geometric information - normal vectors, world-space position, etc - about the scene in a buffer. What we'll do is similar to it, but we are going to do it globally, as a pre-process step.

With the above steps (shooter selection and pre-process information gathering) we should have all the information we need about every patch to determine the amount of energy transmitted between the shooter and every other patch.

What I've not mentioned so far is that we need to store 2 lightmaps per mesh. One for the radiant/outgoing energy (as mentioned above) and one for the incoming radiant energy/irradiance. This is important so we can determine what to show when we render the scene and what to transmit to other patches. When the radiosity process starts the lightmaps of every non-lamp mesh are going to be completely dark RGB(0, 0, 0) whereas lamps are going to be completely white RGB(1, 1, 1).

We can simply add the RGB values of the actual incoming energy to the stored irradiance of every patch to get the new irradiance values after a shooter has shot its energy. Similarly we can get the new radiance value of every patch by multiplying the delta irradiance (so the irradiance values just received) by the underlying diffuse texture values. Simply add this delta radiance to the stored radiance and set the shooter's radiance to 0. We're done with a sub-iteration! (n.b. I consider a full iteration once every patch has shot its energy on a mesh.)

### Final render

We want to display our scene while the radiosity iteration is still running, creating a pseudo-real-time view of the radisoity rendering. The setup described above makes this pretty easy. When rendering the scene for the user get the diffuse and irradiance values in the fragment shader then multiply the two together. Done!

Now I've not mentioned a few techniques (texture-space rendering, texture-space multisampling with a custom resolve operation, etc. etc.) but we'll get to those in due time. Hope this chapter gave you a decent idea of what we're doing here. From the next chapter we'll dive into the code itself. I separated the radiosity functionality into a few individual functions so probably the best way to go about it is to explain each of the functions (and external changes, if needed) in the order they are used within a radiosity (sub-)iteration. Radiosity is technically just a decently complex algorithm applied to a scene many, many times so if you understand one iteration you should understand the whole thing.

I mentioned a pre-process function in this chapter which only has to be applied once for a static scene, before any of the radiosity calculations start. Let's take a look at that in the next chapter, shall we?