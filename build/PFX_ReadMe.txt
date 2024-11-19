====================================================================
Particle Effects Plugin for RPG Maker 2003 ver 2.01
For DynRPG version 0.13 or higher
By Kazesui
====================================================================

This plugin allows you to create particle effects for your game.
These effects can be used on maps and in battles. The effects
have many customizable properties including fading time, amount
of particles, colours, generation function and much more.


Changes from earlier versions
------------------------------

A lot. This version has seen a complete overhaul.
- every command is now prefixed with "pfx"
- name tags are now used rather than numbers to identify an effect
- effects now come in two flavours, burst and streams.
- gravitational point and direction properties has been added
- a radial particle generation option has been added
- a secondary angle property has been added to go along with radial
  particle generation
- heart generation was removed ( or rather, not added back in ).
- having a particle image is no longer needed for default particles
- this new version has been optimized to handle a lot more particles
  than the previous version.
- a "does_effect_exist" command has been added, to ensure you can
  know an effect exists ( for load/saving ).

Installation
-------------

To install the plugin, make sure that you have patched your project 
with cherry's DynRPG patch which can be found here: 
http://cherrytree.at/dynrpg

Then go to the DynPlugins folder in this demonstration project and 
copy both the "KazeParticles.dll" to the DynPlugins folder in your own 
project.

Instructions
-------------

This plugin is initiated by using "comment commands" in your events.
You first start by creating an effect using the @pfx_create_effect
command, which will create an effect with a set of default properties.
From this point, you can use commands to customize the properties
of your effect, before you call it.

The effects are divided into two categories, bursts and streams.
A burst effect is a particle effect which will generate a single
burst of particles, and disappear right afterwards. Good for
explosions.
A stream effect, is one which will continue to generate particles
as long as it is active. Good for smoke, waterfalls and similar.
Both can almost imitate the behaviour of each other, but this
is not advisable as the effects are optimized for their own task.
A notable difference between the two effects is that with stream
effects, the first particle drawn will always be drawn on top of
older particles. For multiple bursts, you're never guaranteed
which particles will be drawn on top.

Notice that at the moment, the plugin does not differentiate between
maps and battles in terms of effects. This means that any effect
you have running on your map, might transition along to the the
battle screen.

Try to avoid changing the "amount", "timeout", "texture" and
"simultanuous effects" when particles of the effect is on
the screen, as this could lead to undefined behaviour
(i.e. it will probably crash the game).


Commands
-------------

@pfx_create_effect
(text) 	 parameter#1: name of the effect
(text) 	 parameter#2: effect type ( "burst" or "stream" )	 
       / Creates an effect with the specified name and type

@pfx_destroy_effect
(text) 	 parameter#1: name of the effect	 
       / Destroys the specified effect

@pfx_destroy_all
(none)
       / Destroys all effects

@pfx_does_effect_exist
(text)	 parameter#1: name of effect
(number) parameter#2: id of switch
       / Checks if effect exists, and turns designated switch on
         if it exists. Effects are erased when the game is quit,
	 so use this, to check if the effect exists before you
	 use it, in case the game has been loaded. If it doesn't
	 exist, simply recreate the effect.

@pfx_burst
(text)	 parameter#1: name of effect
(number) parameter#2: x coordinate
(number) parameter#3: y coordinate
       / Initiates a burst at designated coordinates. 
	 Only works with burst effects!

@pfx_start
(text)	 parameter#1: name of the effect
(text)	 parameter#2: name of the stream
(number) parameter#3: x coordinate
(number) parameter#4: y coordinate
       / Generates a stream and designated coordinates and gives
	 it a name. Only works with stream effects!

@pfx_stop
(text)	 parameter#1: name of the effect
(text)	 parameter#2: name of the stream
       / stops a selected stream of a the specified effect.
	 Only works for stream effects!

@pfx_stopall
(text)	 parameter#1: name of the effect
       / stops all streams of the selected effect from streaming.
	 Only works for stream effects!

@pfx_set_simul_effects
(text)	 parameter#1: name of the effect
(number) parameter#2: number of simultanuous effects possible
       / sets the amount of simultanuous effects possible to
	 prevent lag spikes ( Usually not neccessary when it's
	 less than 10 effects ).
	 default is 2 for bursts and 1 for streams.

@pfx_set_amount
(text)	 parameter#1: name of the effect
(number) parameter#2: number of particles
       / Sets amount of particles to be generated.
	 For a burst, this is all the particles generated for 
	 the burst, while for streams this is the amount of 
    	 particles to be generated per frame.
	 default is 50 for bursts and 10 for streams

