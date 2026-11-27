// OAE camera module: OV5640 + ESP32-S3 CAM + focus driver in one box that snaps magnetically onto a
// kinematic ring that stays on each eyepiece (design: plans/camera-module/design.md)
//
// Printed parts:
//   ring        : one per eyepiece (25 / 12.5 / 8 mm). Sleeve over the eyepiece housing, three radial M3
//                 thumbscrews (120 deg) grip and centre it once. The top face is the kinematic seat: three
//                 radial 90 deg V-grooves for the module's balls, three magnet pockets, a key notch, and a
//                 light lip. Its height carries the eye relief of that eyepiece.
//   module_base : one. Island with the ring interface underneath (steel balls, magnets, labyrinth
//                 skirt, key tab, OV5640 nose), tray for the ESP32-S3 CAM board and the USB-C breakout, the
//                 ULN2003 board on posts above the island, wall windows for USB-C and the motor plug.
//   module_lid  : closes the tray (4 x M2 screws), vent slots over the ESP32.
//
// Usage (see README.md):
//   openscad -o ring_12.5.stl -D 'part="ring"' -D eyepiece=12.5 camera_cap.scad
//   openscad -o base.stl      -D 'part="module_base"' camera_cap.scad
//   openscad -o lid.stl       -D 'part="module_lid"'  camera_cap.scad
//
// Frames. Ring: z = 0 is the open (bottom) end of the sleeve, the eyepiece rim is at z = grip_len.
// Module: u = 0 is the flat underside plane that faces the ring (the "roof" of the island), u < 0 is
// the nose, skirt and walls hanging below it, u > 0 is the inside of the box. In the assembly the
// module frame is lifted by z_mod0. All lengths in mm.

// ------------------------------------------------------------------ what to show
part = "assembly";      // "ring" | "module_base" | "module_lid" | "assembly" | "exploded" | "clash"
eyepiece = 12.5;        // 25 | 12.5 | 8 : selects the row of eyepiece_table below
section = false;        // true: flat colour cross-section of the assembly (look from -y); single parts: y < 0 half cut away
section_plane = "y";    // "y": plane y = 0 (nose, ESP32 side) | "x": plane x = 0 (a ball in its groove, a magnet pair)
print_pose = true;      // module_base / module_lid exported in print orientation (see README)
explode = 45;           // exploded view: spacing between ring, module and lid
clash_dz = 0;           // part="clash" self-test: module lowered by this much (0 = as built; -0.1 must then report a clash)
show_lid = true;        // false: leave the lid out of the assembly (look inside the box)

$fn = 96;               // round parts; small holes use $fn = 24..32

// ------------------------------------------------------------------ per-eyepiece defaults
// Measure your eyepieces and edit the row (or override a value with -D housing_od=32.4 ...).
// eye_relief is measured WITH the Barlow in place (paper method, README).
eyepiece_table = [
  // [eyepiece mm, housing OD at the grip, eye relief with Barlow, rim recess (eye lens below rim)]
  [25,   33, 18, 3],
  [12.5, 33,  9, 3],
  [8,    33,  5, 3],
];
_rows = [for (r = eyepiece_table) if (r[0] == eyepiece) r];
assert(len(_rows) == 1, "eyepiece must be one of the first column of eyepiece_table: 25, 12.5, 8");
_row = _rows[0];

housing_od = _row[1];   // eyepiece housing outer diameter where the sleeve grips
eye_relief = _row[2];   // eye lens to exit pupil, with the 3x Barlow
lens_recess = _row[3];  // how far the eye lens sits below the housing rim

// ------------------------------------------------------------------ optics height
lens_stop_offset = 1.5; // camera aperture stop sits this far behind the front glass of the lens
// Rim to front of the camera lens. Negative: the nose enters the eyepiece.
gap = eye_relief - lens_recess - lens_stop_offset;
seat_trim = 0;          // mm added to this ring's seat height after the first test (lens up = +)

// ------------------------------------------------------------------ ring (sleeve + tube + seat)
sleeve_clr = 1.0;       // sleeve ID = housing_od + this (the thumbscrews take up the play)
wall = 2;               // wall thickness of the ring's sleeve and tube
box_wall = 1.6;         // wall thickness of the module's box (4 perimeters)
grip_len = 15;          // sleeve length below the rim (also the z of the rim)
rim_id = 20;            // opening of the eyepiece housing at the rim (nose must fit if gap < 0)
tube_id = rim_id;       // bore of the light tube between rim and seat
floor_t = 2;            // minimum thickness of the plate between rim and the seat's under-side
seat_chamfer = 0.6;     // lead-in / elephant-foot chamfer at the open end of the sleeve
seat_extra = 1.5;       // seat plate radius = sleeve outer radius + this
lip_t = 1.6;            // light lip wall (stands up around the bore, enters the module's groove)
lip_h = 2.2;            // light lip height above the seat plane
lab_clr = 0.4;          // radial clearance of every labyrinth step
key_angle = 60;         // key notch (ring) / key tab (module) direction; keeps the module the same way round
key_w = 4.4;            // key notch width (tab is key_tab_w wide)
key_tab_w = 3.0;        // key tab width on the module skirt
key_depth = 3.0;        // key notch depth below the seat plane

