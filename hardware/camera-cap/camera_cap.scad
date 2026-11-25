// OAE camera cap: OV5640 on the eyepiece, afocal (design: plans/camera-cap/design.md)
//
// Two printed parts:
//   cap  : sleeve that slides over the eyepiece housing, 3 radial M3 thumbscrews (120 deg) grip and
//          centre it, closed light-tight tube above the rim, top socket for the camera head.
//   head : puck that friction-fits into the socket (+ one M3 set screw), holds the OV5640 module
//          lens down on the axis, FPC slot, platform for the ESP32-S3 CAM board.
//
// Usage (see README.md):
//   openscad -o cap.stl  -D 'part="cap"'  -D eyepiece=12.5 -D print_pose=true camera_cap.scad
//   openscad -o head.stl -D 'part="head"' -D print_pose=true camera_cap.scad
//
// Z frame of the cap: z = 0 is the open (bottom) end of the sleeve, the eyepiece housing top edge
// (rim) is at z = grip_len. All lengths in mm.

// ------------------------------------------------------------------ what to show
part = "assembly";      // "cap" | "head" | "assembly"
eyepiece = 12.5;        // 25 | 12.5 | 8 : selects the row of eyepiece_table below
section = false;        // true: cut away the y < 0 half (look from -y to see the cross-section)
print_pose = false;     // true: cap/head in print orientation (head flipped, platform on the bed)

$fn = 96;               // round parts; small holes use $fn = 24

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
// Rim to front of the camera lens (nominal head height). Negative: the nose enters the eyepiece.
gap = eye_relief - lens_recess - lens_stop_offset;
z_adjust = 4;           // head travel in the socket (set screw locks it); nominal = mid travel
head_lift = z_adjust / 2; // assembly only: head height above the socket floor, 0 .. z_adjust

// ------------------------------------------------------------------ cap (sleeve + tube + socket)
sleeve_clr = 1.0;       // sleeve ID = housing_od + this (the thumbscrews take up the play)
wall = 2;               // wall thickness of sleeve, tube and socket
grip_len = 15;          // sleeve length below the rim (also the z of the rim)
rim_id = 20;            // opening of the eyepiece housing at the rim (nose must fit if gap < 0)
tube_id = rim_id;       // bore of the light tube between rim and socket
floor_t = 2;            // minimum thickness of the plate between rim and socket floor
fit = 0.15;             // radial clearance head puck to socket bore (lower = tighter friction)
min_engage = 6;         // minimum puck length left in the socket at full raise
seat_chamfer = 0.6;     // lead-in / elephant-foot chamfer at the open end of the sleeve

// M3 hardware (thumbscrews, socket set screw)
m3_clear = 3.4;         // clearance hole for M3
nut_af = 5.5;           // M3 nut across flats
nut_t = 2.4;            // M3 nut thickness
nut_clr = 0.3;          // clearance around the nut in its pocket
nut_back = 2.0;         // wall behind the nut pocket
boss_wall = 2.4;        // wall either side of the nut pocket
thumb_z = grip_len / 2; // height of the thumbscrew axes (above the sleeve bottom)

// ------------------------------------------------------------------ camera head
head_d = 30;            // puck diameter (socket bore = head_d + 2 * fit)
puck_h = 12;            // puck length (also the socket depth)
plat_t = 3;             // platform thickness
nose_d = 15;            // nose outer diameter (must fit inside rim_id when the nose is below the rim)
nose_len = 6;           // nose length below the puck; longer = lens reaches deeper (gap < 0)
nose_wall_min = 1.2;    // minimum nose wall around the module pocket
lens_in = 0.5;          // lens front recessed this far inside the nose end

// OV5640 24-pin module (measure yours)
pcb_w = 8.5;            // sensor PCB width (square)
pcb_t = 1.0;            // PCB thickness
holder_w = 8;           // lens holder width (square)
barrel_d = 8;           // lens barrel diameter (only drawn in the assembly)
module_h = 6;           // lens front to PCB front face (holder + lens height)
fpc_w = 8.5;            // FPC ribbon width
pocket_clr = 0.4;       // module pocket clearance (total)
conn_y = 4;             // connector zone depth (y) above the PCB
conn_h = 2.5;           // connector zone height above the PCB
fpc_slot_t = 2;         // FPC slot thickness (y)
fpc_groove_d = 1;       // depth of the FPC groove across the platform top
retain = true;          // two M2 grub screws pinch the PCB edges (set false to glue / tape)
retain_hole_d = 1.7;    // tap hole for M2 (self-tapping into PETG)

// ESP32-S3 CAM board on the platform
esp_w = 27;             // board width (x)
esp_l = 50;             // board length (y)
tie_slot = [2, 4];      // zip-tie slot size [x, y]
tie_margin = 2.5;       // slot centre outside the board edge
plat_margin = 2;        // platform margin beyond the slots / board ends

