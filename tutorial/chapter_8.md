# Chapter 8 - Texture-space multisampling

Welcome to the last (main) chapter! Near the end of the previous tutorial we still had one, rather sigificant issue. Bars near the face edges. As a reminder:

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/noMultiNoFilter.png" alt="Image with artifacts" width="500">
</p>

 One thing to note is that the example scenes use UV islands for every face of every object. This means that we'd not have a problem when we are rendering (in texture-space) slightly outside the allocated UV space for the given face. This is going to be relevant when we also want to switch on texture filtering. Today we'll eliminate the wide black bars we saw in the previous tutorial, however, on lower lightmap resolutions there's still a tiny darker region around face edges with texture filtering. If you want to fix it, it'd definitely not be a bad exercise. I'll provide a few pointers near the end of this chapter.

Anyways, how can we fix the large black bars? Why, thank you for asking, the answer is texture-space multisampling. Now, hopefully you've heard of multisampling before but in general it's used for anti-aliasing. That's nice but we're not going to use it for that. At all. Suffice to say that OpenGL is not particularly pleased about that so we'll need a few workarounds.

Before that, let's take a step back. What's causing the black bars in the first place? When we render in texture-space we do it at a rather low resolution (by default 32*32, which is only 1024 lightmap pixels per object (not face!) and in most cases it's fewer because we don't use the whole UV space) so it is possible that some pixel centres would lie outside the face but one part of that (say, the left 25% of a pixel) would still lie on the face. The rasteriser only invokes the fragment/pixel shader for a given pixel if a triangle covers the centre of a pixel. As I said previously, a pixel can be partly on a face but its centre isn't. This leads to the fragment shader not being invoked and - you guessed it - the pixel being left at its previous colour which, in this case, is RGB(0, 0, 0) - completely black. Good thing there's a way to subdivide a pixel into smaller samples and check the coverage that way eh?

That's where multisampling enters the picture. With multisampling it is fine if a triangle doesn't hit the centre of the pixel - it only has to hit the centre for one of the sample points of a pixel. In traditional usage (AA) the sample values would be averaged leading to a nice, smooth, anti-aliased image. This usage is so common that it can be done in a few lines of code.