// M3 hardware (thumbscrews)
m3_clear = 3.4;         // clearance hole for M3
nut_af = 5.5;           // M3 nut across flats
nut_t = 2.4;            // M3 nut thickness
nut_clr = 0.3;          // clearance around the nut in its pocket
nut_back = 2.0;         // wall behind the nut pocket
boss_wall = 2.4;        // wall either side of the nut pocket
thumb_z = grip_len / 2; // height of the thumbscrew axes (above the sleeve bottom)
relief_w = 8;           // width of the relief that keeps the seat plate off the nut slots

// ------------------------------------------------------------------ kinematic seat (bought parts)
ball_d = 5;             // steel (bearing) balls, 3 pcs, grade 100 or better
ball_clr = 0.2;         // ball pocket diameter clearance (glue gap)
ball_protrusion = 3;    // how far the balls stick out below the module's underside plane
groove_depth = 3.2;     // V-groove depth in the ring (90 deg included angle, fixed)
seat_r = 16;            // radius of the ball circle (balls and grooves)
mag_d = 6;              // neodymium disc magnets N52, 6 x 3 mm, 6 pcs (3 ring + 3 module)
mag_t = 3;
mag_clr = 0.15;         // magnet pocket diameter clearance
mag_recess = 0.3;       // magnet faces sit this far below the seat plane / underside plane
mag_r = 16;             // radius of the magnet circle (between the balls, 60 deg off)
ball_angles = [90, 210, 330];   // balls and V-grooves, 120 deg apart (= the thumbscrew angles)
mag_angles  = [30, 150, 270];

// ------------------------------------------------------------------ camera module
nose_out = 7;           // nose and skirt hang this far below the underside plane (u = -nose_out)
nose_d = 15;            // nose outer diameter (must fit inside rim_id when the nose is below the rim)
nose_wall_min = 1.2;    // minimum nose wall around the module pocket
lens_in = 0.5;          // lens front recessed this far inside the nose end
skirt_t = 1.8;          // wall of the skirt that wraps the ring's seat plate
low_t = 1.6;            // floor of the tray (outside the island)
lid_t = 1.6;            // lid plate thickness
lid_lip_h = 2;          // lid lip depth inside the walls
lid_lip_t = 1.2;        // lid lip thickness
lid_clr = 0.25;         // lid lip clearance to the wall
corner_r = 5;           // outer corner radius of the box
post_d = 6;             // lid screw posts (4, M2 self tapping)
m2_pilot = 1.7;         // pilot hole for M2 into PETG
m2_clear = 2.4;         // M2 clearance hole in the lid

// OV5640 24-pin module (measure yours)
pcb_w = 8.5;            // sensor PCB width (square)
pcb_t = 1.0;            // PCB thickness
holder_w = 8;           // lens holder width (square)
barrel_d = 8;           // lens barrel diameter (only drawn in the assembly)
module_h = 6;           // lens front to PCB front face (holder + lens height)
fpc_w = 8.5;            // FPC ribbon width
fpc_len = 40;           // FPC ribbon length from the module PCB to its plug end
pocket_clr = 0.4;       // module pocket clearance (total)
conn_h = 2.5;           // connector zone height above the PCB
fpc_slot_t = 2;         // FPC slot thickness (x)
fpc_groove_d = 0.8;     // depth of the FPC groove across the island top

// ESP32-S3 CAM board (measure yours). Long axis along x, its FPC socket end faces the island.
esp_w = 27;             // board width
esp_l = 50;             // board length
esp_pcb_t = 1.6;        // board thickness
esp_stand = 2.5;        // clearance under the board (solder joints, pins)
esp_comp_h = 6;         // tallest part above the board
esp_sock_end = 4;       // camera FPC socket centre: distance from the board end that faces the island
esp_sock_y = 0;         // ... and its offset across the board

// ULN2003 stepper driver board (the common 35 x 32 mm board), on 4 posts above the island
uln = [35, 32];         // board size [x, y]
uln_holes = [30, 27];   // M3 hole spacing [x, y]
uln_pcb_t = 1.6;        // board thickness
uln_stand = 2.5;        // post height above the island top
uln_comp_h = 9;         // tallest part above the board (LEDs, ULN2003, motor socket; room for a mated plug)
uln_pilot = 2.6;        // pilot hole for M3 self tapping screws in the posts
uln_post_d = 5;         // post diameter
uln_sock_x = 0;         // motor socket centre along the board (from the board centre)
uln_sock_h = 6.5;       // motor socket height above the board
uln_sock_w = 15;        // motor socket body width (5 pin XH 2.5 mm)

