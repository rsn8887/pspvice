Vice-PSP
=======

C64 emulator for PSP, based on Akop's and DelayedQuasar's work.

I am updating this emulator with additional options.

![](https://i.postimg.cc/Bvj57k0K/2019-02-27-004153.jpg)
![](https://i.postimg.cc/j5NHtfCq/2019-02-27-004130.jpg)
![](https://i.postimg.cc/XNsJXJFn/2019-02-27-004834.jpg)

The new sound chip option is useful for modern games which were designed for the newer 8580 chips. Two such games are Caren and the Tangled Tentacles and Sam's Journey. Setting sound chip to 8580 and sound engine to resid gives those games a more authentic sound.

The new palette option is useful to achieve more saturated, darker colors. My personal favorite is `pepto-pal`, but there's also `colodore` which gives higher saturation. Setting this to `none` gives the terrible, neon-colored Vice 3.2 default palette.



Original website by Akop (v2.2.15):
https://psp.akop.org/vice.htm

Thread about DelayedQuasar's work on this emulator (v3.2.3):
http://wololo.net/talk/viewtopic.php?f=47&t=49620

Thanks
======

Thanks to the Vice team for an excellent emulator.

Thanks to Akop for the amazing original PSP port.

Thanks to DelayedQuasar for updating Akop's port to Vice 3.2.

Thanks to my supporters on Patreon: Andyways, CountDuckula, Greg Gibson, Jesse Harlin, Duncan Harris, Özgür Karter, Matthew Machnee, Mored1984, Ibrahim Fazel Poor, and RadicalR.

Changelog:
========
3.2.4

- add SID model option with two choices: 6581, and 8580.
- add palette option with three choices: none (vice palette), pepto-pal, and colodore.
