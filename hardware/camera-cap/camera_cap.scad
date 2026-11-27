// OAE camera module: OV5640 + ESP32-S3 CAM + focus driver in one compact box, centred on the optical axis,
// that snaps magnetically onto a kinematic ring that stays on each eyepiece (design: plans/archive/camera-module/design.md)
//
// Printed parts:
//   ring        : one per eyepiece (25 / 12.5 / 8 mm). Sleeve over the eyepiece housing, three radial M3
//                 thumbscrews (120 deg) grip and centre it once. The top face is the kinematic seat: three
//                 radial 90 deg V-grooves for the module's balls, three magnet pockets, a key notch, and a
//                 light lip. Its height carries the eye relief of that eyepiece.
//   module_base : one. Compact box centred on the optical axis, boards stacked: the ring interface underneath
//                 (steel balls, magnets, labyrinth skirt, key tab, OV5640 nose), the island floor, the ESP32-S3 CAM
//                 board 2.5 mm above it (the ribbon runs under it and wraps its end), the ULN2003 driver on four
//                 posts above the ESP32, USB-C breakout and JST-XH motor socket behind two windows in one end wall,
//                 and the Mini-360 buck converter (15 V in over the cable, 5.1 V out) with its output capacitor in the
//                 strip beside the ESP32, behind the wall vents.
//   module_lid  : closes the box (4 x M2 screws), vent slots over the boards.
//
// Usage (see README.md):
//   openscad -o ring_12.5.stl -D 'part="ring"' -D eyepiece=12.5 camera_cap.scad
//   openscad -o base.stl      -D 'part="module_base"' camera_cap.scad
//   openscad -o lid.stl       -D 'part="module_lid"'  camera_cap.scad
//
// Frames. Ring: z = 0 is the open (bottom) end of the sleeve, the eyepiece rim is at z = grip_len.
// Module: u = 0 is the flat underside plane that faces the ring (the "roof" of the island), u < 0 is
// the nose, skirt and walls hanging below it, u > 0 is the inside of the box. Layout frame = module frame
// with the ports at +x (mx() mirrors the interior for port_side = "-x"). In the assembly the
// module frame is lifted by z_mod0. All lengths in mm.

// ------------------------------------------------------------------ what to show
part = "assembly";      // "ring" | "module_base" | "module_lid" | "assembly" | "exploded" | "clash" | "clash_in"
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
// Compact layout, centred on the optical axis, boards stacked: OV5640 lowest (nose into the ring),
// ESP32-S3 CAM above it, driver (ULN2003) on top. USB-C and the motor socket share one x end wall.
nose_out = 7;           // nose and skirt hang this far below the underside plane (u = -nose_out)
nose_d = 15;            // nose outer diameter (must fit inside rim_id when the nose is below the rim)
nose_wall_min = 1.2;    // minimum nose wall around the module pocket
lens_in = 0.5;          // lens front recessed this far inside the nose end
skirt_t = 1.8;          // wall of the skirt that wraps the ring's seat plate
low_t = 1.6;            // floor of the box outside the island (its top is flush with the island roof)
lid_t = 1.6;            // lid plate thickness
lid_lip_h = 2;          // lid lip depth inside the walls
lid_lip_t = 1.2;        // lid lip thickness
lid_clr = 0.25;         // lid lip clearance to the wall
corner_r = 5;           // outer corner radius of the box
cav_r = 0.8;            // inner corner radius of the box cavity (nearly square: the boards reach the corners)
post_d = 6;             // lid screw posts (4, M2 self tapping)
m2_pilot = 1.7;         // pilot hole for M2 into PETG
m2_clear = 2.4;         // M2 clearance hole in the lid

// Layout choices
port_side = "+x";       // "+x" | "-x": end wall with the USB-C and motor sockets (put it away from the focus motor);
                        // the interior is mirrored, the ring interface and key are not
usb_side = 1;           // +1 | -1: y strip beside the ESP32 that carries the USB-C breakout (the motor socket takes the other)
driver = "board";       // "board": common ULN2003 board 35 x 32 | "dip": bare ULN2003A DIP-16 on a small perfboard

// OV5640 24-pin module (measure yours)
pcb_w = 8.5;            // sensor PCB width (square)
pcb_t = 1.0;            // PCB thickness
holder_w = 8;           // lens holder width (square)
barrel_d = 8;           // lens barrel diameter (only drawn in the assembly)
module_h = 6;           // lens front to PCB front face (holder + lens height)
fpc_w = 8.5;            // FPC ribbon width
fpc_len = 40;           // FPC ribbon length from the module PCB to its plug end
fpc_bend = 1.0;         // ribbon used up by each 90 degree fold
fpc_margin = 2;         // spare ribbon the model insists on
pocket_clr = 0.4;       // module pocket clearance (total)
conn_h = 2.5;           // connector zone height above the PCB
fpc_slot_t = 2;         // FPC slot thickness (x)
fpc_groove_d = 0.8;     // depth of the bend relief across the island top
fpc_relief = 8;         // length of that relief (toward the end where the ribbon rises)