// USB-C breakout (panel mount, receptacle at the wall)
usb_pcb = [16, 11, 1.0];    // breakout board [length (into the box), width, thickness]
usb_shell = [8.94, 3.26];   // receptacle shell [width, height]
usb_stand = 2;              // clearance under the breakout board
usb_clr = 0.5;              // window clearance (total)

// Motor plug
motor_port = "uln";     // "uln": window to the ULN board's own socket | "jst": window for a JST-XH 5-pin header
motor_win = [18, 10];   // window [width, height]: plug body (socket faces the wall) or cable pass-through (socket faces up)
jst_win = [17.5, 7.5];  // window for a JST-XH 5-pin header body ("jst")

// Vents and antenna
vent_n = 6;             // lid slots over the ESP32 (0 = none)
vent_slot = [2, 16];    // lid slot size [x, y]
wall_vent_n = 5;        // slots low in the -y wall beside the ESP32 (0 = none)
antenna_hole = false;   // 6 mm hole in the +x end wall at the antenna end
antenna_d = 6;

// ------------------------------------------------------------------ derived: ring
R_i    = (housing_od + sleeve_clr) / 2;   // sleeve bore radius
R_go   = R_i + wall;                      // sleeve outer radius
R_tb   = tube_id / 2;                     // tube bore radius
R_t    = R_tb + wall;                     // tube outer radius
R_seat = R_go + seat_extra;               // seat plate radius
R_lip  = R_tb + lip_t;                    // lip outer radius
ball_r = ball_d / 2;
// 90 deg V-groove: the ball centre sits ball_r * sqrt(2) above the vertex
ball_c = ball_r * sqrt(2) - groove_depth;                 // ball centre above the seat plane
seat_gap = ball_c - (ball_r - ball_protrusion);           // underside plane above the seat plane (ball centre is ball_r - ball_protrusion above it)
contact_h = ball_r / sqrt(2);                             // flank contact above the groove vertex
plate_t = groove_depth + 1.2;             // seat plate thickness (1.2 mm under the groove vertex)
gap_mag = seat_gap + 2 * mag_recess;      // distance between facing magnet faces
boss_top = thumb_z + (nut_af / cos(30)) / 2 + boss_wall;  // top of the thumbscrew bosses
nut_depth = nut_t + nut_clr + 0.1;
nut_ac = nut_af / cos(30);                // across corners
// seat plane height: lens front at z_rim + gap, nose end lens_in below it, module underside nose_out above that
z_rim  = grip_len;
z_seat = z_rim + gap - lens_in + nose_out - seat_gap + seat_trim;
z_mod0 = z_seat + seat_gap;               // world height of the module's u = 0
z_pl   = z_seat - plate_t;                // underside of the seat plate
z_gt   = z_rim + floor_t;                 // top of the sleeve cylinder
z_c0   = z_pl - (R_seat - R_t);           // start of the 45 deg cone under the plate

// ------------------------------------------------------------------ derived: module
R_fl   = R_tb - lab_clr;                  // labyrinth flange on the nose (inside the ring's lip)
R_gw   = R_lip + lab_clr;                 // outer wall of the roof groove that takes the lip
R_sk_i = R_seat + 0.5;                    // skirt bore (wraps the seat plate)
R_isl  = R_sk_i + skirt_t;                // island radius
pocket_w = max(pcb_w, holder_w) + pocket_clr;
fw = fpc_w + 0.2;                         // FPC slot / groove width
u_pcb_back = -nose_out + lens_in + module_h + pcb_t;      // back face of the OV5640 PCB
roof_t = max(mag_t + mag_recess, ball_d - ball_protrusion) + 1.2;   // island roof thickness
u_lf   = -nose_out + low_t;               // top of the tray floor
u_top  = roof_t + uln_stand + uln_pcb_t + uln_comp_h + 1;           // lid plane
tab_in = R_seat - 1.2;                    // key tab inner radius
groove_up = lip_h - seat_gap + 0.5;       // roof groove depth above the underside plane
// layout of the box (x: ESP32 side is +x, y: motor window on -y, USB-C strip on +y)
x0i = -(R_isl + 1.5);
esp_x0 = R_isl + 1.5;
x1i = esp_x0 + esp_l + 1.5;
Y_in = max(R_isl + 1.5, esp_w / 2 + 3.5 + usb_pcb[1] + 0.5);
x0o = x0i - box_wall;
x1o = x1i + box_wall;
Yo = Y_in + box_wall;
esp_c = [esp_x0 + esp_l / 2, 0];
esp_u0 = u_lf + esp_stand;                // underside of the ESP32 board
usb_c = [x1i - usb_pcb[0] / 2, Y_in - 0.5 - usb_pcb[1] / 2];
usb_u0 = u_lf + usb_stand;
uln_u0 = roof_t + uln_stand;              // underside of the ULN board
sock_u = uln_u0 + uln_pcb_t + uln_sock_h / 2;             // motor socket centre height
pk = post_d / 2 - 0.4;                    // posts overlap the walls by 0.4 mm (no tangent contact)
post_pts = [[x0i + pk, Y_in - pk], [x0i + pk, -(Y_in - pk)],
            [x1i - pk, -(Y_in - pk)],
            [x1i - usb_pcb[0] - 1 - post_d / 2, Y_in - pk]];
