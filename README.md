## TinyGL for MinGW32 (c) 2022 Fabrice Bellard.

Prerequired
-----------
```
msys2
```

Build
------

```
MINGW64 ~/src$ cd TinyGL/src
MINGW64 ~/src/TinyGL/src$ mingw32-make -f build.mak

...

MINGW64 ~/src/TinyGL/src$ cp libTinyGL.a ../lib
MINGW64 ~/src/TinyGL/src$ cd ../examples
MINGW64 ~/src/TinyGL/examples$ mingw32-make

...

```



Test
-----
```
MINGW64 ~/src/TinyGL/examples$ ./gears.exe
```


+ <br><img src="http://www.atmarkartworks.com/TinyGL/screenshot02.jpg" width="50%">




---

# TinyGL : a Small, Free and Fast Subset of OpenGL*

News
----

* (Mar 5 2022) TinyGL 0.4.1 is out ([Changelog](https://bellard.org/TinyGL/changelog.html))
* (Mar 17 2002) TinyGL 0.4 is out ([Changelog](https://bellard.org/TinyGL/changelog.html))

Download
--------
Get it: TinyGL-0.4.1.tar.gz

Introduction
------------
TinyGL is intended to be a very small implementation of a subset of [OpenGL](http://www.opengl.org)\* for embedded systems or games. It is a software only implementation.
Only the main OpenGL calls are implemented.

The main strength of TinyGL is that it is fast and simple because
it does not have to be completely compatible with OpenGL. In
particular, the texture mapping and the geometrical transformations
are very fast. TinyGL is a lot faster than Mesa or the software
Solaris OpenWin OpenGL implementation for
the [VReng Virtual
Reality engine](https://github.com/philippedax/vreng)for example.

The main features of TinyGL are:

* Header compatible with OpenGL (the headers are adapted from the very good
    [Mesa ](https://www.mesa3d.org/)by Brian
    Paul et al.)
* Zlib-like licence for easy integration in commercial designs (read the
    LICENCE file).
* Subset of GLX for easy testing with X Window.
* GLX like API (NGLX) to use it with [NanoX](http://www.microwindows.org).
* Subset of BGLView under BeOS (thank to [Peder Blekken](mailto:pederb@sim.no)). 
* OpenGL like lightening.
* Limited support of OpenGL&nbsp;1.1 arrays.
* Complete OpenGL selection mode handling for object picking.
* 16 bit Z buffer. 16 bit RGB display. High speed dithering to paletted 8
    bits if needed. High speed convertion to 24 or 32&nbsp;bits.
* Fast Gouraud shadding optimized for 16 bit RGB.
* Fast texture mapping capabilities, with perspective correction and texture
    objects.
* 32 bit float only arithmetic.
* Very small: compiled code size of about 40 kB on x86.
* C sources for GCC on 32/64 bit architectures. It has been tested succesfully
    on x86-Linux and sparc-Solaris.

Architecture
------------
TinyGL is made up four main modules:

* Mathematical routines (zmath).
* OpenGL-like emulation (zgl).
* Z buffer and rasterisation (zbuffer).
* GLX interface (zglx).
To use TinyGL in an embedded system, you should look at the GLX layer and
modify it to suit your need. Adding a more user friendly developper layer
(as in Mesa) may be useful.

Why ?
-----
TinyGL was developped as a student project for a Virtual Reality network
system called VReng (see the [VReng project](https://github.com/philippedax/vreng)).

At that time (January 1997), my initial project was to write my own
3D rasterizer based on some old sources I wrote. But I realized that it
would be better to use OpenGL to work on any platform. My problem was that
I wanted to use texture mapping which was (and is still) quite slower on
many software OpenGL implementation. I could have modified Mesa to suit
my needs, but I really wanted to use my old sources for that project.

I finally decided to use the same syntax as OpenGL but with my own libraries,
thinking that later it could ease the porting of VReng to OpenGL.

Now VReng is at last compatible with OpenGL, and I managed to patch
TinyGL so that VReng can still work with it without any modifications.

Since TinyGL may be useful for some people, especially in the world
of embedded designs, I decided to release it 'as is', otherwise, it would
have been lost on my hard disk !



###### \* OpenGL(R) is a registered trademark of Silicon Graphics, Inc.&nbsp;

---

 - [Fabrice Bellard ](https://bellard.org/)

##### 