// ESP32-S3 CAM board (measure yours). Long axis along x, component side up, camera socket at the end away from the ports.
esp_w = 27;             // board width
esp_l = 50;             // board length
esp_pcb_t = 1.6;        // board thickness
esp_stand = 2.5;        // clearance under the board (solder joints, pins, microSD)
esp_comp_h = 6;         // tallest part above the board
esp_sock_end = 4;       // camera FPC socket centre: distance from the board end (the end away from the ports)
esp_dx = 6;             // board centre shifted toward the ports (shortens the ribbon, mass is balanced by the driver)
esp_chan = 2.4;         // free gap between the board end and the wall at the ribbon end
esp_holes = [];         // optional mounting holes [[dx, dy], ...] from the board centre (M2 posts); none: pads, stops and a dab of glue
esp_post_d = 5;

// Driver. "board": the common 35 x 32 mm board on 4 posts (turned 90 degrees: its 35 mm side runs along y).
// "dip": a bare ULN2003A on a perfboard of the same width, shorter along x (lower, lighter, no footprint gain).
_drivers = [
  // [name, board [long, short], M3 holes [long, short], tallest part above the board, mass g]
  ["board", [35, 32], [30, 27], 9, 8],
  ["dip",   [34, 20], [30, 12], 6, 3],
];
_drv_rows = [for (d = _drivers) if (d[0] == driver) d];
assert(len(_drv_rows) == 1, "driver must be \"board\" or \"dip\"");
uln = _drv_rows[0][1];        // board size [long, short]
uln_holes = _drv_rows[0][2];  // M3 hole spacing [long, short]
uln_comp_h = _drv_rows[0][3]; // tallest part above the board (LEDs, ULN2003, header; room for a plug)
m_uln = _drv_rows[0][4];      // board mass
uln_x = -7;             // board centre along x (away from the ports; the USB-C breakout and motor socket need the other end)
uln_pcb_t = 1.6;        // board thickness
uln_clr = 1.2;          // clearance between the ESP32's tallest part and the driver board
uln_top_clr = 1.2;      // clearance between the driver's tallest part and the lid
uln_pilot = 2.6;        // pilot hole for M3 self tapping screws in the posts
uln_post_d = 5;         // post diameter
esp_edge_clr = 0.25;    // posts stay this far off the ESP32's long edge (they also stop it sideways)

// USB-C breakout (panel mount, receptacle at the wall)
usb_pcb = [16, 11, 1.0];    // breakout board [length (into the box), width, thickness]
usb_shell = [8.94, 3.26];   // receptacle shell [width, height]
usb_stand = 2;              // clearance under the breakout board
usb_clr = 0.5;              // window clearance (total)
usb_gap = 0.4;              // gap between the ESP32's long edge and the breakout
usb_wall_clr = 0.8;         // gap between the breakout and the outer y wall

// Motor socket: JST-XH 5-pin male header with side entry, standing on its edge (row of pins vertical)
// in a cradle in the other y strip, fed through a window in the same end wall; wires go on to the driver.
jst_body = [14.9, 5.75, 7.0];   // [row length, width, depth along the plug direction]
jst_blk = 9;                    // cradle length along x from the wall
jst_wall = 1.0;                 // cradle wall behind the header
jst_tail = 3.5;                 // open space beside the header for its pins and solder joints
jst_blk_h = 8;                  // cradle height above its floor (the header stands above it)

// Power: Mini-360 buck converter (MP2307 type), 15 V in over the cable's VBUS, 5.1 V out for the ESP32 and the driver.
// It lies flat on two short rails in the motor-side strip beside the ESP32, behind the wall vents, far from the ribbon,
// OUT end toward +x (layout frame). The output capacitor (100 uF electrolytic) lies next to it in a cradle on the wall
// side; the 10 uF ceramic needs no pocket (1206 across the OUT pads, or radial on the pads: free air above the board).
// There is no access hole for the trim pot: the driver board sits over half of the Mini-360. Set 5.1 V on the bench first.
mini_pcb = [17, 11, 1.0];   // Mini-360 board [length, width, thickness] (measure yours)
mini_h = 4;                 // height from the board's underside to the top of the inductor
mini_stand = 1.5;           // clearance under the board (corner pads, wire stubs)
mini_pad = 3.5;             // corner zone with the four solder pads: the rails stay out of it
mini_rail_w = 1.8;          // rail width under each long edge
mini_post_clr = 0.6;        // gap between the board and the driver post at the ribbon end
mini_wall_clr = 0.3;        // gap between the board and the outer y wall
cap_d = 8;                  // output capacitor diameter (100 uF, 8 x 12 mm)
cap_l = 12;                 // its length (lies along x)
cap_clr = 0.2;              // diameter clearance in the cradle
cap_lift = 0.6;             // clearance between the capacitor and the floor
cap_gap = 1.6;              // gap between the Mini-360 OUT end and the capacitor (leads, solder joints)
cap_saddle_l = 2;           // length of each of the two cradle blocks