fpc_path = esp_x0 + esp_sock_end + (roof_t - u_lf) / 2;   // rough FPC run from the OV5640 to the socket

// ------------------------------------------------------------------ sanity checks
assert(nose_d + 1 <= tube_id, "nose_d must be at least 1 mm smaller than tube_id");
assert(gap - lens_in >= 0 || nose_d + 1 <= rim_id,
  "nose_d >= rim_id: the nose goes below the rim (small gap) but does not fit the eyepiece opening");
assert(gap - lens_in >= -lens_recess + 0.5,
  "nose would touch the eye lens: eye_relief too small for lens_in and lens_recess");
assert(nose_d >= sqrt(2) * pocket_w + 2 * nose_wall_min,
  "nose_d too small for the square module pocket plus nose_wall_min");
assert((housing_od - rim_id) / 2 >= 2, "rim_id too large for housing_od: no seat left for the rim");
assert(R_tb - R_fl >= 0.3, "lab_clr must be at least 0.3 mm");
assert(R_fl > nose_d / 2, "flange smaller than the nose: increase tube_id or decrease lab_clr");
assert(z_seat - groove_depth - z_rim >= 1.2,
  "ring too short: the groove would break through to the rim; increase nose_out (or eye_relief)");
assert(seat_gap >= 0.6,
  "seat_gap under 0.6 mm: the module's underside could touch the ring; increase ball_protrusion or decrease groove_depth");
assert(seat_gap <= 2.5, "seat_gap over 2.5 mm: the magnets would be too far apart; decrease ball_protrusion or increase groove_depth");
assert(groove_depth >= contact_h + 0.5, "groove too shallow for the ball: the ball would not touch the flanks below the edge");
assert(ball_protrusion > 0.5 && ball_protrusion < ball_d - 1, "ball_protrusion must leave the ball seated in its pocket");
assert(plate_t - (mag_t + mag_recess) >= 1.0, "seat plate too thin for the magnet pocket");
assert(mag_r - (mag_d + mag_clr) / 2 >= R_lip + 1.0, "magnets too close to the lip: increase mag_r");
assert(mag_r + (mag_d + mag_clr) / 2 + 1.0 <= R_seat, "magnets too close to the plate edge: increase seat_extra or decrease mag_r");
assert(mag_r - (mag_d + mag_clr) / 2 >= R_gw + 0.8, "magnet pocket too close to the roof groove: increase mag_r");
assert(seat_r - (ball_d + ball_clr) / 2 >= R_gw + 0.8, "ball pocket too close to the roof groove: increase seat_r");
assert(seat_r - ball_r >= R_lip + 0.6 + 0.5, "balls too close to the lip: increase seat_r");
assert(seat_r + ball_r + 0.5 <= R_seat, "balls beyond the seat plate: decrease seat_r");
assert(z_mod0 - nose_out >= boss_top + 0.8,
  "module skirt would hit the thumbscrew bosses: increase nose_out or the eye relief, or reduce boss height");
assert(roof_t >= 4, "island roof too thin");
assert(fpc_len >= fpc_path + 4, "FPC too short for the ESP32 socket distance: decrease esp_sock_end or use a longer ribbon");
assert(esp_u0 + esp_pcb_t + esp_comp_h + 1 <= u_top, "ESP32 board too tall for the box: lid plane is lower");
assert(max([for (s = [-1, 1], t = [-1, 1]) norm([s * uln_holes[0] / 2, t * uln_holes[1] / 2])]) + uln_post_d / 2 <= R_isl,
  "ULN posts reach beyond the island: reduce the hole pattern or uln_post_d");
assert(uln[0] <= x1i - x0i - 2 && uln[1] <= 2 * Y_in - 2, "ULN board does not fit the box");
assert(sock_u + (motor_port == "uln" ? motor_win[1] : jst_win[1]) / 2 < u_top + lid_lip_h, "motor window above the lid plane");
assert(motor_port == "uln" || motor_port == "jst", "motor_port must be \"uln\" or \"jst\"");
assert(corner_r > box_wall, "corner_r must be larger than box_wall");

// ------------------------------------------------------------------ helpers
e = 0.01;

module rrect(w, l, r) {
  offset(r = r) square([w - 2 * r, l - 2 * r], center = true);
}

