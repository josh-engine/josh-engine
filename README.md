# JoshEngine
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/d48b09d1f17e4d38bc40fd5b14bde807)](https://app.codacy.com/gh/josh-engine/josh-engine/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
![CI Status](https://github.com/josh-engine/josh-engine/actions/workflows/ci.yml/badge.svg)

> *Code it for the meme, end up using it somehow.*

Hello ladies, gents, and fellows anywhere inbetween!
Welcome to the amazing and wonderful home of JoshCo's *fourth attempt* at a usable game engine!

Alright, I'll cut the theatrics now.

This is the work we have to get done:

- [X] ~~OpenGL working~~ OpenGL has been removed in favor of Vulkan and other modern APIs. ~~If you have old hardware, use Zink.~~ Oops! I'm an idiot. Zink goes the *other* way. If you have old hardware, good luck! :D
- [X] GameObjects working
- [X] OBJ models and removing duplicate vertices
- [X] OpenAL sound working
- [ ] ~~ChaiScript~~ Scripting support with at least *some* language (any kind of hot reloadable code)
- [ ] C++ PYSON implementation because I said I'd do it for Liam so he would *merge the damn PR*

---------------------------------------------------------------------------------------------------------------
Once everything above this line is done, the engine is ready for a beta!

---------------------------------------------------------------------------------------------------------------

- [ ] Python support
- [ ] Lua support
- [X] Deeply refactor graphics backend to be a little more fluid and support multiple APIs
- [ ] OpenGL ES 2.0 or 3.0 for Emscripten+WebGL
- [X] Vulkan
- [ ] DX12
- [ ] Skinned animation support
