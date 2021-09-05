
Introduction
============

One weekend I decided to see if it was possible to salvage parts from
broken LED lights for use in other projects.  Since these lights are
composed of numerous small LEDs, I wondered if they could be used in
lower-voltage projects.


E14 E27 3528 Bulb
=================

This bulb was obtained cheaply off Ebay but it didn't last long.  It seems to
be this single LED that failed open-circuit thereby taking the whole thing out.

![](img/3w_1.jpg)

It was easy to separate the two parts and get the LED section out, just pulling
with moderate pressure did the trick.  It was joined to the base with two 
wires which I cut.  A simple bridge rectifier and smoothing capacitor,
no constant-current circuit that I could make out.

![](img/3w_2.jpg)

I used my wire-strippers as a clamp (I really 
shouldn't, but they're always to hand)

![](img/3w_3.jpg)

The LED section consists of 6 PCBs joined in a cylinder and connected to two 
circular PCBs at each end.  The upper PCB sports 6 more LEDs, the lower one 
has the electronics

![](img/3w_4.jpg)

I really liked that I didn't need to resort to hot air and yanking to 
de-solder these PCBs they came apart cleanly with nothing more than braid

![](img/3w_5.jpg)

And each section could be used independently as 4-LED array, just perfect
for low voltage projects.

![](img/3w_6.jpg)

Connected up these gave good brightness at around 10v across the four.  Each 
side-PCB is 4 LEDs series-connected.  The pads at each end were easy to connect
solid-core wire to.  It was clear the LEDs contained their own series resistors.
I think they must have done it this way so they don't need any complex current 
generator circuits and they can spread the power dissipation across all the LEDs
instead of having some meaty resistors inside the unit which would heat up.

![](img/3w_7.jpg)