// box footprint, inset by off (0 = outer face)
module foot(off = 0) {
  translate([(x0o + x1o) / 2, 0]) rrect(x1o - x0o - 2 * off, 2 * Yo - 2 * off, max(corner_r - off, 0.5));
}

// Solid nut boss along +x. R: radius of the surface it sits on. Profile is (x, z).
module nut_boss(R, z_lo, z_hi) {
  bw = nut_af + nut_clr + 2 * boss_wall;
  x0 = R - 0.8;
  x1 = R + nut_depth + nut_back;
  rotate([90, 0, 0]) linear_extrude(height = bw, center = true)
    polygon([[x0, z_lo], [x1, z_lo], [x1, z_hi], [x0, z_hi]]);
}

// Cuts for one screw+nut along +x: nut slot open at the top, clearance hole through everything.
module nut_cut(R_wall_in, R, z_ax, z_hi) {
  translate([R, -(nut_af + nut_clr) / 2, z_ax - nut_ac / 2 - 0.1])
    cube([nut_depth, nut_af + nut_clr, z_hi - (z_ax - nut_ac / 2 - 0.1) + 1]);
  translate([R_wall_in - 2, 0, z_ax]) rotate([0, 90, 0])
    cylinder(d = m3_clear, h = R + nut_depth + nut_back + 4 - (R_wall_in - 2), $fn = 24);
}

// 90 deg V-groove along +x from r0 to r1, vertex at z = -depth, open at z = 0
module vgroove(r0, r1, depth) {
  translate([r0, 0, 0]) rotate([90, 0, 90]) linear_extrude(r1 - r0)
    polygon([[-(depth + e), e], [depth + e, e], [0, -depth]]);
}

// ------------------------------------------------------------------ ring
thumb_angles = ball_angles;          // thumbscrews sit under the grooves (relief keeps the plate clear)

module ring() {
  difference() {
    union() {
      cylinder(r = R_go, h = z_gt);                       // grip sleeve
      cylinder(r = R_t, h = z_c0 + e);                    // light tube
      rotate_extrude()                                    // cone (45 deg) + seat plate
        polygon([[0, z_c0], [R_t, z_c0], [R_seat, z_pl], [R_seat, z_seat], [0, z_seat]]);
      rotate_extrude()                                    // light lip
        polygon([[R_tb, z_seat - e], [R_lip, z_seat - e], [R_lip, z_seat + lip_h], [R_tb, z_seat + lip_h]]);
      for (a = thumb_angles) rotate(a) nut_boss(R_go, 0, boss_top);
    }
    // bore: sleeve (with lead-in) / rim shoulder / tube
    rotate_extrude()
      polygon([[0, -e], [R_i + seat_chamfer, -e], [R_i + seat_chamfer, 0], [R_i, seat_chamfer],
               [R_i, z_rim], [R_tb, z_rim], [R_tb, z_seat + lip_h + e], [0, z_seat + lip_h + e]]);
    for (a = thumb_angles) rotate(a) nut_cut(R_i, R_go, thumb_z, boss_top);
    // relief: keeps the seat plate and cone off the nut slots (nuts drop in from above)
    for (a = thumb_angles)
      rotate(a) translate([R_go - e, -relief_w / 2, boss_top]) cube([30, relief_w, 100]);
    // V-grooves for the module's balls
    for (a = ball_angles)
      rotate(a) translate([0, 0, z_seat]) vgroove(R_lip + 0.6, R_seat + 3, groove_depth);
    // magnet pockets
    for (a = mag_angles)
      rotate(a) translate([mag_r, 0, z_seat - (mag_t + mag_recess)])
        cylinder(d = mag_d + mag_clr, h = mag_t + mag_recess + 1, $fn = 48);
    // key notch (open to the plate edge)
    rotate(key_angle) translate([tab_in - 0.6, -key_w / 2, z_seat - key_depth])
      cube([R_seat - tab_in + 3, key_w, key_depth + 1]);
  }
}

// ------------------------------------------------------------------ module base (u frame)
module bx(x0, x1, y0, y1, z0, z1) {
  translate([min(x0, x1), min(y0, y1), z0]) cube([abs(x1 - x0), abs(y1 - y0), z1 - z0]);
}

// four corner pads (under the board) + L stops (outside its edges) around a w x l board, centred
module board_seat(w, l, stand, t) {
  pad = 4; stop = 1.2; clr = 0.3;
  for (sx = [-1, 1], sy = [-1, 1]) {
    bx(sx * w / 2, sx * (w / 2 - pad), sy * l / 2, sy * (l / 2 - pad), 0, stand);
    bx(sx * (w / 2 + clr), sx * (w / 2 + clr + stop), sy * (l / 2 + clr + stop), sy * (l / 2 - pad), 0, stand + t * 0.7);
    bx(sx * (w / 2 - pad), sx * (w / 2 + clr + stop), sy * (l / 2 + clr), sy * (l / 2 + clr + stop), 0, stand + t * 0.7);
  }
}