// Vents and antenna (all above the island: nothing opens over the optical path)
vent_n = 6;             // lid slots over the boards (0 = none)
vent_slot = [2, 16];    // lid slot size [x, y]
vent_x0 = -14;          // x of the first lid slot
vent_pitch = 6;
wall_vent_n = 5;        // slots in the y wall beside the ESP32, on the motor socket side (0 = none)
wall_vent_x0 = -12;
wall_vent_pitch = 7;
antenna_hole = false;   // 6 mm hole in the end wall at the antenna end
antenna_d = 6;

// Masses for the centre of mass estimate (g)
m_esp = 10;  m_ov = 3;  m_usb = 2;  m_jst = 1;  m_mini = 2;  m_cap = 2;   // m_cap: electrolytic and ceramic
m_hw = 6;               // magnets (3), balls (3), screws
petg_dens = 1.27 * 0.4; // g/cm3 x effective fill
com_limit = 5;          // allowed lateral centre of mass offset from the optical axis (mm)

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
tab_in = R_seat - 1.2;                    // key tab inner radius
groove_up = lip_h - seat_gap + 0.5;       // roof groove depth above the underside plane
// layout frame: ports at +x, the ribbon rises at -x. mx() mirrors the interior for port_side = "-x".
s = port_side == "+x" ? 1 : -1;
crop_y = esp_w / 2 + esp_edge_clr;          // inner face of the driver posts and of the motor cradle
ox = uln_holes[1] / 2;  oy = uln_holes[0] / 2;    // post offsets from the driver centre (board turned 90 degrees)
lx = uln[1];  ly = uln[0];                        // driver board extent along x, y
esp_x_min = esp_dx - esp_l / 2;
esp_x_max = esp_dx + esp_l / 2;
x0i = min(-(R_isl + 1.5), esp_x_min - esp_chan);
x1i = esp_x_max + 1.5;
cap_dc = cap_d + cap_clr;                 // cradle diameter
mini_y_need = oy + uln_post_d / 2 + 0.1 + cap_dc + 0.2;   // capacitor between the driver post and the wall
Y_in = max(R_isl + 1.5, esp_w / 2 + usb_gap + usb_pcb[1] + usb_wall_clr, mini_y_need);
x0o = x0i - box_wall;
x1o = x1i + box_wall;
Yo = Y_in + box_wall;
esp_u0 = roof_t + esp_stand;              // underside of the ESP32 board
esp_face = esp_u0 + esp_pcb_t;            // its component face
esp_top = esp_face + esp_comp_h;          // tallest part
uln_u0 = esp_top + uln_clr;               // underside of the driver board
u_top = uln_u0 + uln_pcb_t + uln_comp_h + uln_top_clr;    // lid plane
usb_u0 = roof_t + usb_stand;
usb_c = [x1i - usb_pcb[0] / 2, usb_side * (esp_w / 2 + usb_gap + usb_pcb[1] / 2)];
jst_cross = jst_body[1] + 0.4;
jst_y0 = Y_in - jst_wall - jst_cross;     // header inner face (|y|)
jst_yc = -usb_side * (jst_y0 + jst_cross / 2);            // header centre y
jst_floor = roof_t + 1.2;                 // cradle floor
jst_win = [jst_body[1] + 1.5, jst_body[0] + 2.6];         // window in the wall [y, u]
mini_sg = -usb_side;                      // Mini-360 and capacitor strip: the motor side
mini_x0 = uln_x - ox + uln_post_d / 2 + mini_post_clr;    // board end toward the ribbon (x)
mini_x1 = mini_x0 + mini_pcb[0];          // OUT end
mini_yc = mini_sg * (Y_in - mini_wall_clr - mini_pcb[1] / 2);
mini_z0 = roof_t + mini_stand;            // underside of the board
cap_x0 = mini_x1 + cap_gap;
cap_x1 = cap_x0 + cap_l;
cap_yc = mini_sg * (Y_in - 0.2 - cap_dc / 2);
cap_zc = roof_t + cap_lift + cap_dc / 2;  // capacitor axis
lp = post_d / 2 - 0.4;                    // lid posts overlap the walls by 0.4 mm (no tangent contact)
post_pts = [[x0i + lp, Y_in - lp], [x0i + lp, -(Y_in - lp)],
            [x1i - usb_pcb[0] - 1 - post_d / 2, usb_side * (Y_in - lp)],
            [x1i - jst_blk - 1 - post_d / 2, -usb_side * (Y_in - lp)]];
