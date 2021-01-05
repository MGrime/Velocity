@echo off

%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/triangle.vert -o Velocity/assets/shaders/vert.spv

%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/triangle.frag -o Velocity/assets/shaders/frag.spv

pause