module window_cut_motor() {   // through the -y wall, at the ULN board's motor socket (also through the lid lip)
  ww = motor_port == "uln" ? motor_win : jst_win;
  translate([uln_sock_x - ww[0] / 2, -Yo - 1, sock_u - ww[1] / 2]) cube([ww[0], box_wall + lid_lip_t + 3, ww[1]]);
}

module module_base() {
  difference() {
    union() {
      // tray floor outside the island
      translate([0, 0, -nose_out]) linear_extrude(low_t)
        difference() { foot(0); circle(r = R_sk_i); }
      // walls (they reach the print bed so the roof can bridge between skirt and walls)
      translate([0, 0, -nose_out]) linear_extrude(u_top + nose_out)
        difference() { foot(0); foot(box_wall); }
      // skirt wall of the island, from the bed up to the roof
      translate([0, 0, -nose_out]) linear_extrude(nose_out + e)
        difference() { circle(r = R_isl); circle(r = R_sk_i); }
      // roof (island top, carries the ULN board)
      cylinder(r = R_isl, h = roof_t);
      // nose and its labyrinth flange (45 deg under-side)
      translate([0, 0, -nose_out]) cylinder(d = nose_d, h = nose_out + e);
      rotate_extrude()
        polygon([[0, e], [R_fl, e], [R_fl, -1.0], [nose_d / 2, -1.0 - (R_fl - nose_d / 2)],
                 [0, -1.0 - (R_fl - nose_d / 2)]]);
      // key tab on the skirt bore, 45 deg under-side
      rotate(key_angle) rotate([90, 0, 0]) linear_extrude(height = key_tab_w, center = true)
        polygon([[R_sk_i + 0.3, e], [tab_in, e], [tab_in, -2.0], [R_sk_i + 0.3, -2.0 - (R_sk_i + 0.3 - tab_in)]]);
      // lid screw posts
      for (p = post_pts) translate([p[0], p[1], -nose_out]) cylinder(d = post_d, h = u_top + nose_out, $fn = 32);
      // ULN posts on the island top
      for (sx = [-1, 1], sy = [-1, 1])
        translate([sx * uln_holes[0] / 2, sy * uln_holes[1] / 2, roof_t - e])
          cylinder(d = uln_post_d, h = uln_stand + e, $fn = 32);
      // ESP32 seat (long axis along x)
      translate([esp_c[0], esp_c[1], u_lf - e]) board_seat(esp_l, esp_w, esp_stand + e, esp_pcb_t);
      // USB-C breakout seat
      translate([usb_c[0], usb_c[1], u_lf - e]) board_seat(usb_pcb[0], usb_pcb[1], usb_stand + e, usb_pcb[2]);
      // JST header shelf
      if (motor_port == "jst")
        translate([uln_sock_x - 10, -Y_in, u_lf - e]) cube([20, 6, sock_u - jst_win[1] / 2 - 1 - u_lf]);
    }
    // OV5640: square pocket open at the nose end, connector zone, FPC slot, groove toward the ESP32
    translate([-pocket_w / 2, -pocket_w / 2, -nose_out - e]) cube([pocket_w, pocket_w, u_pcb_back + nose_out + e]);
    translate([-fpc_slot_t / 2 - 1, -fw / 2, u_pcb_back - e]) cube([fpc_slot_t + 2, fw, conn_h + e]);
    translate([-fpc_slot_t / 2, -fw / 2, u_pcb_back + conn_h - e]) cube([fpc_slot_t, fw, roof_t - (u_pcb_back + conn_h) + 2 * e]);
    translate([-fpc_slot_t / 2, -fw / 2, roof_t - fpc_groove_d]) cube([R_isl + fpc_slot_t / 2 + 0.5, fw, fpc_groove_d + e]);
    // roof groove that takes the ring's light lip (labyrinth)
    rotate_extrude() polygon([[R_fl, -e], [R_gw, -e], [R_gw, groove_up], [R_fl, groove_up]]);
    // ball pockets (balls are pressed in from below up to the pocket ceiling, then glued)
    for (a = ball_angles) rotate(a) translate([seat_r, 0, -e])
      cylinder(d = ball_d + ball_clr, h = ball_d - ball_protrusion + e, $fn = 32);
    // magnet pockets (magnets are pressed in from below up to the pocket ceiling, then glued)
    for (a = mag_angles) rotate(a) translate([mag_r, 0, -e])
      cylinder(d = mag_d + mag_clr, h = mag_t + mag_recess + e, $fn = 48);
    // lid screw pilot holes
    for (p = post_pts) translate([p[0], p[1], u_top - 8]) cylinder(d = m2_pilot, h = 9, $fn = 24);
    // ULN pilot holes (M3 self tapping, 5 mm deep)
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * uln_holes[0] / 2, sy * uln_holes[1] / 2, uln_u0 - 5]) cylinder(d = uln_pilot, h = 6, $fn = 24);
    // USB-C window in the +x end wall
    translate([x1i - 1, usb_c[1] - (usb_shell[0] + usb_clr) / 2, usb_u0 + usb_pcb[2] + usb_shell[1] / 2 - (usb_shell[1] + usb_clr) / 2])
      cube([box_wall + 2, usb_shell[0] + usb_clr, usb_shell[1] + usb_clr]);
    // motor plug window in the -y wall
    window_cut_motor();
    // wall vents beside the ESP32 (-y wall)
    for (i = [0 : wall_vent_n - 1])
      translate([esp_x0 + 8 + i * 8, -Yo - 1, u_lf + 3]) cube([vent_slot[0] - 0.4, box_wall + 2, 8]);
    // antenna hole (far +x end, board centre line)
    if (antenna_hole)
      translate([x1i - 1, esp_c[1], esp_u0 + esp_pcb_t + 1]) rotate([0, 90, 0]) cylinder(d = antenna_d, h = box_wall + 2, $fn = 32);
  }
}