// ------------------------------------------------------------------ derived
R_i    = (housing_od + sleeve_clr) / 2;   // sleeve bore radius
R_go   = R_i + wall;                      // sleeve outer radius
R_tb   = tube_id / 2;                     // tube bore radius
R_t    = R_tb + wall;                     // tube outer radius
R_so   = head_d / 2 + fit;                // socket bore radius
R_s    = R_so + wall;                     // socket outer radius
z_rim  = grip_len;
// Socket floor: the puck bottom rests here at the lowest head position.
z_sf   = z_rim + gap - lens_in - z_adjust / 2 + nose_len;
z_gt   = z_rim + floor_t;                 // top of the sleeve cylinder
z_top  = z_sf + puck_h;                   // top of the socket
z_c0   = z_sf - (R_so - R_t) - 1.5;       // start of the 45 deg cone under the socket
nut_depth = nut_t + nut_clr + 0.1;
nut_ac = nut_af / cos(30);                // across corners
pocket_w = max(pcb_w, holder_w) + pocket_clr;
z_pcb_back = lens_in + module_h + pcb_t;  // head frame (z = 0 at nose end)
plat_w = esp_w + 2 * (tie_margin + tie_slot[0] / 2 + plat_margin);
plat_l = esp_l + 2 * plat_margin;
head_h = nose_len + puck_h + plat_t;
nose_low = gap - lens_in - z_adjust / 2;  // nose end height vs the rim at the lowest head position

// ------------------------------------------------------------------ sanity checks
assert(nose_d + 1 <= tube_id, "nose_d must be at least 1 mm smaller than tube_id");
assert(nose_low >= 0 || nose_d + 1 <= rim_id,
  "nose_d >= rim_id: the nose goes below the rim (gap < 0 or small gap) but does not fit the eyepiece opening");
assert(nose_low >= -lens_recess + 0.5,
  "nose would touch the eye lens: eye_relief too small for lens_in, z_adjust and lens_recess");
assert(nose_d >= sqrt(2) * pocket_w + 2 * nose_wall_min,
  "nose_d too small for the square module pocket plus nose_wall_min");
assert(z_sf - z_rim >= floor_t,
  "socket floor too close to the rim: increase nose_len (or gap), or decrease z_adjust");
assert(puck_h - z_adjust >= min_engage, "puck too short for z_adjust: increase puck_h");
assert(R_so - R_tb >= 2, "socket floor shoulder under 2 mm wide: reduce tube_id or increase head_d");
assert((housing_od - rim_id) / 2 >= 2, "rim_id too large for housing_od: no seat left for the rim");
assert(nose_d + 2 * nose_wall_min <= head_d, "head_d too small for nose_d");
assert(plat_w >= head_d, "platform narrower than the puck");
assert(z_pcb_back + conn_h < nose_len + puck_h, "module pocket does not fit in the puck: increase puck_h");
assert(lens_in >= 0 && head_lift >= 0 && head_lift <= z_adjust, "head_lift must be within 0 .. z_adjust");

// ------------------------------------------------------------------ helpers
e = 0.01;

module rrect(w, l, r) {
  offset(r = r) square([w - 2 * r, l - 2 * r], center = true);
}

// Solid nut boss along +x. R: radius of the surface it sits on. Profile is (x, z).
module nut_boss(R, z_lo, z_hi, slope_z = undef) {
  bw = nut_af + nut_clr + 2 * boss_wall;
  x0 = R - 0.8;
  x1 = R + nut_depth + nut_back;
  rotate([90, 0, 0]) linear_extrude(height = bw, center = true)
    if (slope_z == undef)
      polygon([[x0, z_lo], [x1, z_lo], [x1, z_hi], [x0, z_hi]]);
    else // 45 deg underside so it prints without support
      polygon([[x0, z_hi], [x1, z_hi], [x1, slope_z],
               [x0, slope_z - (x1 - x0)]]);
}

// Cuts for one screw+nut along +x: nut slot open at the top, clearance hole through everything.
module nut_cut(R_wall_in, R, z_ax, z_hi) {
  // nut slot (flats parallel to the slot walls), open upward so the nut drops in
  translate([R, -(nut_af + nut_clr) / 2, z_ax - nut_ac / 2 - 0.1])
    cube([nut_depth, nut_af + nut_clr, z_hi - (z_ax - nut_ac / 2 - 0.1) + 1]);
  // screw hole
  translate([R_wall_in - 2, 0, z_ax]) rotate([0, 90, 0])
    cylinder(d = m3_clear, h = R + nut_depth + nut_back + 4 - (R_wall_in - 2), $fn = 24);
}

// ------------------------------------------------------------------ cap
set_angle = 0;                       // socket set screw
thumb_angles = [90, 210, 330];       // 120 deg apart
z_set = z_sf + (z_adjust + puck_h) / 2;   // set screw height: inside the puck at any head height

