
# 3D Object Manipulation and Shaders with OpenGL

This project is a 3D object manipulation application using OpenGL, allowing transformations such as translation, scaling, shearing, rotation, and reflection. The application supports different shading techniques including Toon, Gooch, and Phong shading.

## Features

- **File Parsing**: Loads 3D models from `.off` files.
- **Vertex and Face Normal Calculation**: Computes face normals for accurate lighting.
- **Shader Support**: Implements Toon, Gooch, and Phong shading techniques.
- **Transformations**:
  - **Translation**: Move the object in 3D space.
  - **Scaling**: Resize the object along the X, Y, and Z axes.
  - **Shearing**: Apply shearing transformations in X, Y, and Z directions.
  - **Rotation**: Rotate the object around an arbitrary axis.
  - **Reflection**: Reflect the object over an arbitrary plane.
- **Interactive Controls**: Use keyboard inputs to apply transformations.

## Requirements

- **OpenGL**: Ensure you have a compatible version of OpenGL installed.
- **GLEW**: OpenGL Extension Wrangler Library.
- **GLUT**: OpenGL Utility Toolkit for creating and managing windows.
- **C++ Compiler**: Any standard C++ compiler that supports C++11 or later.

## Installation



## Usage

1. **Load a 3D Model**: The application will automatically load a model from `off/6.off`. Make sure this file exists or update the path in the code.
2. **Apply Transformations**:
   - Press `t` to translate the model.
   - Press `s` to scale the model.
   - Press `q`, `w`, or `e` to shear the model in X, Y, or Z directions, respectively.
   - Press `r` to rotate the model around an arbitrary axis.
   - Press `u` to reflect the model over an arbitrary plane.
   - Press `a` to undo the last transformation.

3. **Choose Shading**: At runtime, choose a shading technique (1 for Toon, 2 for Gooch, 3 for Phong) when prompted.

## Shader Files

- **Vertex Shaders**:
  - `vertexShader`: Used for Toon and Phong shading.
  - `vertexShader2`: Used for Gooch shading.

- **Fragment Shaders**:
  - `fragmentShader`: Used for Toon shading.
  - `fragmentShader2`: Used for Gooch shading.
  - `fragmentShader3`: Used for Phong shading.

## Acknowledgments

- OpenGL for graphics rendering.
- GLEW for managing OpenGL extensions.
- GLUT for window management and user input.
