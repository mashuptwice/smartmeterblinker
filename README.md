# smartmeterblinker
read out european smart meters with LED-blinks/KWh interface with just 3 components

### Background

I just got a Kaifa smart meter installed by the utilities provider. It has a serial interface to read out the power usage. As I am too lazy to fiddle around with this interface, I decided to use the impulse LED instead.
It blinks 1000 times for every KWh of energy. Doing some simple maths, it is easily possible to calculate the momentaneous power usage

#### Math
b = blinks

1000b/KWh = 1000b/1000Wh = 1b/1Wh
1000b/KWh = 16,666b/KWm = 0,2777b/KWs
