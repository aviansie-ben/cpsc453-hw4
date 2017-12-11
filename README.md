# CPSC 453 HW4

Benjamin Thomas, 10125343, T04

## Building/Running

There are two ways to build and run the ray tracer depending on whether you want to use it with the
`--yours` and `--default` arguments or with much more flexibility. To build and run the simplified
version:

1. `cd` into the `wrapper` directory
2. Run `make`
3. Run `./rayTracer --default` or `./rayTracer --yours` to generate images

To build without the wrapper so that more complicated command-line arguments can be used:

1. Create and `cd` into a new build directory
2. Run `cmake -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG" [path to project root]`
  - While it is possible to build and use the raytracer with optimizations disabled, the performance
    penalty from doing so is quite extreme; thus, use of compiler optimizations is highly
    recommended
3. Run `make`
4. Run `./hw4 [options] <scene file>` to generate images (for more information about available
   options, run `./hw4 -h`)

## Features

The raytracer uses a modified version of the scene file format used in HW3 to load its scenes. Thus,
it is capable of handling any arbitrary scene made up of triangulated meshes and sphere. In terms of
scenes and rendering, the ray tracer supports:

- Any number of perspective cameras with per-camera position, orientation, and horizontal field of
  view
- Grid-based supersampling for anti-aliasing
- Controllable maximum recursion depth
- Controllable recursive ray bias
- Spheres and OBJ models as objects
- Phong illumination and shading with any number of point lights
  - Controllable light attenuation coefficients
- Diffuse textures read from image files
- Shadows calculated using shadow rays
  - Partially occlusive surfaces (in terms of lighting and shadows)
- Fully and partially reflective surfaces using perfect specular reflection
- Fully and partially refractive surfaces (so long as they are _well-nested_)
  - Simulation of total internal reflection
  - Note: Does **not** simulate Fresnel reflection

Internally, the raytracer supports several techniques for speeding up the rendering process:

- Automatric construction and use of bounding volume hierarchies to speed up ray intersection
  finding
  - One-per-scene object BVH
  - One-per-model triangle BVH
- Parallelism through splitting an image into 8x8 pixel "patches"