module cap() {
  difference() {
    union() {
      cylinder(r = R_go, h = z_gt);                       // grip sleeve
      cylinder(r = R_t, h = z_c0 + e);                    // light tube
      rotate_extrude()                                    // cone (45 deg) + socket
        polygon([[0, z_c0], [R_t, z_c0], [R_s, z_c0 + (R_s - R_t)], [R_s, z_top], [0, z_top]]);
      for (a = thumb_angles) rotate(a) nut_boss(R_go, 0, z_gt);
      rotate(set_angle)
        nut_boss(R_s, 0, z_top, slope_z = z_set - nut_ac / 2 - 1.5);
    }
    // bore: sleeve (with lead-in) / rim shoulder / tube / socket floor / socket
    rotate_extrude()
      polygon([[0, -e], [R_i + seat_chamfer, -e], [R_i + seat_chamfer, 0], [R_i, seat_chamfer],
               [R_i, z_rim], [R_tb, z_rim], [R_tb, z_sf], [R_so, z_sf], [R_so, z_top + e],
               [0, z_top + e]]);
    for (a = thumb_angles) rotate(a) nut_cut(R_i, R_go, thumb_z, z_gt);
    rotate(set_angle) nut_cut(R_so, R_s, z_set, z_top);
  }
}

// ------------------------------------------------------------------ head (z = 0 at nose end)
module head() {
  fw = fpc_w + 0.6;
  z_pl = nose_len + puck_h;            // platform underside
  z_pt = z_pl + plat_t;                // platform top
  difference() {
    union() {
      cylinder(d = nose_d, h = nose_len + e);
      translate([0, 0, nose_len]) cylinder(d = head_d, h = puck_h + e);
      translate([0, 0, z_pl]) linear_extrude(plat_t) rrect(plat_w, plat_l, 3);
    }
    // square module pocket, open at the nose end
    translate([-pocket_w / 2, -pocket_w / 2, -e]) cube([pocket_w, pocket_w, z_pcb_back + e]);
    // connector zone above the PCB, then the FPC slot up through the platform
    translate([-fw / 2, -conn_y / 2, z_pcb_back - e]) cube([fw, conn_y, conn_h + e]);
    translate([-fw / 2, -fpc_slot_t / 2, z_pcb_back + conn_h - e])
      cube([fw, fpc_slot_t, z_pt - (z_pcb_back + conn_h) + 2 * e]);
    // FPC groove across the platform top toward +y
    translate([-fw / 2, 0, z_pt - fpc_groove_d]) cube([fw, plat_l / 2 + e, fpc_groove_d + e]);
    // M2 grub screws through the puck, pinching the PCB edges (along y)
    if (retain)
      translate([0, 0, lens_in + module_h + pcb_t / 2]) rotate([90, 0, 0])
        cylinder(d = retain_hole_d, h = head_d + 2, center = true, $fn = 24);
    // zip-tie slots
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * (esp_w / 2 + tie_margin), sy * esp_l / 4, z_pl - e])
        translate([-tie_slot[0] / 2, -tie_slot[1] / 2, 0]) cube([tie_slot[0], tie_slot[1], plat_t + 2 * e]);
  }
}

// ------------------------------------------------------------------ assembly extras
module eyepiece_ghost() {
  len_vis = 25;
  color("silver", 0.5) difference() {
    translate([0, 0, -len_vis]) cylinder(d = housing_od, h = len_vis + z_rim);
    translate([0, 0, z_rim - lens_recess]) cylinder(d = rim_id, h = lens_recess + e);
  }
  color("skyblue", 0.8) translate([0, 0, z_rim - lens_recess - 1]) cylinder(d = rim_id - 1, h = 1);
}

module ov5640_ghost() {   // head frame, z = 0 at nose end
  color("limegreen") translate([-pcb_w / 2, -pcb_w / 2, lens_in + module_h])
    cube([pcb_w, pcb_w, pcb_t]);
  color("dimgray") translate([-holder_w / 2, -holder_w / 2, lens_in + 3])
    cube([holder_w, holder_w, module_h - 3]);
  color("black") translate([0, 0, lens_in]) cylinder(d = barrel_d, h = 3);
}

module sec() {   // section=true keeps only y >= 0 (cut applied per part so colours survive)
  if (section)
    intersection() {
      children();
      translate([-200, 0, -200]) cube([400, 400, 400]);
    }
  else children();
}

module assembly() {
  sec() color("orange") cap();
  translate([0, 0, z_sf - nose_len + head_lift]) {
    sec() color("steelblue") head();
    sec() ov5640_ghost();
  }
  sec() eyepiece_ghost();
  // marker: nominal lens front plane (rim + gap)
  sec() color("red") translate([0, 0, z_rim + gap - 0.05]) cylinder(d = 1.2 * nose_d, h = 0.05);
}

module cap_view() { sec() cap(); }

module head_view() {
  if (print_pose) translate([0, 0, head_h]) rotate([180, 0, 0]) sec() head();
  else sec() head();
}

if (part == "cap") cap_view();
else if (part == "head") head_view();
else if (part == "assembly") assembly();
else assert(false, "part must be \"cap\", \"head\" or \"assembly\"");