// ------------------------------------------------------------------ lid (u frame)
module module_lid() {
  difference() {
    union() {
      translate([0, 0, u_top]) linear_extrude(lid_t) foot(0);
      // lip inside the walls
      difference() {
        translate([0, 0, u_top - lid_lip_h]) linear_extrude(lid_lip_h + e)
          difference() { foot(box_wall + lid_clr); foot(box_wall + lid_clr + lid_lip_t); }
        for (p = post_pts) translate([p[0], p[1], u_top - lid_lip_h - 1]) cylinder(d = post_d + 0.6, h = lid_lip_h + 2, $fn = 32);
      }
    }
    for (p = post_pts) translate([p[0], p[1], u_top - 1]) cylinder(d = m2_clear, h = lid_t + 2, $fn = 24);
    // vent slots over the ESP32
    for (i = [0 : vent_n - 1])
      translate([esp_x0 + 8 + i * 6 - vent_slot[0] / 2, -vent_slot[1] / 2, u_top - 1]) cube([vent_slot[0], vent_slot[1], lid_t + 2]);
    window_cut_motor();
    // USB-C window reaches into the lip too
    translate([x1i - 3, usb_c[1] - (usb_shell[0] + usb_clr) / 2, usb_u0 + usb_pcb[2] + usb_shell[1] / 2 - (usb_shell[1] + usb_clr) / 2])
      cube([box_wall + 6, usb_shell[0] + usb_clr, usb_shell[1] + usb_clr]);
  }
}

// ------------------------------------------------------------------ ghosts and hardware (assembly only)
module eyepiece_ghost() {
  len_vis = 25;
  color("silver", 0.5) difference() {
    translate([0, 0, -len_vis]) cylinder(d = housing_od, h = len_vis + z_rim);
    translate([0, 0, z_rim - lens_recess]) cylinder(d = rim_id, h = lens_recess + e);
  }
  color("skyblue", 0.8) translate([0, 0, z_rim - lens_recess - 1]) cylinder(d = rim_id - 1, h = 1);
}

module ov5640_ghost() {   // module frame
  color("limegreen") translate([-pcb_w / 2, -pcb_w / 2, u_pcb_back - pcb_t]) cube([pcb_w, pcb_w, pcb_t]);
  color("dimgray") translate([-holder_w / 2, -holder_w / 2, -nose_out + lens_in + 3])
    cube([holder_w, holder_w, module_h - 3]);
  color("black") translate([0, 0, -nose_out + lens_in]) cylinder(d = barrel_d, h = 3);
}

module balls_ghost() {    // module frame
  color("gainsboro") for (a = ball_angles) rotate(a) translate([seat_r, 0, ball_r - ball_protrusion]) sphere(r = ball_r, $fn = 32);
}

module ring_magnets() {   // ring frame: north up (red)
  color("red") for (a = mag_angles) rotate(a) translate([mag_r, 0, z_seat - mag_recess - mag_t]) cylinder(d = mag_d, h = mag_t, $fn = 32);
}

module module_magnets() { // module frame: south down (blue)
  color("royalblue") for (a = mag_angles) rotate(a) translate([mag_r, 0, mag_recess]) cylinder(d = mag_d, h = mag_t, $fn = 32);
}

