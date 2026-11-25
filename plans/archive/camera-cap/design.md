# Camera cap: OV5640 directly on the eyepiece (design)

Date: 2026-11-26. User: "I need simpler design for it like placing camera directly on the eyepiece",
"Design it with the data that you can find from internet".

Telescope: National Geographic (Bresser) 76/700 Newtonian, f/9.2, 1.25" focuser, eyepieces 25, 12.5,
8 mm (confirmed by the user), almost always with the 3x Barlow. Camera: ESP32-S3 CAM board + OV5640 24-pin fixed-focus module
(lens focused at infinity), afocal behind the eyepiece.

## Parts

1. **Eyepiece cap** (one print per eyepiece, same file, different parameters): a sleeve that slides over
   the top of the eyepiece housing and is held by **three radial M3 thumbscrews at 120 deg** (heat-set
   insert or captive hex nut pocket). The three screws both grip and centre the camera axis on the
   eyepiece axis. The sleeve continues above the eyepiece rim as a closed light-tight tube up to a top
   socket that takes the camera head. Inside: matte, no reflections (rib/step baffles optional).
2. **Camera head** (one print, moves between caps): a puck that friction-fits into any cap's top socket
   (plus one M3 set screw in the socket wall to lock it), holding the OV5640 module with its lens on the
   cap axis pointing down, an exit slot for the FPC ribbon, and on top a flat platform for the ESP32-S3
   CAM board with zip-tie slots. Module retained by a printed clip or two M2 screws; parametric.

## Optics height

Distance from the eyepiece rim (top edge of the housing) to the front of the camera lens:

`gap = eye_relief - lens_recess - lens_stop_offset`

- `eye_relief`: from the eyepiece glass to the exit pupil, with the 3x Barlow. Defaults (no published
  data for these Bresser kit eyepieces; Kellner typical + Barlow shift): 25 mm -> 18, 12.5 mm -> 9,
  8 mm -> 5.
- `lens_recess`: how far the eye lens sits below the rim, default 3.
- `lens_stop_offset`: aperture stop behind the camera lens's front glass, default 1.5.
- If `gap` < 0 the camera nose protrudes into the eyepiece recess; the nose diameter
  (`nose_d`, default 10) must then fit inside the rim opening (`rim_id`, default 20). The design must
  handle both cases.
- Fine height: the head sits in the socket on a short travel (`z_adjust`, default 4 mm) locked by the
  socket set screw, so measured eye relief within a few mm needs no reprint.

## Default dimensions (all parameters at the top of the file)

- Eyepiece housing OD at the grip: 33 mm (budget 1.25" Kellner, e.g. Bushnell 25 mm Kellner is 33 mm
  wide); per-eyepiece values in a table, all 33 until measured. Sleeve ID = OD + 1.0 clearance (the
  screws take up the play), grip length 15 mm.
- OV5640 module: sensor PCB 8.5 x 8.5 mm, lens holder 8 x 8 mm, lens barrel OD 8 mm, holder+lens height
  6 mm, FPC width 8.5 mm. All parameters; measure the real module.
- ESP32-S3 CAM board platform 27 x 50 mm (parameter), zip-tie slots 4 x 2 mm.
- Wall 2 mm, PETG, no supports if possible (print orientation stated in comments).

## Deliverables

- `hardware/camera-cap/camera_cap.scad`: parametric, `part = "cap" | "head" | "assembly"`,
  `eyepiece = 25 | 12.5 | 8` selects the defaults row.
- `hardware/camera-cap/README.md`: what to measure (housing OD, eye relief by the paper method, rim
  recess, module sizes), how to export STLs, print settings, assembly and centring, the gap formula.
- Rendered STLs are not committed (build output); a PNG preview of the assembly is fine.
