# JoshEngine
[![lies](https://app.codacy.com/project/badge/Grade/d48b09d1f17e4d38bc40fd5b14bde807)](https://app.codacy.com/gh/josh-engine/josh-engine/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
![more lies](https://github.com/josh-engine/josh-engine/actions/workflows/ci.yml/badge.svg)

![Beautiful Image](https://cloud-8kzl199m5-hack-club-bot.vercel.app/0screenshot_2024-12-01_at_2.43.54___am.png)
> *if at first you do not succeed, try again in C++*
__________________________________________________________

JoshEngine is a *"high-performance"* programmatic __real-time__ game engine written in __C++__.
It can target both the __Vulkan__ and __WGPU__ graphics APIs for __widespread compatibility__, 
and has full web compilation support with Emscripten.

<sub><sup>side note on performance, it's actually relatively good. 
since it's relatively lower level than many other engines, 
there's less abstraction in the way of performance. 
i don't have any benchmarks at the moment but will post some
along with next (some) update.</sup></sub>

# Basic features
- Simplified input/output system with options for both event-driven programming and a more classical loop approach
- Audio functionality built atop OpenAL
  - Simple `Sound` objects for effects and more
  - Advanced `MusicTrack` objects for smoothly transitioning music
- Basic support for loading of .obj files
- Powerful platform-agnostic graphics support
  - Near-seamless transition between Vulkan and WGPU backends
  - Full support for custom shaders (GLSL)
- Verbose and utilitarian debugging GUI, made for debugging your shaders *and* your functions
- Ultra-simple recompilation for web with Emscripten
- Barebones built-in UI library for any of your UI needs

# When should I use JoshEngine?

If you want to try C++ in Game Development 
but don't want the overhead of something like Unreal,
JoshEngine might be perfect for you.

It's especially useful in projects that don't require
high graphical standards, but still need high graphical performance
that engines like Unity may not be able to provide.

I personally have found it incredibly useful in game jams,
where I can rapidly prototype and iterate upon my ideas
while also requiring myself to stop and think to write better code.
<sub><sup>although that might just be me because i wrote it...</sup></sub>

# Why did you make this?


> *TLDR: I wanted to learn graphics programming and my Java code sucked*

I started working on JoshEngine in the summer of 2023, 
fresh out of Minecraft modding and hence a minor brush up 
against real OpenGL code. The idea of graphics programming
intrigued me, so I started out with my first iteration of the engine,
in Java.

Unfortunately, that one didn't really work out, as my
inefficient code shone itself in the light of a not-so-fast language,
and I decided I needed to move on to something faster.

I discovered C and C++ while trying to make my own programming language
later that year and ended up going back to the game engine idea.
JoshEngine "2.0" was written in C++ and targeted Vulkan, although
it was _absurdly bad code_ and randomly `SEGFAULT`ed, causing me to give up.

Later in the school year I ran out of other project ideas and still needed
to procrastinate my schoolwork somehow. I worked on JoshEngine "3" for a week
or two in raw C and OpenGL, until I realized the standard library didn't have
vectors, in realizing which I promptly restarted in C++.

The rest after that is (commit) history! I started with OpenGL,
added Vulkan, had a Metal phase (the API not the music), axed
OpenGL and Metal in favor of Vulkan, and added WGPU. Heck of a ride.

# How do I use it?
I'll write a tutorial on basic engine functions if anyone actually
reads this and stars the repo. This is pretty much just my personal
project at this point but if anyone wants to mess around with it, 
*please do I need contributors and feel lonely*.
