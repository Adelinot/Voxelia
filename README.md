VOXELIA

A Minecraft-inspired 3D voxel sandbox built using C++ and OpenGL.

VOXELIA is a procedural voxel game where players can explore a generated block-based world, break blocks, and place blocks.

Features

- Procedurally generated voxel terrain.
- 3D rendering using OpenGL.
- First-person player movement.
- Block breaking and placing.
- Block selection using the number keys (1–8).
- Basic hotbar and crosshair HUD.

Technologies Used

- C++
- OpenGL
- GLFW
- GLEW
- GLM
- stb_image
- JSON

Controls
W- Move forward
A- Move left
S- Move backward
D- Move right
Space- Jump
Mouse- Look around
Left Mouse Button- Break blocks
Right Mouse Button- Place blocks
1–8- Select blocks

VOXELIA is build for Windows using C++ and MSYS2 UCRT64.

The project requires a C++17 compiler and the necessary libraries (GLFW, GLEW, and GLM).

Compile from the project root using:

g++ -std=c++17 src/main.cpp src/texture.cpp src/blocks.cpp src/terrain.cpp src/player.cpp src/interaction.cpp src/hud.cpp -o VOXELIA.exe -I/ucrt64/include -L/ucrt64/lib -lglfw3 -lglew32 -lopengl32 -lgdi32

Run:

./VOXELIA.exe

Project Status

VOXELIA is an ongoing game development project. More features and improvements may be added in future updates.