So that's my haul for the parts bin.  But since I wasn't sure if I wanted to use 
10v for my projects (I'd need two PSU voltages, 5v for any MCU and 10V for the lamps)
I checked to see if I could de-solder LEDs and use them individually at lower 
voltages.

![](img/3w_8.jpg)

Getting an LED off was easy with two soldering irons, I just used them as hot
tweezers, although I melted the plastic sides of the LED a little in doing
so.  These are clearly not meant to be soldered with an iron because it's
hard to get the iron on the contacts without touching the plastic.  I have a hot 
air rework station, but it's not very good with temperature regulation and has a 
habit of destroying components (at least when I'm using it) so I didn't use it here.

In any case, once the SMD LED was off, I prepared some stripboard, to connect it.
A couple of blobs of solder were going to be my 'paste' I decided to try to melt
the solder from the side with the LED on top and hope it settled in the right place.

![](img/3w_9.jpg)

I thought a flux pen would be desirable for this, so I put some on top of the two
blobs of solder on the board.

![](img/3w_10.jpg)

At this point, bearing in mind the LED casing was plastic and seemingly very easy
to melt with my iron, my plan was to rest the LED at an angle, and then melt the 
solder from underneath it.  Hopefully I'd melt both blobs in quick succession so 
it settled into a sensible position.

![](img/3w_11.jpg)

And it turned out that it did, indeed settle, and the operation went a lot better 
than I expected.  Some would have purchased solder paste, and/or adapter boards, 
but I always found that stuff expensive, so I've tried to manage without.

![](img/3w_12.jpg)

So with the board cut smaller and some headers attached I could now bread-board 
the LED

![](img/3w_13.jpg)

It even switched on when I ran my continuity tester across it.

![](img/3w_14.jpg)

And it gave reasonable brightness with the expected voltage.

![](img/3w_15.jpg)

Philips 11w Bayonet
===================

A lot more posh, more powerful and expensive, the Philips bulbs in my home have
tended to last a lot longer than any of the cheap stuff, but this unit failed somewhat
earlier than the stated life suggested all the same.

![](img/11w_1.jpg)

But re-using any of this tech is a lot more of a challenge than it was for
the cheap E14 bulb.  The first problem was how to get into it.  The diffuser didn't
unscrew despite my best efforts.  I started hack-sawing off the bayonet.  That didn't
help in the slightest.  So when I had almost given up I had a hunch.  I got out a brick
bolster (that's used for cutting bricks in half), and a lump hammer.  I gave one
sharp blow and the thing came apart.  In fact, it came apart so cleanly and with so
little damage I honestly don't think there's a better way than this.

![](img/11w_2.jpg)

You can see inside the rubber gunk that was presumably holding that plastic diffuser
so firmly in position.  I was immediately a bit disappointed because I could see that
there were only 11 LEDs and the voltage drop across each was going to be fairly high
so not of much use for my project.  Since I'd gone this far I decided to still try to
see if I could open the thing further out of curiosity.   Next step was to try to 
open the lower part.

![](img/11w_3.jpg)

Again, this was really difficult.  I could see that I might be able to pull out the 
metal heatsink below the LED PCB but it wouldn't budge.  I widened one of the holes
to get some flat bladed tools through to try to rotate it but it wasn't moving
So I decided to just forget about the electronics inside and get the LEDs.

Removing the 'PCB' was fairly easy.  There was some sticky, presumably heat-conductive
gunk and cutting through that it slowly lifted off.

![](img/11w_4.jpg)

The 'PCB' wasn't actually a PCB at all in the conventional sense.  It seemed to be a
very thin-film PCB element, laid atop a steel plate.  My attempts to de-solder the LEDs
from the PCB went nowhere, but it wasn't surprising because the heat was 
probably conducted away by the steel backing plate.  

![](img/11w_5.jpg)

Just while I'm here, I sawed open the rest of the bulb to pull out the electronics
(it was the only way) and discovered what looked to be a 'proper' constant current
generator circuit

![](img/11w_6.jpg)

...with some kind of supervisor chip along with the bridge.  So with a constant current
generator perhaps the voltage across individual LEDs wouldn't need to be so high 
after all.

![](img/11w_7.jpg)

But next off I removed all the gunk from the LED board, in preparation for using hot
air.

![](img/11w_8.jpg)

At first I tried just heating the board from the back, and lifting components off.
This wasn't very satisfactory, and I damaged some LEDs like this.

![](img/11w_9.jpg)

Then I had another idea.  Since I had a powerful heat gun I'd use it to literally 
blow the components off the board, that way they wouldn't hang around any longer than
they needed to, as soon as the solder was melted they'd be blown out of the heat path.

I cleared a wide area of space, so I could spot the components later, blasted the heat
at the board, and didn't worry too much about where they were going.

![](img/11w_10.jpg)

I collected them all up.  The melted ones were damaged in the earlier attempt. 
My approach with blasting them off the board was highly effective and
resulted in minimal damage.  You just need a decent space to work in.

![](img/11w_11.jpg)

Next I did the same as I'd done for the E14 lamp LEDs.  Flux pen, a couple of solder 
blobs, 

![](img/11w_12.jpg)

I think I tried to solder this more from the sides because it was a different shape
and slightly larger.  It wasn't quite so easy and the pads weren't even sizes, one
was about double the size of the other.

![](img/11w_14.jpg)

And it also worked!

![](img/11w_13.jpg)

Current/Brightness Testing
==========================

Largely subjective, I don't really know what the spec of these LEDs is but experience 
tells me most LEDs have absolute maximum rating somewhere around 50-60mA, and I may be 
looking to drive them around half of that based on the brightness.

It's quite hard to see where optimum brightness of the LEDs lies and to compare with the
brightness on mains power.  Maybe (as with the cheaper bulb) they were not driven at the 
right point for best longevity anyway.  So I just watched how the brightness and current
consumption varied as I took the voltage up at 0.1v increments (my PSU has a button to 
increase voltage by 0.1v).

![](img/test_1.jpg)

So taking the first smaller LED from the cheaper, lower wattage lamp up to 'reasonable'
brightness (remember, highly subjective), I ended up at around 5v.

![](img/test_2.jpg)

Around 38mA was reached at 5v.  This is OK, but I'd probably need to power such an arrangement
slightly over 5v, if I was driving the LED from an open-collector my PSU would realistically
need to be about 6v.  This is a bit of a nuisance for projects with e.g. a Pi, or 5v Arduino.
Alternatively I could use e.g.
[MIC2545A](https://www.microchip.com/en-us/product/MIC2545A) (a personal favourite of 
mine) to drive the LED at (almost) the full supply voltage, but it does seem overkill.

![](img/test_3.jpg)

Next up I tried the Philips LED.  This produced a lot more brightness for not much more voltage
and about the same current.

![](img/test_6.jpg)

In fact, I quickly covered it with the diffuser to save my eyes.  The LED produced a lot of brightness,
but please bear in mind I was using no heat-sink, didn't run it for long, and I'm unsure if 
the couple of blobs of solder on the strip-board would be enough to cool it adequately.  This
is something I may look into in the future.
It's a bit disappointing that the sweet-spot for this LED isn't 5v, however an Atmega 328p
should theoretically be OK up to 5.5v.  You could always get a slightly higher voltage to drive
the LED with a 
[switching regulator](https://hackaday.com/2021/01/09/avr-microcontroller-doubles-up-as-a-switching-regulator/),
and I think the precise output voltage wouldn't be critical so long as you're averaging out at 
something within spec for the LED.

![](img/test_4.jpg)


Summary
=======

I hope this readme is useful and helps you recycle old bulbs for your 
projects.  It seems depending on the bulb, you may have more or less effort to take it apart.
The internal current-limiting resistors in these parts do make them slightly more 
challenging - but not impossible - for home project use at low voltage.
Also, if you are 100% committed to saving the environment or something, then bear in mind that
all the compact bulbs are a dead loss in terms of ECO credentials.  It's far better to
go for a bulkhead light that directs all the light down, and has a proper constant current 
circuit which you can easily open up and repair.  These lights consume tiny amounts of power
in comparison to the bulb format because all the light is sent in one direction and the 
diffuser spreads it around the room.  They are, of course, not very nice to look at, and I'd 
only put one in a kitchen/bathroom/workshop but why not go for that option when you can?

Below is the internal layout of a Meridian 12w LED bulkhead with constant current driver.
If any LEDs blow, you can just short them out with a piece of wire and be working again.  No
need to dispose of the whole unit.  The light output of this 12w unit is uncomfortably high,
and I'll be looking into adjusting it lower by modifying the circuit board but that will be
another project!

![](img/bulkhead.jpg)


