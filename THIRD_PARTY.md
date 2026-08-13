# Third-party dependencies

Jevaing itself remains MIT licensed.

For external 3D model import, development builds may use **Open Asset Import Library (Assimp) 6.0.5**, licensed under the BSD 3-Clause license. The dependency is kept behind Jevaing's model-import boundary and is not part of the public rendering API.

CMake uses an installed Assimp package when available and otherwise fetches the pinned upstream source release. Refer to the Assimp source distribution for its complete license text and notices.

For PNG/JPG texture decoding, development builds may use **stb_image** from the public-domain/MIT licensed stb project, pinned to commit `2c980bb59875b0d32144a71867fbdebb2f77cd20`. The decoder is kept behind Jevaing's texture-loader boundary and is not part of the public rendering API.
