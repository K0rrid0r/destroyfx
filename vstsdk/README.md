This directory contains the VST 2 SDK, but it's not
part of the repository because the SDK is not free
software.


Here's what the (relevant) contents looked like in
the final version 2.4 rev2 in 2006:

```
$ ls -lR vstsdk
total 0
drwxr-xr-x  4 user  group  128 Jul 26  2009 pluginterfaces/
drwxr-xr-x  5 user  group  160 Jul 26  2009 public.sdk/

vstsdk/pluginterfaces:
total 0
drwxr-xr-x  6 user  group  192 Aug 25  2009 vst2.x/

vstsdk/pluginterfaces/vst2.x:
total 176
-rw-r--r--  1 user  group  16961 Jun 20  2006 aeffect.h
-rw-r--r--  1 user  group  64725 Jun 20  2006 aeffectx.h
-rw-r--r--  1 user  group   4044 Feb 10  2006 vstfxstore.h

vstsdk/public.sdk:
total 0
drwxr-xr-x  4 user  group  128 Jul 26  2009 samples/
drwxr-xr-x  4 user  group  128 Jul 26  2009 source/

vstsdk/public.sdk/samples:
total 0
drwxr-xr-x  4 user  group  128 Jul 26  2009 vst2.x/

vstsdk/public.sdk/samples/vst2.x:
total 0
drwxr-xr-x  4 user  group  128 Jul 26  2009 win/

vstsdk/public.sdk/samples/vst2.x/win:
total 8
-rw-r--r--  1 user  group  44 Nov 17  2005 vstplug.def

vstsdk/public.sdk/source:
total 0
drwxr-xr-x  8 user  group  256 Nov 13  2006 vst2.x/

vstsdk/public.sdk/source/vst2.x:
total 248
-rw-r--r--  1 user  group   3115 Jan 16  2006 aeffeditor.h
-rw-r--r--  1 user  group  24787 Jun  7  2006 audioeffect.cpp
-rw-r--r--  1 user  group  11541 Jun  7  2006 audioeffect.h
-rw-r--r--  1 user  group  56569 Oct  5  2006 audioeffectx.cpp
-rw-r--r--  1 user  group  17371 Jun 20  2006 audioeffectx.h
-rw-r--r--  1 user  group   2275 Sep  5  2006 vstplugmain.cpp
```


VST2 is definitely deprecated and it's harder and harder to
find these files. You can maybe find them in the VST3 SDK,
which for some time contained the VST2 headers as well.