@pfx_set_timeout
(text)	 parameter#1: name of the effect
(number) parameter#2: fading time ( 0 to 255 )
(number) parameter#3: color delay time ( 0 to 255 )
       / sets how long it will take for a particle to perish
	 in frames. Color delay time specifies how many frames
	 will be spent before the color starts fading.
	 255 is the maximum amount of frames, i.e. 4.25 seconds.
	 default is fading time of 30 and color delay of 0

@pfx_set_initial_color
(text)	 parameter#1: name of the effect
(number) parameter#2: red component ( 0 to 255 )
(number) parameter#3: green component ( 0 to 255 )
(number) parameter#4: blue component ( 0 to 255 )
       / sets the initial colour of the selected effect
	 default color is 255,255,255

@pfx_set_final_color
(text)	 parameter#1: name of the effect
(number) parameter#2: red component ( 0 to 255 )
(number) parameter#3: green component ( 0 to 255 )
(number) parameter#4: blue component ( 0 to 255 )
       / sets the final colour of the selected effect
	 default color is 255,255,255

@pfx_set_growth
(text)	 parameter#1: name of the effect
(number) parameter#2: initial size
(number) parameter#3: final size
       / sets size growth of the particles
	 default growth is 1,1

@pfx_set_generating_function
(text)	 parameter#1: name of the effect
(text)	 parameter#1: name of function ("standard" or "radial")
       / sets the initial colour of the selected effect.
	 "standard" effect generates particles at the
	 selected point, while "radial" generates the particles
	 in a circle around the selected point.
	 default function is "standard"

@pfx_set_position
(text)	 parameter#1: name of the effect
(text)	 parameter#2: name of the stream
(number) parameter#3: new x coordinate
(number) parameter#4: new y coordinate
       / allows you to change the position of an effect stream which
	 is alread running. Only works with Stream effects!

@pfx_set_random_position
(text)	 parameter#1: name of the effect
(number) parameter#2: random x offset
(number) parameter#3: random y offset
       / adds random offset to "standard" generating function
	 default values are 0,0

@pfx_set_radius
(text)	 parameter#1: name of the effect
(number) parameter#2: length of radius
       / sets the radius for generation of particles for
	 "radial" generation function.
	 default radius is 30

@pfx_set_random_radius
(text)	 parameter#1: name of the effect
(number) parameter#2: sets random radius offset
       / sets a random offset for the radius when using "radial"
	 particles generation.
	 default offset is 0

@pfx_set_texture
(text)	 parameter#1: name of the effect
(text)	 parameter#2: name of image file
       / sets the texture of the particles to one designated by
	 a selected image. Notice that this is significantly
	 less efficient than using no texture. Worth noting is that
	 if your picture is in the picture folder, you will have to
	 add "Picture/" in front of the filename(full path required)
	 NOTICE!
	 if you are using DynRPG 0.14 or older, you must add
	 a comma after the last parameter to ensure it working
	 properly!

@pfx_set_gravity_direction
(text)	 parameter#1: name of the effect
(number) parameter#2: gravity angle
(number) parameter#3: strength of gravity
       / sets a gravitational direction and strength affecting
	 all particles of the selected effect
	 not used by default

@pfx_set_acceleration_point
(text)	 parameter#1: name of the effect
(number) parameter#2: x coordinate
(number) parameter#3: y coordinate
(number) parameter#4: acceleration strength
       / sets a "acceleration point". All particles of the
	 selected effect will accelerate towards this point at
	 the given strength. This could be used to make
	 particles travel around in a circle if correct
	 values are chosen.
	 not used by default

@pfx_set_velocity
(text)	 parameter#1: name of the effect
(number) parameter#2: initial velocity
(number) parameter#3: random velocity offset
       / sets the velocity of the particles, as well as a
	 random velocity offset.
	 default values 30,30

@pfx_set_angle
(text)	 parameter#1: name of the effect
(number) parameter#2: angle
(number) parameter#3: width
       / for standard generation, it sets the velocity
	 direction of the particles, while with radial it
	 sets the generation point of the particles.
	 defaults to 0, 360 (full circle)

@pfx_set_secondary_angle
(text)	 parameter#1: name of the effect
(number) parameter#2: angle
       / when using radial generation, this will set the
	 velocity angle of the particles relative to
	 the primary angle.

@pfx_use_screen_relative
(text)	 parameter#1: name of the effect
(text)	 parameter#2: command ("true" or "false")
       / tells whether to use screen relative or
	 map relative pixel coordinates.
	 disabled by default

@pfx_unload_texture
(text)	 parameter#1: name of the effect
       / unloads the texture of the selected effect