// ribbon: up through the island, along the roof to the board end, up the gap, back over the board to the socket
x_ch = esp_x_min - esp_chan / 2;          // where it rises past the board end
x_sock = esp_x_min + esp_sock_end;
fpc_path = (roof_t - u_pcb_back) + abs(x_ch) + (esp_face + 1 - roof_t) + (x_sock - x_ch) + 3 * fpc_bend;

// centre of mass (layout frame; x flips with port_side, which does not change the offset)
foot_a = (x1o - x0o) * 2 * Yo - (4 - PI) * corner_r * corner_r;
foot_p = 2 * ((x1o - x0o) + 2 * Yo) - (8 - 2 * PI) * corner_r;
cxl = (x0o + x1o) / 2;
_pxm = (post_pts[0][0] + post_pts[1][0] + post_pts[2][0] + post_pts[3][0]) / 4;
_pym = (post_pts[0][1] + post_pts[1][1] + post_pts[2][1] + post_pts[3][1]) / 4;
_cry = -usb_side * (crop_y + Y_in) / 2;
_v_isl = PI * R_isl * R_isl * roof_t;
_v_flr = (foot_a - PI * R_isl * R_isl) * low_t;
_v_wal = foot_p * box_wall * (u_top + nose_out);
_v_lid = foot_a * lid_t + foot_p * lid_lip_t * lid_lip_h;
_v_skt = PI * (R_isl * R_isl - R_sk_i * R_sk_i) * nose_out + PI * nose_d * nose_d / 4 * nose_out - pocket_w * pocket_w * nose_out;
_v_pst = 4 * PI * post_d * post_d / 4 * (u_top - roof_t + low_t) * 0.8;
_v_col = 4 * 0.7 * PI * uln_post_d * uln_post_d / 4 * (uln_u0 - roof_t);
_v_crd = jst_blk * (Y_in - crop_y) * jst_blk_h;
// [mass g, x, y, u]
com_items = [
  [m_esp, esp_dx, 0, esp_u0 + esp_pcb_t / 2],
  [m_uln, uln_x, 0, uln_u0 + uln_pcb_t / 2 + 1],
  [m_ov, 0, 0, -nose_out + lens_in + module_h / 2],
  [m_usb, usb_c[0], usb_c[1], usb_u0 + 1],
  [m_jst, x1i - jst_body[2] / 2, jst_yc, jst_floor + jst_body[0] / 2],
  [m_hw, 0, 0, 0],
  [m_mini, (mini_x0 + mini_x1) / 2, mini_yc, mini_z0 + mini_h / 2],
  [m_cap, (cap_x0 + cap_x1) / 2, cap_yc, cap_zc],
  [_v_isl * petg_dens / 1000, 0, 0, roof_t / 2],
  [_v_flr * petg_dens / 1000, cxl, 0, roof_t - low_t / 2],
  [_v_wal * petg_dens / 1000, cxl, 0, (u_top - nose_out) / 2],
  [_v_lid * petg_dens / 1000, cxl, 0, u_top + lid_t / 2],
  [_v_skt * petg_dens / 1000, 0, 0, -nose_out / 2],
  [_v_pst * petg_dens / 1000, _pxm, _pym, (u_top + roof_t) / 2],
  [_v_col * petg_dens / 1000, uln_x, 0, (uln_u0 + roof_t) / 2],
  [_v_crd * petg_dens / 1000, x1i - jst_blk / 2, _cry, roof_t + jst_blk_h / 2]
];
_ones = [for (i = com_items) 1];
com_m = [for (i = com_items) i[0]] * _ones;
com_x = ([for (i = com_items) i[0] * i[1]] * _ones) / com_m;
com_y = ([for (i = com_items) i[0] * i[2]] * _ones) / com_m;
com_u = ([for (i = com_items) i[0] * i[3]] * _ones) / com_m;
com_off = norm([com_x, com_y]);
_n_hw = 8;   // items 0 .. _n_hw - 1 are boards and hardware, the rest is printed
petg_m = ([for (i = [_n_hw : len(com_items) - 1]) com_items[i][0]] * [for (i = [_n_hw : len(com_items) - 1]) 1]);

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
assert(port_side == "+x" || port_side == "-x", "port_side must be \"+x\" or \"-x\"");
assert(usb_side == 1 || usb_side == -1, "usb_side must be 1 or -1");
assert(fpc_len >= fpc_path + fpc_margin,
  "FPC too short for the ESP32 socket (ribbon goes up, along the roof, up past the board end and back): use a longer ribbon, raise esp_dx or reduce esp_sock_end");
