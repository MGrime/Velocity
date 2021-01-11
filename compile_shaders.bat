@echo off

%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/standard.vert -o Velocity/assets/shaders/standardvert.spv
%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/standard.frag -o Velocity/assets/shaders/standardfrag.spv

%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/skybox.vert -o Velocity/assets/shaders/skyboxvert.spv
%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/skybox.frag -o Velocity/assets/shaders/skyboxfrag.spv

%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/pbr.vert -o Velocity/assets/shaders/pbrvert.spv
%VK_SDK_PATH%/bin32/glslc.exe Velocity/assets/shaders/pbr.frag -o Velocity/assets/shaders/pbrfrag.spv

pause