module boards_ghost() {   // module frame
  // ESP32-S3 CAM
  color("seagreen") translate([esp_c[0] - esp_l / 2, esp_c[1] - esp_w / 2, esp_u0]) cube([esp_l, esp_w, esp_pcb_t]);
  color("dimgray") translate([esp_c[0] - 6, esp_c[1] - 9, esp_u0 + esp_pcb_t]) cube([18, 18, 3.2]);
  color("black") translate([esp_x0 + esp_sock_end - 2, esp_sock_y - 6, esp_u0 + esp_pcb_t]) cube([4, 12, 2]);
  // ULN2003 driver
  color("steelblue") translate([-uln[0] / 2, -uln[1] / 2, uln_u0]) cube([uln[0], uln[1], uln_pcb_t]);
  color("white") translate([uln_sock_x - uln_sock_w / 2, -uln[1] / 2, uln_u0 + uln_pcb_t]) cube([uln_sock_w, 6, uln_sock_h]);
  color("black") translate([-8, -4, uln_u0 + uln_pcb_t]) cube([20, 8, 4.5]);
  // USB-C breakout
  color("goldenrod") translate([usb_c[0] - usb_pcb[0] / 2, usb_c[1] - usb_pcb[1] / 2, usb_u0]) cube(usb_pcb);
  color("silver") translate([usb_c[0] + usb_pcb[0] / 2 - 7, usb_c[1] - usb_shell[0] / 2, usb_u0 + usb_pcb[2]])
    cube([7, usb_shell[0], usb_shell[1]]);
}

module sec() {   // section=true keeps only y >= 0 (cut applied per part so colours survive)
  if (section)
    intersection() {
      children();
      translate([-300, 0, -300]) cube([600, 600, 600]);
    }
  else children();
}

module module_all(lid_dz = 0) {   // module frame: base + hardware + lid
  sec() color("steelblue") module_base();
  sec() ov5640_ghost();
  sec() balls_ghost();
  sec() module_magnets();
  sec() boards_ghost();
  if (show_lid) translate([0, 0, lid_dz]) sec() color("lightsteelblue", 0.9) module_lid();
}

module assembly(ex = 0) {
  sec() color("orange") ring();
  sec() ring_magnets();
  sec() eyepiece_ghost();
  translate([0, 0, z_mod0 + ex]) module_all(ex);
}

// ------------------------------------------------------------------ console summary
echo(str("eyepiece ", eyepiece, ": gap (rim to lens front) = ", gap, " mm, ring top of lip at z = ",
         z_seat + lip_h, " mm, seat plane z = ", z_seat, " mm, seat_gap = ", seat_gap, " mm, magnet face gap = ", gap_mag, " mm"));
echo(str("module outer size (x y z, lid on) = ", x1o - x0o, " x ", 2 * Yo, " x ", u_top + lid_t + nose_out,
         " mm; FPC run to the ESP32 socket about ", fpc_path, " mm of ", fpc_len, " mm"));

// ------------------------------------------------------------------ views
module base_view() {
  if (print_pose) translate([0, 0, nose_out]) sec() module_base();
  else sec() module_base();
}

module lid_view() {   // print pose: plate on the bed, lip pointing up
  if (print_pose) translate([0, 0, u_top + lid_t]) rotate([180, 0, 0]) sec() module_lid();
  else sec() module_lid();
}

// flat cross-section: every part cut at its plane and drawn in its own colour (OpenSCAD's preview does not cap cuts)
module cut2d() {
  if (section_plane == "y") rotate([-90, 0, 0]) children();
  else rotate([-90, 0, 0]) rotate([0, 0, -90]) children();
}
module secview() {
  color("orange") projection(cut = true) cut2d() ring();
  color("lightgray") projection(cut = true) cut2d() eyepiece_ghost();
  color("steelblue") projection(cut = true) cut2d() translate([0, 0, z_mod0]) module_base();
  color("limegreen") projection(cut = true) cut2d() translate([0, 0, z_mod0]) ov5640_ghost();
  color("purple") projection(cut = true) cut2d() translate([0, 0, z_mod0]) module_lid();
  color("darkgreen") projection(cut = true) cut2d() translate([0, 0, z_mod0]) boards_ghost();
  if (section_plane == "x") {   // balls and magnets lie on this plane (90 and 270 degrees)
    color("red") projection(cut = true) cut2d() ring_magnets();
    color("white") projection(cut = true) cut2d() translate([0, 0, z_mod0]) balls_ghost();
    color("blue") projection(cut = true) cut2d() translate([0, 0, z_mod0]) module_magnets();
  }
}

if ((part == "assembly" || part == "exploded") && section) secview();
else if (part == "ring") sec() ring();
else if (part == "module_base") base_view();
else if (part == "module_lid") lid_view();
else if (part == "assembly") assembly();
else if (part == "exploded") assembly(explode);
else if (part == "clash") {   // developer check: everything that must not touch the ring (should render empty)
  intersection() {
    ring();
    translate([0, 0, z_mod0 + clash_dz]) union() { module_base(); module_lid(); balls_ghost(); module_magnets(); }
  }
}
else assert(false, "part must be \"ring\", \"module_base\", \"module_lid\", \"assembly\", \"exploded\" or \"clash\"");