assert(esp_x_min - x0i >= esp_chan - 0.01, "gap between the ESP32 end and the wall too small for the ribbon: decrease esp_dx or increase esp_chan");
assert(esp_u0 + esp_pcb_t + esp_comp_h + 1 <= uln_u0, "ESP32 tallest part would touch the driver board");
assert(oy - uln_pilot / 2 >= crop_y - 0.1,
  "driver post pilot would break out on the ESP32 side: ESP32 too wide for this driver hole pattern (esp_w, esp_edge_clr, uln_holes)");
assert(uln_x + ox + uln_post_d / 2 <= x1i - max(usb_pcb[0], jst_blk) - 0.5,
  "driver posts collide with the USB-C breakout or the motor cradle: move the driver away from the ports (uln_x)");
assert(uln_x - lx / 2 >= x0i + 0.3 && uln_x + lx / 2 <= x1i - 0.3 && ly / 2 <= Y_in - 0.3, "driver board does not fit the box");
assert(uln_x - ox - uln_post_d / 2 >= x0i + 0.3, "driver post would touch the wall at the ribbon end: increase uln_x");
assert(Y_in - jst_wall - jst_cross - jst_tail >= crop_y + 0.8, "motor cradle does not fit the strip beside the ESP32");
assert(roof_t + 0.2 + jst_win[1] < u_top - lid_lip_h, "motor window reaches the lid lip: reduce jst_body[0]");
assert(usb_u0 + usb_pcb[2] + usb_shell[1] < esp_top, "USB-C receptacle higher than the ESP32 layer");
assert(mini_pcb[1] + mini_wall_clr + 0.3 <= Y_in - crop_y, "Mini-360 does not fit the strip beside the ESP32");
assert(cap_x1 + 0.3 <= post_pts[3][0] - post_d / 2, "output capacitor would touch the lid post: shorten cap_gap or move the driver (uln_x)");
assert(mini_z0 + mini_h + 1 <= u_top - lid_lip_h, "Mini-360 reaches the lid lip");
assert(com_off <= com_limit, "centre of mass further than com_limit from the optical axis: re-balance esp_dx / uln_x");
assert(corner_r > box_wall, "corner_r must be larger than box_wall");

// ------------------------------------------------------------------ helpers
e = 0.01;

module rrect(w, l, r) {
  offset(r = r) square([w - 2 * r, l - 2 * r], center = true);
}