Unfortunately our job isn't that easy. First, let's identify where we need texture-space multisampling. As you might've guessed, we'll need it in the functions where we render in texture-space - the preprocess and lightmap update steps. Second, let's see what we *actually* want to do. Normal multisampling averages sample values, which means that instead of the black bars we'd simply have darker colours near the edge - the colour we want (the properly covered and thus shaded samples) would be averaged with unshaded sample values. It'd also throw off the preprocess since non-colour (e.g. normal vector) data is handled there. So that's not an option for us. The sample values are either going to be the value that should go into the whole pixel (the shaded samples) and the previous value of the unshaded samples. What if we took the *maximum* value of a sample from the pixel and applied it to the whole pixel? It would...write the value we need! As seems to be the running theme in this chapter (for example, what happens in the case of a normal vector that points to (-1, 0, 0)? It's not bigger than (0, 0, 0), is it?), there's always a catch but don't worry, we'll get to that.

So let's get to the final catch and transition to the part where I introduce the solutions. So the last catch is that we don't want the values that we'd get if we sampled the *centre* of a pixel if we don't cover the centre (what'd happen with our world-space position texture?). We want to sample at the centre of the covered area, don't we? Luckily, we have *centroid* sampling. It's not even hard to implement, basically we just set our `in` and `out` variables in the shader programs to `centroid in` and `centroid out`. That's the only difference needed for our preprocess and lightmap update shader programs.

Now how are we going to do that max sampling? Multisampled textures have to be resolved which can be done more-or-less automatically by blitting but that does the equivalent of the aforementioned average sampling. It is definitely not what we need so let's write our custom resolve pass.

This is going to be a render pass like any other (just a *bit* different) so we need to actually draw some triangles. As we're doing this in texture-space we only need a screen-aligned quad. We also use an "intermediate" framebuffer to render into. Since I've already covered the base versions of the preprocess and lightmap update functions I'm only going to show extracts of the multisampled versions, refer to the code for...well, the whole code. Don't forget that a multisampled texture is created slightly differently than a normal one is. Refer to the full code if you want a reminder on how it's done. Also note that I refactored the non-multisampled lightmap update a bit so, for example, visibility textures get bound outside the render loop. I haven't got around to doing this for the multisampled variant yet but functionally they should be equivalent.

But I digress, let's get to the resolve step. This code is from the multisampled preprocess step, but the idea is the same for the multisampled lightmap update.

```cpp
		//Do a custom resolve pass
		glBindFramebuffer(GL_FRAMEBUFFER, intermediateFramebuffer);
		glClear(GL_COLOR_BUFFER_BIT);

		resolveShader.useProgram();
		glBindVertexArray(screenAlignedQuadVAO);

		glActiveTexture(GL_TEXTURE0);
		resolveShader.setUniformInt("multisampledPosTexture", 0);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, worldspacePosData);

		glActiveTexture(GL_TEXTURE1);
		resolveShader.setUniformInt("multisampledNormalTexture", 1);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, worldspaceNormalData);

		glActiveTexture(GL_TEXTURE2);
		resolveShader.setUniformInt("multisampledIDTexture", 2);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, idData);

		glActiveTexture(GL_TEXTURE3);
		resolveShader.setUniformInt("multisampledUVTexture", 3);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, uvData);


		glDrawArrays(GL_TRIANGLES, 0, 6);
```

The interesting thing here is going to be our resolve pass fragment shader. The resolve pass vertex shader is a simple passthrough vertex shader that we also used for the shooter mesh selection quad so I opted to reuse that. Let's look at the fragment shader for the preprocess resolve.

PreprocessResolve.fs
```glsl
#version 450 core

layout (location = 0) out vec3 posData;
layout (location = 1) out vec3 normalData;
layout (location = 2) out vec3 idData;
layout (location = 3) out vec3 uvData;

uniform sampler2DMS multisampledPosTexture;
uniform sampler2DMS multisampledNormalTexture;
uniform sampler2DMS multisampledIDTexture;
uniform sampler2DMS multisampledUVTexture;
```

We need to sample multisampled textures a bit differently. First step is, they have to be declared as `sampler2DMS` instead of `sampler2D`.

```glsl
void main() {
    vec3 finalPos = vec3(0.0, 0.0, 0.0);
    vec3 finalNormal = vec3(0.0, 0.0, 0.0);
    vec3 finalID = vec3(0.0, 0.0, 0.0);
    vec3 finalUV = vec3(0.0, 0.0, 0.0);
```

We declare all of the vectors we'll use.

```glsl
    for (int i = 0; i <8; ++i) {
        //texelFetch is needed to sample multisampled textures
        vec3 fetchedPos = texelFetch(multisampledPosTexture, ivec2(gl_FragCoord.xy), i).rgb;
        vec3 fetchedNormal = texelFetch(multisampledNormalTexture, ivec2(gl_FragCoord.xy), i).rgb;
        vec3 fetchedID = texelFetch(multisampledIDTexture, ivec2(gl_FragCoord.xy), i).rgb;
        vec3 fetchedUV = texelFetch(multisampledUVTexture, ivec2(gl_FragCoord.xy), i).rgb;

        //Need to initialise them this way so their min value is accurate
        if (i == 0) {
            finalPos = fetchedPos;
            finalNormal = fetchedNormal;
            finalID = fetchedID;
            finalUV = fetchedUV;
        }

        finalPos = max(finalPos, fetchedPos);
        finalNormal = max(finalNormal, fetchedNormal);
        finalID = max(finalID, fetchedID);
        finalUV = max(finalUV, fetchedUV);
    }
```

I've not mentioned the sample count we're using. I had to experiment with it a bit since unlike in the case of AA higher sample counts did not necessarily lead to better results. I found 8 samples per pixel to be the sweet spot.

Anyways, we want to get the value for each individual sample in a pixel. For this we have to loop through all of them. Thing is, the way we have to retrieve values from a multisampled texture is with `texelFetch()`, `texture()` does not work, unfortunately. `texelFetch()`, however, expects the coordinate of the texel/pixel we want to fetch from the texture, which is in the range of [0, textureSize] instead of [0, 1]. We can get this information from a handy built-in fragment shader variable called `gl_FragCoord`. `texelFetch()` also requires an integer vector `ivec` for the pixel coordinate so we use that.

We can solve the problem with max sampling by simply initialising vectors to the first sample we have instead of (0, 0, 0). Once that's done, we do the max sampling for each of the samples then assign our working vectors to our output vectors:

```glsl
    posData = finalPos;
    normalData = finalNormal;
    idData = finalID;
    uvData = finalUV;
}
```

...and we're done! This last step isn't even needed if you use the declared output vectors instead of separate vectors. The idea is the same for the lightmap resolve as well, just with the ir/radiance textures.

After the resolve pass the textures are used in the exact same way as the non-multisampled versions used them.

This is how the previous picture looks like, with texture-space multisampling but without texture filtering:

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/multiNoFilter.png" alt="Image without artifacts" width="500">
</p>

If we switch on texture filtering for our lightmaps:

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/multiFilter.png" alt="Image without artifacts, with texture filtering" width="500">
</p>

It also gets us other, quite decent looking images, like these:

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/complexLights_4.png" alt="Quite decent looking image 1" width="500">
</p>

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/testScene_128_1.png" alt="Quite decent looking image 2" width="500">
</p>

<p align="center">
    <img src="https://raw.githubusercontent.com/TamasKormendi/opengl-radiosity-tutorial/master/tutorial/images/testScene_128_3.png" alt="Quite decent looking image 3" width="500">
</p>

These images were all produced by the renderer, using "pure" radiosity, that is, there is no ambient lighting or other lighting techniques used. The resolution of the lightmaps is 128*128. The radiosity loop was run for about 40 minutes on a GeForce GTX 560M and an i7 2630QM for these images. If you're curious, the repo has a few more images and if you want to know more about how these images were produced (number of patches processed, exact time it took to produce images with different lightmap resolutions, etc.) then take a look at the Evaluation section of my dissertation.

Finally, as promised, some tips for fixing the minor black bars on lower lightmap resolutions with texture filtering: they happen because the filtering algorithm averages neighbouring pixels to smooth the texture, which means that even unshaded pixels might sneak into the process, when it happens at texture edges. The solution is edge padding. You can read more about that here: http://wiki.polycount.com/wiki/Edge_padding. For our purposes, the padding would have to have the same colour as the very edge of the face. In other words, the lightmap would have to be slightly stretched outwards so the filtering algorithm would never encounter unshaded pixels.

Well, that has to be it from me, thanks very much for reading and I hope you enjoyed this series and found it at least a bit useful. There's a bonus chapter coming up (about the frontend) but this concludes the tutorial for the renderer.

Again, thanks for reading and see you around!

Tamás Körmendi