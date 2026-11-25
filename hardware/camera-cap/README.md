# Camera cap (OV5640 on the eyepiece)

Parametric OpenSCAD model: `camera_cap.scad`. Design and reasoning: `plans/camera-cap/` (archived
after release). Two printed parts plus hardware:

- **cap**: one per eyepiece. A sleeve slides over the eyepiece housing and is centred and held by
  three radial M3 thumbscrews at 120 degrees. A closed tube leads from the eyepiece rim up to a
  socket for the camera head.
- **head**: one, moves between caps. A puck that friction-fits into the socket (plus one M3 set
  screw), holds the OV5640 module lens down on the axis, has an FPC slot, and a platform for the
  ESP32-S3 CAM board with zip-tie slots.

![assembly](preview.png)
![cross-section](section.png)

Assumptions to check: defaults come from the design (Bresser 76/700 kit, 25 / 12.5 / 8 mm
eyepieces, always with the 3x Barlow). No published eye relief exists for these eyepieces, so the
eye relief values are estimates; measure them.

## Parts you need

- 3x M3 x 16 thumbscrews per cap (knurled head), 3x M3 nut per cap
- 1x M3 x 10 grub or cap screw and 1x M3 nut for the socket set screw (one per cap)
- 2x M2 x 4 grub screws for the OV5640 module (or leave `retain = false` and use a dab of hot glue)
- zip ties 2 mm thick x 4 mm wide, black tape or flocking paper, PETG filament

## What to measure

Use calipers. Measure each eyepiece separately and edit its row in `eyepiece_table`
(`[eyepiece mm, housing OD, eye relief with Barlow, rim recess]`), or override on the command
line, for example `-D housing_od=32.6`.

1. **housing_od**: outer diameter of the eyepiece housing over the top 15 mm (where the sleeve
   grips). Measure at three places and use the largest. Default 33.
2. **rim_id**: inner diameter of the opening at the top rim. The camera nose must fit through it
   when it goes below the rim. Default 20.
3. **lens_recess**: how far the eye lens sits below the rim. Lay a straight edge across the rim
   and measure down to the glass with the depth rod of the calipers. Default 3.
4. **eye_relief** (eye lens to exit pupil) **with the Barlow in place**, by the paper method:
   1. Put the Barlow and the eyepiece in the focuser as you normally use them.
   2. Point the telescope at a bright, distant, daylight scene (a window with sky, a bright wall
      far away, an indoor lamp). **Never point it at the Sun, not even briefly: the telescope
      focuses sunlight into a burn spot and a blinding beam.** Cover the Sun's direction and keep
      the telescope far from it.
   3. Hold a sheet of white paper behind the eyepiece and move it away from the eye lens until the
      bright disc on the paper is smallest and sharpest. That disc is the exit pupil.
   4. Measure from the eye lens glass (not the rim) to the paper. That is `eye_relief`. Repeat
      three times and take the mean.
5. **OV5640 module**: sensor PCB width and thickness, lens holder width, height from lens front to
   PCB front face (`module_h`), FPC width. Edit the "OV5640" block at the top of the file.
6. **ESP32-S3 CAM board**: `esp_w` x `esp_l`.

## The gap formula

`gap` is the distance from the eyepiece rim to the front of the camera lens at nominal head height:

    gap = eye_relief - lens_recess - lens_stop_offset

`lens_stop_offset` (default 1.5) is how far the camera's aperture stop sits behind the front glass.
Defaults with the Barlow:

| eyepiece | eye_relief | lens_recess | lens_stop_offset | gap |
|----------|-----------:|------------:|-----------------:|----:|
| 25 mm    | 18         | 3           | 1.5              | 13.5 |
| 12.5 mm  | 9          | 3           | 1.5              | 4.5 |
| 8 mm     | 5          | 3           | 1.5              | 0.5 |

If `gap` is positive the lens is above the rim. If it is negative (or the lowest head position
goes below the rim) the nose enters the eyepiece opening, so `nose_d` must be at least 1 mm
smaller than `rim_id`; the model asserts this. The socket floor is placed automatically so the head
travels `z_adjust` (default 4 mm, centred on the nominal gap). If a combination is impossible
(nose does not fit, nose would touch the eye lens, socket floor too close to the rim) OpenSCAD stops
with an `assert` message that names the parameter to change; a deeper reach (large negative gap)
needs a longer `nose_len` and `lens_in = 0`.

## Export STLs

With `openscad` (Windows: `"C:/Program Files/OpenSCAD/openscad.com"`) in this folder. STLs are
build output and are not committed. `print_pose=true` puts the head in print orientation.

    openscad -o cap_25.stl   -D 'part="cap"' -D eyepiece=25   camera_cap.scad
    openscad -o cap_12.5.stl -D 'part="cap"' -D eyepiece=12.5 camera_cap.scad
    openscad -o cap_8.stl    -D 'part="cap"' -D eyepiece=8    camera_cap.scad
    openscad -o head.stl     -D 'part="head"' -D print_pose=true camera_cap.scad

Pictures: `-D 'part="assembly"'` (add `-D section=true` and look from -y for the cross-section).

## Print settings

- PETG, 0.2 mm layers, 0.4 mm nozzle, 4 perimeters (walls are 2 mm), 30 % infill, no supports.
- Black or dark PETG (matt if possible): the inside of the tube must not reflect.
- **cap**: print as exported, open sleeve end on the bed, axis vertical. The underside of the rim
  seat is a 7 mm bridge (slow it down, fan on). The cone and the set-screw boss have 45 degree
  undersides. Nut slots are open at the top: drop the nuts in with the print upright.
- **head**: print with `print_pose=true`: platform flat on the bed, nose pointing up. The module
  pocket then opens upward, so nothing needs supports. Print one test fit of the puck in a cap
  first and adjust `fit` (smaller = tighter) so it slides with friction.
- Ream the 3.4 mm screw holes if the first layers squish them; tap the M2 holes by threading the
  grub screws in.

## Assembly

1. Press the three M3 nuts into the slots at the top of the bosses (slide them down the open
   slots), add the set-screw nut in the socket boss the same way.
2. Line the inside of the tube and socket floor with black flocking paper or matt black paint
   if the print is shiny.
3. Fit the OV5640 module into the head pocket from the nose end, lens facing out, FPC up through
   the slot and across the platform groove. Pinch it with the two M2 grub screws. Seal gaps around
   the module with black tape.
4. Fit the cap on the eyepiece (eyepiece in the focuser, Barlow in place) until the rim touches the
   shoulder inside. Run the three thumbscrews in until they just touch the housing.
5. Push the head into the socket, connect the camera, and open the live view.
6. **Centre**: with the live view on a distant target, tighten the three thumbscrews a little at a
   time, one then the next, until the circular field is centred in the image (the circle of the
   eyepiece field stop must not be clipped; the optical centre is the image centre). Do not
   over-tighten; the screws only need to grip.
7. **Set height**: slide the head up or down (up to `z_adjust`, 4 mm total) until the field is
   sharp and evenly lit with no dark corners; focus on the telescope focuser as usual.
8. **Lock**: tighten the M3 set screw in the socket. Turn the head before locking if the image needs
   rotating.
9. Fix the ESP32-S3 CAM board on the platform with zip ties through the slots.

Light leaks: wrap one turn of black tape around the sleeve bottom where it meets the housing if
stray light is visible in the image.
