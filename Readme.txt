Jonathan Bourim
CS350 Assignment 4
--------------------

a. How to use parts of your user interface that is NOT specified in the assignment
description.

Everything that should be necessary for the program to run should be included in the directory.
The CMakeLists.txt builds DemoScene.exe, which is the current assignment's scene.

The project is built in Visual Studio 2019 using their native CMake support.
Project is opened by opening the root folder in VS, which should start up automatically by 
targeting the root CMakeLists.txt and the added subdirectories with their .txt.
The CMakeSettings.json file in the root describes a couple of configurations, and 
I use all three (x64 debug, release w/ debug info, minimal release). 
The only build target for these configurations should be DemoScene.exe.

To create an Octree, navigate to the corresponding drop-down
on the top-left ImGui panel. The Create/Destroy button will be responsible for generation.
The Octree is centered at origin with a half-extent of 10, these parameters are exposed in the UI.

In order to navigate the scene, hit F to lock/unlock the cursor and use WASD to fly around.

The floating cubes throughout the scene are from the previous assignment, where 
you can toggle copying of their depth for the deferred -> forward rendering transition.

In order to load the power plant, place all the power plant files under Assets/Models,
the user interface in the scene (go to Model Loader) should take care of the rest.
Note: It's about as fast to load all sections of the power plant as it is to load only one of them,
this is because 1 section is single-threaded, while loading all of them is multi-threaded.

It's worth noting about model loading that the CMake will execute a post-build event
to copy everything in your Assets folder to its output directory.
This will take a minute for something like the power plant, as its duplicating gigabytes of data.
If you want to bypass this process, you can comment the CMakeLists.txt in the root
and manually copy those files to their respective build folders.

Press SPACE to fire a sphere collider into the scene, which will latch itself onto the
bounding volume of an object that is a part of the Octree. If it is not a part of the
tree, it WILL NOT collide. 


Versions:
C++17
Vulkan SDK 1.2.162.0

b. Any assumption that you make on how to use the application that, if violated, might
cause the application to fail.

Setting number of triangles too low on the Octree may cause either
insane load times or outright stack overflow, particularly if the octree is large.

If the power plant models are not in the correct folder, the validation
will not currently output information about it. It just wont load.
(I don't know why this is the case, because TinyObj should return model loading success/failure)


c. Which part of the assignment has been completed?

Full GJK algorithm functionality.

d. Which part of the assignment has NOT been completed (not done, not working,
etc.) and explanation on why those parts are not completed?

The step by step animation is not functional, but the sphere is shown in red,
and will turn any object struck by its collider red as well.

e. Where the relevant source codes (both C++ and shaders, as applicable) for the
assignment are located. Specify the file path (folder name), file name, function
name (or line number).

The code can be found at the following:

Scene Setup - Application/Scenes/DemoScene.cpp
GJK & Octree - Application/Octree/Octree.hpp
Mesh Splitting Tools - Framework/InternalStructures/Mesh.h
Primitive Collision - Framework/Primitives/Primitives.h

f. Which machine did you test your application on.

Personal desktop computer running Windows 10.

g. The number of hours you spent on the assignment, on a weekly basis

8 hours per week.