The raytracer also prints a small preview image to the console (so long as your terminal emulator
supports [True Color](https://gist.github.com/XVilka/8346728)) and provides a progress indicator
during rendering to allow the user to knowwhat percentage of patches have been rendered.

## Test Scenes

Three different test scenes are included with the ray tracer that demonstrate its functionality. The
first is a basic Cornell box test scene with a reflective sphere and translucent cube. The second is
a fully-modelled chess set, as used in HW3, with reflective and translucent pieces. The third is a
basic demonstration of a sphere used as a refractive lens in front of a knight.

### Cornell Box

The Cornell box test scene demonstrates basic functionality. The differently-coloured walls make it
easy to see how the refractive cube is bending light, with the right side of the cube showing total
internal reflection. It also demonstrates that sphere normals are calculated correctly, as shown by
the reflective sphere.

The shadows below the cube and sphere also demonstrate how shadows are calculated. The presence of
ambient light, which is unaffected by occlusion, is clearly visible underneath the sphere. The
ability to have objects which only have partial shadows is demonstrated by the light shadow seen
under the cube.

A 512-by-512 pixel render of the Cornell box test scene can be found in `images/default.ppm`. The
scene itself is found in `scenes/cornell.scn`.

### Fully-Modelled Chess Set

The chess set scene demonstrates a lot of the more advanced functionality of the ray tracer. All
objects in this scene are made up of triangles loaded from OBJ files. The table, chairs, and chess
board use diffuse textures loaded from image files as their diffuse channels, however (unlike in
HW3) the chess pieces do not, as they use specialized materials. In particular, the chessboard shows
very well that bilinear interpolation is working correctly for textures; it has notably lower
resolution than the high-resolution render and clearly shows blurring between the spaces.

Looking at the lighting centered around the center of the board gives a clear indication that
attenuation of the light sources is working as expected. The shadows also clearly encircle the
center of the board, demonstrating clearly that their directionality is correct. The board itself
also has a small reflective component to demonstrate that reflection on flat surfaces works
correctly.

The white chess pieces have a diffuse component and a reflective component. By looking at the high-
resolution render of this scene and zooming in on the pieces, the reflections can be easily seen and
quite clearly demonstrate that barycentric interpolation of normals for triangles is working as
intended.

The black chess pieces have a reflective component and a refractive component, along with a very
small diffuse component to make them look more defined. They very clearly demonstrate the lensing
effects of refraction through curved surfaces. They also demonstrate very nicely that the three
different components (reflective, refractive, and diffuse) can be used together to create a very
aesthetically pleasing effect. Note that their shadows are also much lighter than the white pieces,
demonstrating partially opaque surfaces once again.

Take particular note of the chess pieces near the bottom of the image. The white pieces demonstrate
multiple shadows due to the fact that this scene actually uses two different point light sources to
get its lighting effect.

The complexity of this scene, with over 500,000 triangles in total, goes to demonstrate the sheer
efficiency of the ray tracer in performing its renders. Even at very high resolutions, the rendering
times for this scene are quite reasonable. As such, it acts as a good stress test of the ray tracer.

The 910-by-512 pixel image found in `images/yours.ppm` demonstrates this scene in very low quality.
This render is simply meant to comply with the assignment directions and to include the output of
the ray tracer as it was written to disk originally.

Two other renders can be found in `images/8k-chessboard.png` and `images/chessboard-far.png`. These
images were converted from the PPM output of the ray tracer into PNG images for space reasons. The
first demonstrates a very detailed render of the chessboard at a resolution of 8K (7680x4320) for
the purposes of being able to zoom in very closely to see the details in the scene that may not be
visible at lower resolutions; this render also uses 4x anti-aliasing (16 samples per pixel) to
achieve maximum fidelity. For reference, this image took approximately 15 minutes to render.

The second of these images features a 1080p render of the chess scene from the perspective of the
second camera defined in the scene to demonstrate that this functionality works correctly. It also
shows much more clearly the shadows underneath the table and chairs and the large-scale attenuation
of the light sources over long distances.

The scene itself can be found in the `scenes/chessboard.scn` file. The `default` camera was used for
the first two renders and the `far` camera was used for the last.

### Knight and Spheres

This scene is a very simple scene just meant to demonstrate more clearly that several features work
as intended. Namely, the sphere in front of the camera demonstrates a particular type of distortion
seen with refraction due to how the sphere acts like a lens. It also shows very clearly that ray
depth is working correctly. By adjusting the depth and looking at the reflection seen through the
refractive sphere, the effect of changing depth can be observed.

This scene also demonstrates that light colours can be changed and that multiple point light sources
act as expected when their light mixes. The two light sources in this scene emit red and blue light,
which can both be seen on the knight. Additionally, the reflective sphere is also partially diffuse
and the specular highlights from both lights can be clearly seen in it.

A basic render of the scene in 1080p can be found in `images/knight-and-spheres.png` and the scene
itself can be found in `scenes/knight.scn`.

## Scene File Format

The scene file format used in HW3 has been slightly altered to allow the new features that are
present in the ray tracer to be controlled. As before, each non-blank line specifies a command and
all attributes for a command should be indented by the same amount and all colour values range from
0 to 1.

The following commands and attributes are available:

- The `mdl <name> <obj file>` command loads the given OBJ file into a model with the given name
- The `mtl <name>` command defines a new material with the given name.
  - The `ambient <r> <g> <b>` attribute defines the ambient reflectivity
  - The `diffuse <r> <g> <b>` attribute defines the diffuse reflectivity
  - The `specular <r> <g> <b>` attribute defines the specular reflectivity
  - The `shininess <value>` attribute defines the shininess exponent
  - The `opacity <value>` attribute controls the opacity of the object for lighting purposes
  - The `diffuse_map <image>` attribute defines the diffuse reflectivity map
  - The `ao_map <image>` attribute defines the ambient occlusion map
  - The `reflectivity <value>` attribute defines the reflectivity of the material
  - The `transmittance <value>` attribute defines the transmittance of the material
  - The `refractive_index <value>` attribute defines the index of refraction of the material
- The `plight` attribute defines a new point light (max 16 per scene)
  - The `pos <x> <y> <z>` attribute defines the position of the light within the scene
  - The `ambient <r> <g> <b>` attribute defines the ambient light intensity/colour
  - The `diffuse <r> <g> <b>` attribute defines the diffuse light intensity/colour
  - The `specular <r> <g> <b>` attribute defines the specular light intensity/colour
  - The `atten <a0> <a1> <a2>` attribute defines the attenuation coefficients
      - Light intensity is multiplied by `1 / (a0 + a1 * d + a2 * d * d)` where `d` is the
        distance to the light
- The `obj mesh` command defines a new triangulated mesh object in the scene
  - The `mdl <model name>` attribute defines what model this object will use (required)
  - The `mtl <material name>` attribute defines what material this object will use
  - The `pos <x> <y> <z>` attribute defines the position of the object within the scene
  - The `rot <yaw> <pitch> <roll>` attribute defines the orientation of the object (rotations are
    performed about the object's origin and are specified in radians)
  - The `scale <scale factor>` attribute defines the object's scale factor (scaling is performed
    about the object's origin)
- The `obj sphere` command defines a new sphere object in the scene
   - The `mtl <material name>` attribute defines what material this object will use
   - The `pos <x> <y> <z>` attribute defines the center of the sphere
   - The `scale <scale factor>` attribute defines the radius of the sphere (the default sphere has
     a radius of 1)
- The `cam <name>` command defines a new camera (the camera with name `default` is used by default
  if no camera is specified on the command line)
   - The `pos <x> <y> <z>` attribute defines the location of the camera
   - The `lookat <x> <y> <z>` attribute defines the point to which the camera will look
   - The `up <x> <y> <z>` attribute defines the up vector of the camera
   - The `hfov <value>` attribute defines the horizontal FOV of the camera (in degrees)

## Resources Used

- [GLM Manual](http://glm.g-truc.net/glm.pdf)
- [GLM API Reference](https://glm.g-truc.net/0.9.4/api/index.html)
- [Ray Tracing Refraction](https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-to-shading/reflection-refraction-fresnel)
- [Möller-Trumbore Algorithm](https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm)
- [BVH Construction using Approximate Agglomerative Clustering](http://graphics.cs.cmu.edu/projects/aac/)
- Some code was loosely based on the skeleton code written by Karl Augsten
- Lots of code and scenes adapted from HW3