// box footprint, inset by off (0 = outer face)
module foot(off = 0, r = -1) {
  translate([(x0o + x1o) / 2, 0]) rrect(x1o - x0o - 2 * off, 2 * Yo - 2 * off, r >= 0 ? r : max(corner_r - off, 0.5));
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

module mx() {   // interior layout: ports at +x; "-x" mirrors it (the ring interface and key never move)
  if (s < 0) mirror([1, 0, 0]) children();
  else children();
}

module my(sg) {   // sg < 0: mirror across y = 0
  if (sg < 0) mirror([0, 1, 0]) children();
  else children();
}

// ESP32 seat: four corner pads under the board, two stops at the ribbon end (the wall stops the other end,
// the driver posts stop the long edges); optional M2 posts for boards that have mounting holes
module esp_seat() {
  translate([esp_dx, 0, roof_t - e]) {
    for (sx = [-1, 1], sy = [-1, 1])
      bx(sx * esp_l / 2, sx * (esp_l / 2 - 4), sy * esp_w / 2, sy * (esp_w / 2 - 4), 0, esp_stand + e);
    for (sy = [-1, 1])
      bx(-esp_l / 2 - 0.3, -esp_l / 2 - 1.5, sy * (fw / 2 + 1.0), sy * esp_w / 2, 0, esp_stand + esp_pcb_t * 0.7);
    for (h = esp_holes) translate([h[0], h[1], 0]) cylinder(d = esp_post_d, h = esp_stand + e, $fn = 32);
  }
}

// USB-C breakout seat: two rails under the long edges and a stop at the inner end (the wall holds the receptacle end)
module usb_seat() {
  ux0 = x1i - usb_pcb[0];
  for (sy = [-1, 1])
    bx(ux0, x1i, usb_c[1] + sy * usb_pcb[1] / 2, usb_c[1] + sy * (usb_pcb[1] / 2 - 2.5), roof_t - e, usb_u0);
  bx(ux0 - 1.5, ux0 - 0.3, usb_c[1] - usb_pcb[1] / 2, usb_c[1] + usb_pcb[1] / 2, roof_t - e, usb_u0 + usb_pcb[2] * 0.7 + 0.5);
}

// driver posts on the island floor, cropped to stay off the ESP32 edge (they also stop it sideways)
module uln_columns() {
  for (sx = [-1, 1], sy = [-1, 1])
    intersection() {
      translate([uln_x + sx * ox, sy * oy, roof_t - e]) cylinder(d = uln_post_d, h = uln_u0 - roof_t + e, $fn = 32);
      translate([-100, sy > 0 ? crop_y : -100, 0]) cube([200, 100 - crop_y, 100]);
    }
}

// motor socket cradle in the strip opposite the USB-C: pocket open to the top (header dropped in from above)
// and to the inner side (pins and solder joints)
module jst_cradle() {
  my(-usb_side) difference() {
    bx(x1i - jst_blk, x1i + e, crop_y, Y_in + e, roof_t - e, jst_floor + jst_blk_h);
    bx(x1i - jst_body[2] - 0.4, x1i + 2, jst_y0 - jst_tail, jst_y0 + jst_cross, jst_floor, 100);
  }
}

// Mini-360: two rails under the long edges between the corner pads, stops at both ends and on the inner side
module mini_seat() {
  yo = mini_sg * mini_pcb[1] / 2;
  for (sy = [-1, 1])
    bx(mini_x0 + mini_pad, mini_x1 - mini_pad, mini_yc + sy * mini_pcb[1] / 2, mini_yc + sy * (mini_pcb[1] / 2 - mini_rail_w), roof_t - e, mini_z0);
  for (xs = [[mini_x0 - 1.0, mini_x0 - 0.2], [mini_x1 + 0.2, mini_x1 + 1.0]])
    bx(xs[0], xs[1], mini_yc - 2, mini_yc + 2, roof_t - e, mini_z0 + mini_pcb[2] * 0.7 + 0.5);
  for (xc = [-1, 1])
    bx(mini_x0 + mini_pcb[0] / 2 + xc * 4 - 1, mini_x0 + mini_pcb[0] / 2 + xc * 4 + 1,
       mini_yc - mini_sg * (mini_pcb[1] / 2 + 0.2), mini_yc - mini_sg * (mini_pcb[1] / 2 + 1.0), roof_t - e, mini_z0 + mini_pcb[2] * 0.7 + 0.5);
}

// output capacitor cradle: two blocks with a round trough (the wall holds the outer side)
module cap_saddles() {
  for (xs = [cap_x0, cap_x1 - cap_saddle_l])
    difference() {
      bx(xs, xs + cap_saddle_l, cap_yc - cap_dc / 2 - mini_sg * 0.2, cap_yc + mini_sg * (cap_dc / 2 + 1.5), roof_t - e, cap_zc + 1);
      translate([xs - 1, cap_yc, cap_zc]) rotate([0, 90, 0]) cylinder(d = cap_dc, h = cap_saddle_l + 2, $fn = 48);
    }
}

module window_cut_jst() {   // through the end wall (and the lip), at the header
  translate([x1i - 1, jst_yc - jst_win[0] / 2, roof_t + 0.2]) cube([box_wall + lid_lip_t + 3, jst_win[0], jst_win[1]]);
}

module window_cut_usb(x_from) {
  translate([x_from, usb_c[1] - (usb_shell[0] + usb_clr) / 2, usb_u0 + usb_pcb[2] + usb_shell[1] / 2 - (usb_shell[1] + usb_clr) / 2])
    cube([x1o - x_from + 1, usb_shell[0] + usb_clr, usb_shell[1] + usb_clr]);
}

module module_base() {
  difference() {
    union() {
      // island roof (carries the ring interface), skirt wall, nose, labyrinth flange, key tab: fixed to the optical axis
      cylinder(r = R_isl, h = roof_t);
      translate([0, 0, -nose_out]) linear_extrude(nose_out + e)
        difference() { circle(r = R_isl); circle(r = R_sk_i); }
      translate([0, 0, -nose_out]) cylinder(d = nose_d, h = nose_out + e);
      rotate_extrude()
        polygon([[0, e], [R_fl, e], [R_fl, -1.0], [nose_d / 2, -1.0 - (R_fl - nose_d / 2)],
                 [0, -1.0 - (R_fl - nose_d / 2)]]);
      rotate(key_angle) rotate([90, 0, 0]) linear_extrude(height = key_tab_w, center = true)
        polygon([[R_sk_i + 0.3, e], [tab_in, e], [tab_in, -2.0], [R_sk_i + 0.3, -2.0 - (R_sk_i + 0.3 - tab_in)]]);
      mx() {
        // floor of the box around the island (top flush with the island roof)
        translate([0, 0, roof_t - low_t]) linear_extrude(low_t) foot(0);
        // walls reach the bed, so the floor bridges between skirt and walls
        translate([0, 0, -nose_out]) linear_extrude(u_top + nose_out)
          difference() { foot(0); foot(box_wall, cav_r); }
        // lid screw posts (sit on the floor)
        for (p = post_pts) translate([p[0], p[1], roof_t - low_t - e]) cylinder(d = post_d, h = u_top - roof_t + low_t + e, $fn = 32);
        uln_columns();
        esp_seat();
        usb_seat();
        jst_cradle();
        mini_seat();
        cap_saddles();
      }
    }
    // OV5640: square pocket open at the nose end, connector zone, FPC slot up through the island, bend relief
    translate([-pocket_w / 2, -pocket_w / 2, -nose_out - e]) cube([pocket_w, pocket_w, u_pcb_back + nose_out + e]);
    translate([-fpc_slot_t / 2 - 1, -fw / 2, u_pcb_back - e]) cube([fpc_slot_t + 2, fw, conn_h + e]);
    translate([-fpc_slot_t / 2, -fw / 2, u_pcb_back + conn_h - e]) cube([fpc_slot_t, fw, roof_t - (u_pcb_back + conn_h) + 2 * e]);
    mx() translate([-fpc_slot_t / 2 - fpc_relief, -fw / 2, roof_t - fpc_groove_d]) cube([fpc_relief + fpc_slot_t, fw, fpc_groove_d + e]);
    // roof groove that takes the ring's light lip (labyrinth)
    rotate_extrude() polygon([[R_fl, -e], [R_gw, -e], [R_gw, groove_up], [R_fl, groove_up]]);
    // ball pockets (balls are pressed in from below up to the pocket ceiling, then glued)
    for (a = ball_angles) rotate(a) translate([seat_r, 0, -e])
      cylinder(d = ball_d + ball_clr, h = ball_d - ball_protrusion + e, $fn = 32);
    // magnet pockets (magnets are pressed in from below up to the pocket ceiling, then glued)
    for (a = mag_angles) rotate(a) translate([mag_r, 0, -e])
      cylinder(d = mag_d + mag_clr, h = mag_t + mag_recess + e, $fn = 48);
    mx() {
      // lid screw pilot holes
      for (p = post_pts) translate([p[0], p[1], u_top - 8]) cylinder(d = m2_pilot, h = 9, $fn = 24);
      // driver pilot holes (M3 self tapping, 5 mm deep)
      for (sx = [-1, 1], sy = [-1, 1])
        translate([uln_x + sx * ox, sy * oy, uln_u0 - 5]) cylinder(d = uln_pilot, h = 6, $fn = 24);
      // ESP32 mounting posts: M2 pilot, kept inside the post
      for (h = esp_holes) translate([esp_dx + h[0], h[1], roof_t + esp_stand - 3]) cylinder(d = m2_pilot, h = 4, $fn = 24);
      // USB-C window and motor window in the end wall
      window_cut_usb(x1i - 1);
      window_cut_jst();
      // wall vents beside the ESP32, on the motor socket side
      for (i = [0 : wall_vent_n - 1])
        translate([wall_vent_x0 + i * wall_vent_pitch, -usb_side * (Y_in - 1), roof_t + 3]) cube([vent_slot[0] - 0.4, box_wall + 2, 8]);
      // antenna hole (end wall, board centre line)
      if (antenna_hole)
        translate([x1i - 1, 0, esp_face + 1]) rotate([0, 90, 0]) cylinder(d = antenna_d, h = box_wall + 2, $fn = 32);
    }
  }
}

// ------------------------------------------------------------------ lid (u frame)
module module_lid() {
  mx() difference() {
    union() {
      translate([0, 0, u_top]) linear_extrude(lid_t) foot(0);
      // lip inside the walls
      difference() {
        translate([0, 0, u_top - lid_lip_h]) linear_extrude(lid_lip_h + e)
          difference() { foot(box_wall + lid_clr, cav_r); foot(box_wall + lid_clr + lid_lip_t, max(cav_r - lid_lip_t, 0.3)); }
        for (p = post_pts) translate([p[0], p[1], u_top - lid_lip_h - 1]) cylinder(d = post_d + 0.6, h = lid_lip_h + 2, $fn = 32);
      }
    }
    for (p = post_pts) translate([p[0], p[1], u_top - 1]) cylinder(d = m2_clear, h = lid_t + 2, $fn = 24);
    // vent slots over the boards
    for (i = [0 : vent_n - 1])
      translate([vent_x0 + i * vent_pitch - vent_slot[0] / 2, -vent_slot[1] / 2, u_top - 1]) cube([vent_slot[0], vent_slot[1], lid_t + 2]);
    // USB-C window reaches into the lip too
    window_cut_usb(x1i - 3);
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

module fpc_ghost() {      // layout frame: the folded ribbon, 0.3 mm thick
  t = 0.3;
  color("darkorange") {
    bx(-t / 2, t / 2, -fpc_w / 2, fpc_w / 2, u_pcb_back, roof_t + 0.2 + t);
    bx(x_ch - t / 2, t / 2, -fpc_w / 2, fpc_w / 2, roof_t + 0.2, roof_t + 0.2 + t);
    bx(x_ch - t / 2, x_ch + t / 2, -fpc_w / 2, fpc_w / 2, roof_t + 0.2, esp_face + 1 + t);
    bx(x_ch - t / 2, x_sock, -fpc_w / 2, fpc_w / 2, esp_face + 1, esp_face + 1 + t);
  }
}

module inner_ghost() {    // module frame: boards, header, ribbon (mirrored with the layout)
  mx() {
    // ESP32-S3 CAM: module near the port end, camera socket at the ribbon end
    color("seagreen") translate([esp_dx - esp_l / 2, -esp_w / 2, esp_u0]) cube([esp_l, esp_w, esp_pcb_t]);
    color("dimgray") translate([esp_x_max - 24, -9, esp_face]) cube([18, 18, 3.2]);
    color("black") translate([x_sock - 2, -6, esp_face]) cube([4, 12, 2]);
    // driver, turned 90 degrees
    if (driver == "board") {
      color("steelblue") translate([uln_x - lx / 2, -ly / 2, uln_u0]) cube([lx, ly, uln_pcb_t]);
      color("black") translate([uln_x - 4, -10, uln_u0 + uln_pcb_t]) cube([8, 20, 4.5]);
      color("white") translate([uln_x - 12, -ly / 2 + 2, uln_u0 + uln_pcb_t]) cube([8, 15, 6.5]);
    } else {
      color("steelblue") translate([uln_x - lx / 2, -ly / 2, uln_u0]) cube([lx, ly, uln_pcb_t]);
      color("black") translate([uln_x - 3.2, -9.9, uln_u0 + uln_pcb_t]) cube([6.4, 19.8, 4.5]);
    }
    // USB-C breakout
    color("goldenrod") translate([usb_c[0] - usb_pcb[0] / 2, usb_c[1] - usb_pcb[1] / 2, usb_u0]) cube(usb_pcb);
    color("silver") translate([usb_c[0] + usb_pcb[0] / 2 - 7, usb_c[1] - usb_shell[0] / 2, usb_u0 + usb_pcb[2]])
      cube([7, usb_shell[0], usb_shell[1]]);
    // JST-XH header standing on its edge in the cradle
    color("ivory") translate([x1i - jst_body[2], jst_yc - jst_body[1] / 2, jst_floor]) cube([jst_body[2], jst_body[1], jst_body[0]]);
    fpc_ghost();
    // Mini-360 (pads at the corners, inductor, trim pot) and the output capacitor
    color("firebrick") translate([mini_x0, mini_yc - mini_pcb[1] / 2, mini_z0]) cube(mini_pcb);
    color("dimgray") translate([mini_x0 + 3.5, mini_yc - 3, mini_z0 + mini_pcb[2]]) cube([6, 6, mini_h - mini_pcb[2]]);
    color("royalblue") translate([mini_x1 - 6, mini_yc - 1.7, mini_z0 + mini_pcb[2]]) cube([3.4, 3.4, 2.5]);
    color("navy") translate([cap_x0, cap_yc, cap_zc]) rotate([0, 90, 0]) cylinder(d = cap_d, h = cap_l, $fn = 48);
  }
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
  sec() inner_ghost();
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
echo(str("module mass about ", com_m, " g (printed ", petg_m, " g at ", petg_dens, " g/cm3, boards and hardware ", com_m - petg_m, " g)"));
echo(str("centre of mass: ", com_off, " mm from the optical axis (x ", com_x * s, ", y ", com_y, "), ", com_u,
         " mm above the seat plane; limit ", com_limit, " mm"));

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
  color("darkgreen") projection(cut = true) cut2d() translate([0, 0, z_mod0]) inner_ghost();
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
else if (part == "clash_in") {   // developer check: boards, header and ribbon against the printed base and lid (should render empty)
  intersection() {
    union() { module_base(); module_lid(); }
    translate([0, 0, 0.05]) union() { inner_ghost(); ov5640_ghost(); }   // lifted 0.05: resting contacts are not clashes
  }
}
else assert(false, "part must be \"ring\", \"module_base\", \"module_lid\", \"assembly\", \"exploded\", \"clash\" or \"clash_in\"");
