// OAE camera arm: swing-away OV5640 mount behind the eyepiece, afocal (option 2 next to ../camera-cap)
//
// Printed parts (see README.md for orientation, hardware and assembly):
//   collar  : split ring that clamps the Barlow body (M3 pinch), with a long tab carrying the vertical
//             hinge (M3 axle, captive nut), two magnet pockets (0 / 90 deg) and the 0 deg hard stop.
//   arm     : foot plate on the hinge, upright post, horizontal boom, and a vertical dovetail RAIL
//             hanging from the boom end (Z travel). ESP32-S3 CAM plate on the post. Engraved Z marks.
//   cup     : dovetail GROOVE block that rides on the rail (M3 lock thumbscrew), shell with three nylon
//             M3 centring thumbscrews, light-shield skirt towards the eyepiece.
//   carrier : puck that floats in the cup (+-xy_travel), holds the OV5640 module lens down, FPC slot.
//
// Usage:
//   openscad -o collar.stl  -D 'part="collar"'  -D print_pose=true camera_arm.scad
//   openscad -o arm.stl     -D 'part="arm"'     -D print_pose=true camera_arm.scad
//   openscad -o cup.stl     -D 'part="cup"'     -D print_pose=true camera_arm.scad
//   openscad -o carrier.stl -D 'part="carrier"' -D print_pose=true camera_arm.scad
//
// Frames. Assembly: z = 0 is the TOP face of the collar, z up, the optical axis is the z axis.
// "Hinge frame" (used by every part below): the hinge pivot P is on the +x axis at x = R_piv, the boom
// reaches from the post to the axis along -x, the cup sits on the axis. The assembly rotates the hinge
// frame to the side opposite the focus motor. Cup frame: z = 0 is the front glass plane of the camera
// lens, the axis is z. All lengths in mm.

// ------------------------------------------------------------------ what to show
part = "assembly";      // "collar" | "arm" | "cup" | "carrier" | "assembly" ("none": render nothing, for include)
eyepiece = 12.5;        // 25 | 12.5 | 8 : selects the row of eyepiece_table (assembly pose of the cup)
swing = 0;              // assembly pose: 0 = camera over the eyepiece, 90 = swung aside
section = false;        // true: cut away the y < 0 half (look from -y to see the cross-section)
print_pose = false;     // true: parts in print orientation (see README)
show_ghosts = true;     // assembly: Barlow, eyepiece, focuser and focus-motor stand-ins

$fn = 96;               // round parts; small holes use $fn = 24

// ------------------------------------------------------------------ Barlow, eyepieces, optics
barlow_od = 36;               // Barlow body outer diameter where the collar clamps (measure it)
barlow_above_collar = 30;     // Barlow top edge above the collar top face (measure it, collar fixed first)
motor_angle = 0;              // assembly: where the focus motor / belt is (deg, seen from above)
                              // the hinge is placed on the opposite side (motor_angle + 180)

// Measure your eyepieces and edit the row (or override one value with -D).
eyepiece_table = [
  // [eyepiece mm, housing OD, eye relief WITH Barlow, rim recess, eyepiece height above the Barlow top]
  [25,   33, 18, 3, 35],
  [12.5, 33,  9, 3, 28],
  [8,    33,  5, 3, 24],
];
_rows = [for (r = eyepiece_table) if (r[0] == eyepiece) r];
assert(len(_rows) == 1, "eyepiece must be one of the first column of eyepiece_table: 25, 12.5, 8");
_row = _rows[0];

rim_id = 20;                  // opening of the eyepiece housing at the rim (nose must fit if it dips below)
lens_stop_offset = 1.5;       // camera aperture stop sits this far behind the front glass of the lens
z_margin = 2;                 // Z travel kept beyond the lowest / highest default mark

function gap_of(r)    = r[2] - r[3] - lens_stop_offset;           // rim to lens front: as in camera_cap
function z_rim_of(r)  = barlow_above_collar + r[4];               // eyepiece rim height over the collar top
function z_lens_of(r) = z_rim_of(r) + gap_of(r);                  // nominal lens front height

z_lens_all = [for (r = eyepiece_table) z_lens_of(r)];
c_min = min(z_lens_all) - z_margin;       // lowest cup position (lens front z), mm over the collar top
c_max = max(z_lens_all) + z_margin;       // highest cup position
travel = c_max - c_min;                   // Z travel of the cup on the rail
cup_trim = 0;                             // assembly: cup height relative to the nominal mark of `eyepiece`
cup_z = z_lens_of(_row) + cup_trim;
od_max = max([for (r = eyepiece_table) r[1]]);

// ------------------------------------------------------------------ hardware
m3_clear = 3.4;               // clearance hole for M3
nut_af = 5.5;                 // M3 nut across flats
nut_t = 2.4;                  // M3 nut thickness
nut_clr = 0.3;                // clearance around the nut in its pocket
nut_back = 2.0;               // wall behind a nut pocket
boss_wall = 2.4;              // wall either side of a nut pocket
hinge_bolt = 3;               // 3 (M3) or 4 (M4) hinge axle
magnet_d = 6;                 // neodymium disc magnets (detents)
magnet_t = 3;
magnet_clr = 0.3;             // pocket diameter = magnet_d + this

// ------------------------------------------------------------------ collar
collar_fit = 0.3;             // bore radius = barlow_od / 2 + this
collar_wall = 4.5;            // ring wall
collar_h = 20;                // ring height (z = -collar_h .. 0)
slit_w = 1.2;                 // pinch slit
ear_len = 14;                 // pinch ear length beyond the ring
ear_wall = 3.0;               // wall between the slit and the pinch nut pocket
hinge_pos = 90;               // deg from the pinch ear to the hinge boss around the collar (ccw from above)
tab_root_hw = 10;             // half-width of the hinge tab where it leaves the ring

// ------------------------------------------------------------------ hinge (all in the hinge frame, P = pivot)
swing_dir = 1;                // +1 swings the arm ccw (seen from above), -1 cw (mirrors the hinge details)
hub_h = 0.6;                  // bearing hub / ring height: the gap between foot and tab (magnet face gap)
foot_t = 6;                   // foot plate thickness
ear_w = 6;                    // hard stop lug on the foot, width
stop_w = 5;                   // hard stop block on the tab, width
stop_len = 5;                 // hard stop block radial length
stop_gap = 0.15;              // clearance lug to stop at 0 deg
detent_preload = 4;           // deg: the 0 deg tab magnet sits this far past the foot magnet, towards the stop
pivot_clear = 10;             // P distance beyond the post outer face

// ------------------------------------------------------------------ arm
Y0 = 12.5;                    // arm half thickness (y), also the rail head half width; everything is flush
                              // with the y = -Y0 plane, which is the print bed
post_d = 14;                  // post depth (x)
beam_h = 12;                  // boom height (z)
stem_hw = 5;                  // rail stem half width
head_flat = 3;                // rail head flat length (gives the first print layers some width)
dv_clr = 0.35;                // dovetail clearance (groove vs rail)
neck_len = 6;                 // straight neck of the groove (the lock screw bears here)
groove_back = 2.5;            // cup block wall behind the groove floor
stop_head = 5.5;              // travel stop bolt head (M3) diameter
stop_bolt_below = 3.5;        // stop bolt axis below the cup block bottom at the lowest position
plate_t = 3;                  // ESP32 plate thickness (flush with the arm's y = -Y0 face)
plate_z0 = 14;                // ESP32 plate bottom height over the collar top
engrave = 0.6;                // depth of engraved Z marks
mark_w = 1.0;                 // width of an engraved mark line
mark_text = 2.8;              // mark label height

// ------------------------------------------------------------------ cup + carrier
carrier_d = 30;               // puck diameter
puck_h = 12;                  // puck length
nose_d = 15;                  // nose outer diameter
nose_len = 6;                 // nose length below the puck
nose_wall_min = 1.2;          // minimum nose wall around the module pocket
lens_in = 0.5;                // lens front recessed this far inside the nose end
xy_travel = 2;                // X/Y centring range (+-)
bore_clr = 0.4;               // extra bore clearance
shell_wall = 2.6;             // cup shell wall around the bore
shell_over = 2;               // shell wall above the carrier top
floor_t = 2;                  // cup floor thickness
skirt_len = 3;                // light-shield skirt length below the floor (stays above the nose end)
skirt_gap = 1;                // skirt bore radial clearance to the widest eyepiece housing
cup_screws = [60, 180, 300];  // angles of the three nylon centring screws (keeps the dovetail block clear)
fpc_angle = 240;              // direction of the FPC notch / groove (deg)
fpc_notch_depth = 4;          // FPC notch depth below the shell top

// OV5640 24-pin module (same as camera_cap, measure yours)
pcb_w = 8.5;                  // sensor PCB width (square)
pcb_t = 1.0;                  // PCB thickness
holder_w = 8;                 // lens holder width (square)
barrel_d = 8;                 // lens barrel diameter (only drawn in the assembly)
module_h = 6;                 // lens front to PCB front face
fpc_w = 8.5;                  // FPC ribbon width
pocket_clr = 0.4;             // module pocket clearance (total)
conn_y = 4;                   // connector zone depth (y) above the PCB
conn_h = 2.5;                 // connector zone height above the PCB
fpc_slot_t = 2;               // FPC slot thickness
fpc_groove_d = 1;             // FPC groove depth across the carrier top
retain = true;                // two M2 grub screws pinch the PCB edges (false: glue / tape)
retain_hole_d = 1.7;          // tap hole for M2

// ESP32-S3 CAM board on the arm plate
esp_w = 27;                   // board width (x on the plate)
esp_l = 50;                   // board length (z on the plate)
tie_slot = [2, 4];            // zip-tie slot size [width, length]
tie_margin = 2.5;             // slot centre outside the board edge
plat_margin = 2;              // plate margin beyond the slots / board ends

// ------------------------------------------------------------------ derived: cup (cup frame, z = 0 lens front)
R_b      = (carrier_d + 2 * xy_travel + bore_clr) / 2;   // cup bore radius
R_cup    = R_b + shell_wall;                             // cup shell outer radius
z_pb     = -lens_in + nose_len;                          // carrier puck bottom = cup floor top
z_fl     = z_pb - floor_t;                               // floor underside
z_sk0    = z_fl - skirt_len;                             // skirt bottom = lowest point of the cup shell
z_ct     = z_pb + puck_h + shell_over;                   // shell / dovetail block top
R_hole   = nose_d / 2 + xy_travel + 0.6;                 // floor hole for the nose
R_sk     = (od_max + 2 * skirt_gap) / 2;                 // skirt bore radius
z_ts     = z_pb + puck_h / 2;                            // centring screw height

// dovetail (hinge frame x, same in cup and arm), rail head wide end faces the cup
flare_d  = Y0 - stem_hw;                                 // 45 deg flare depth
x_gf     = R_cup + groove_back;                          // groove floor
x_h0     = x_gf + dv_clr;                                // rail head wide end
x_h1     = x_h0 + head_flat + flare_d;                   // rail head narrow end = stem start
x_cf     = x_h1 + neck_len + dv_clr;                     // groove opening face (cup block outer face)
x_bb0    = x_cf + 1;                                     // post inner face
x_lock   = (x_h1 + x_cf) / 2;                            // lock screw / stop bolt axis (in the neck)
blk_hw   = Y0 + dv_clr + 3;                              // cup block half width
z_lock   = (z_sk0 + z_ct) / 2;                           // lock screw height in the cup frame

// ------------------------------------------------------------------ derived: arm / hinge (assembly z)
z_bu     = c_max + z_ct;                                 // boom underside = top stop of the cup block
z_top    = z_bu + beam_h;                                // top of arm
z_stop   = c_min + z_sk0 - stop_bolt_below;              // travel stop bolt axis
z_rb     = z_stop - 6;                                   // rail bottom
x_post1  = x_bb0 + post_d;                               // post outer face
R_piv    = x_post1 + pivot_clear;                        // pivot distance from the optical axis
hinge_hole = hinge_bolt + 0.4;
hinge_nut_af = hinge_bolt == 4 ? 7 : 5.5;
hinge_nut_t  = hinge_bolt == 4 ? 3.2 : 2.4;
hub_r    = hinge_hole / 2 + 1.5;
mag_d_p  = magnet_d + magnet_clr;
r_mag    = hub_r + 0.8 + mag_d_p / 2;                    // magnet pocket centre radius around P
ring_ri  = r_mag + mag_d_p / 2 + 0.5;                    // raised bearing ring on the tab
ring_ro  = ring_ri + 1.2;
foot_r   = 13.5;
stop_r0  = foot_r + 0.3;                                 // stop block starts outside the foot
ear_r    = stop_r0 + stop_len;                           // lug tip radius
ear_ang  = swing_dir * 45;                               // lug angle on the foot (45 deg: prints without support)
mag_ang0 = 180 - swing_dir * 45;                         // foot magnet angle, 0 deg pose
mag_angs = [mag_ang0 - swing_dir * detent_preload, mag_ang0 + swing_dir * 90];   // tab magnets: 0 deg (offset so it pulls the lug onto the stop) / 90 deg
r_boss   = ear_r + 1.5;                                  // tab radius around P
plate_w  = esp_w + 2 * (tie_margin + tie_slot[0] / 2 + plat_margin);
plate_l  = esp_l + 2 * plat_margin;

// collar
R_ci     = barlow_od / 2 + collar_fit;
R_co     = R_ci + collar_wall;
pinch_ang = -hinge_pos;
nut_depth = nut_t + nut_clr + 0.1;
nut_ac   = nut_af / cos(30);
ear_hw   = slit_w / 2 + ear_wall + nut_depth + 1.2;

// carrier
pocket_w = max(pcb_w, holder_w) + pocket_clr;
z_pcb_back = lens_in + module_h + pcb_t;                 // carrier frame (z = 0 at nose end)
carrier_h = nose_len + puck_h;

// ------------------------------------------------------------------ sanity checks
assert(swing == 0 || swing == 90, "swing must be 0 or 90");
assert(swing_dir == 1 || swing_dir == -1, "swing_dir must be 1 or -1");
assert(hinge_bolt == 3 || hinge_bolt == 4, "hinge_bolt must be 3 or 4");
assert(barlow_od >= 20 && barlow_od <= 60, "barlow_od outside 20..60 mm");
assert(hinge_pos >= 60 && hinge_pos <= 300, "hinge_pos: the pinch ear must be at least 60 deg away from the hinge tab");
assert(travel >= 25, "Z travel is under 25 mm: lower z_margin or check the eyepiece_table heights");
assert(cup_z >= c_min && cup_z <= c_max, "cup_trim puts the cup outside the Z travel");
assert(Y0 >= ring_ro, "Y0 (arm half thickness) must cover the bearing ring on the tab: raise Y0");
assert(Y0 > stem_hw + 2, "Y0 too small for stem_hw: no dovetail flare left");
assert(r_mag + mag_d_p / 2 + 1.0 <= foot_r, "magnet pockets do not fit the foot plate: reduce magnet_d or the hinge bolt");
assert(z_fl - skirt_len >= -lens_in + 0.3,
  "skirt reaches below the nose end: shorten skirt_len (it must stay above the eyepiece rim)");
assert(R_sk > R_hole + 1.0, "skirt bore too small for the nose hole");
assert(R_cup - R_sk >= 1.5, "skirt wall under 1.5 mm: reduce skirt_gap / od_max or raise shell_wall");
assert(nose_d >= sqrt(2) * pocket_w + 2 * nose_wall_min,
  "nose_d too small for the square module pocket plus nose_wall_min");
assert(carrier_d >= nose_d + 2 * nose_wall_min, "carrier_d too small for nose_d");
assert(z_pcb_back + conn_h < carrier_h, "module pocket does not fit in the carrier: increase puck_h");
// nose end height above the rim at the lowest travel, for every eyepiece row
_nose_min = [for (r = eyepiece_table) gap_of(r) - lens_in - z_margin];
assert(min(_nose_min) >= -min([for (r = eyepiece_table) r[3]]) + 0.5,
  "nose would touch the eye lens at the low end of the travel: raise eye_relief or lower z_margin");
assert(min(_nose_min) >= 0 || nose_d + 1 <= rim_id,
  "the nose dips below the rim at the low end of the travel but nose_d + 1 > rim_id");
assert(od_max / 2 + 2 <= x_gf, "rail too close to the eyepiece housing: the cup shell must be wider than the housing");
assert(R_piv - r_boss >= R_co + 4, "hinge tab does not clear the collar ring");
assert(z_rb > barlow_above_collar + 2, "rail would reach down to the Barlow: raise eyepiece heights or lower stop_bolt_below");
assert(plate_z0 >= hub_h + foot_t + 4 && plate_z0 + plate_l < z_top, "ESP32 plate does not fit on the post: change plate_z0");

// ------------------------------------------------------------------ helpers
e = 0.01;
function ang_pt(r, a) = [r * cos(a), r * sin(a)];

// Radial nut boss along +x with a nut slot open at the top (like camera_cap).
module nut_boss(R, z_lo, z_hi) {
  bw = nut_af + nut_clr + 2 * boss_wall;
  x0 = R - 0.8;
  x1 = R + nut_depth + nut_back;
  translate([x0, -bw / 2, z_lo]) cube([x1 - x0, bw, z_hi - z_lo]);
}
module nut_cut(R_in, R, z_ax, z_hi) {
  translate([R, -(nut_af + nut_clr) / 2, z_ax - nut_ac / 2 - 0.1])
    cube([nut_depth, nut_af + nut_clr, z_hi - (z_ax - nut_ac / 2 - 0.1) + 1]);
  translate([R_in - 2, 0, z_ax]) rotate([0, 90, 0])
    cylinder(d = m3_clear, h = R + nut_depth + nut_back + 4 - (R_in - 2), $fn = 24);
}

// ------------------------------------------------------------------ collar (hinge frame, z = -collar_h .. 0)
module tab2d() {
  hull() {
    translate([R_co - 3, -tab_root_hw]) square([5, 2 * tab_root_hw]);
    translate([R_piv, 0]) circle(r = r_boss);
  }
}

module collar() {
  difference() {
    union() {
      translate([0, 0, -collar_h]) cylinder(r = R_co, h = collar_h);
      translate([0, 0, -collar_h]) linear_extrude(collar_h) tab2d();
      rotate(pinch_ang) translate([R_co - 1, -ear_hw, -collar_h]) cube([ear_len + 1, 2 * ear_hw, collar_h]);
      // bearing hub and ring: the arm foot rests on these (hub_h above the tab top)
      translate([R_piv, 0, -e]) {
        cylinder(r = hub_r, h = hub_h + e);
        difference() { cylinder(r = ring_ro, h = hub_h + e); translate([0, 0, -e]) cylinder(r = ring_ri, h = hub_h + 1); }
      }
      // hard stop for the 0 deg position
      translate([R_piv, 0, -e]) rotate(ear_ang)
        translate([stop_r0, swing_dir > 0 ? -(ear_w / 2 + stop_gap + stop_w) : ear_w / 2 + stop_gap, 0])
          cube([stop_len, stop_w, hub_h + foot_t + 1.5]);
    }
    // Barlow bore
    translate([0, 0, -collar_h - 1]) cylinder(r = R_ci, h = collar_h + 2);
    // pinch slit and M3 pinch screw (along the local y), nut pocket open at the top
    rotate(pinch_ang) {
      translate([R_ci - 1, -slit_w / 2, -collar_h - 1]) cube([ear_len + R_co - R_ci + 2, slit_w, collar_h + 2]);
      x_s = R_co + 7;
      z_s = -collar_h / 2;
      translate([x_s, 0, z_s]) rotate([90, 0, 0]) cylinder(d = m3_clear, h = 2 * ear_hw + 2, center = true, $fn = 24);
      translate([x_s - (nut_af + nut_clr) / 2, slit_w / 2 + ear_wall, z_s - nut_ac / 2 - 0.1])
        cube([nut_af + nut_clr, nut_depth, 1 - z_s + nut_ac / 2 + 0.1]);
    }
    // hinge axle hole and captive nut slot (slides in from the outer end of the tab)
    translate([R_piv, 0, -collar_h - 1]) cylinder(d = hinge_hole, h = collar_h + hub_h + 3, $fn = 24);
    // slot starts one nut half-width (across corners) inboard of the axle so the nut centres on it
    translate([R_piv - hinge_nut_af / cos(30) / 2 - 0.3, -(hinge_nut_af + nut_clr) / 2, -3 - (hinge_nut_t + nut_clr)])
      cube([r_boss + hinge_nut_af / cos(30) / 2 + 2.3, hinge_nut_af + nut_clr, hinge_nut_t + nut_clr]);
    // magnet pockets (0 / 90 deg detents)
    for (a = mag_angs)
      translate([R_piv + r_mag * cos(a), r_mag * sin(a), -(magnet_t + 0.1)])
        cylinder(d = mag_d_p, h = magnet_t + 0.1 + e, $fn = 48);
  }
}

// ------------------------------------------------------------------ arm (hinge frame, z as in the assembly)
module foot2d() {
  intersection() {
    circle(r = foot_r);
    square(2 * Y0, center = true);
    rotate(45) square(2 * (foot_r + 4) / sqrt(2) * 0.99, center = true);
  }
}

module rail2d() {   // (x, y) profile: wide head toward the cup, 45 deg flare, stem to the post
  polygon([[x_h0, -Y0], [x_h0 + head_flat, -Y0], [x_h1, -stem_hw], [x_bb0 + e, -stem_hw],
           [x_bb0 + e, stem_hw], [x_h1, stem_hw], [x_h0 + head_flat, Y0], [x_h0, Y0]]);
}

module mark_cut(z_mark, label) {
  // engraved on the +y face of the post: a line right behind the cup block top edge, label beside it
  translate([x_bb0 + 0.6, Y0 - engrave, z_mark - mark_w / 2]) cube([5, engrave + e, mark_w]);
  translate([x_post1 - 1, Y0 + e, z_mark - mark_text / 2]) rotate([90, 0, 0])
    linear_extrude(engrave + e) mirror([1, 0]) text(label, size = mark_text, valign = "center", halign = "left");
}

module arm() {
  difference() {
    intersection() {
      union() {
        // foot + web to the post
        translate([0, 0, hub_h]) linear_extrude(foot_t)
          hull() {
            translate([R_piv, 0]) foot2d();
            translate([x_bb0, -Y0]) square([post_d, 2 * Y0]);
          }
        // hard stop lug
        translate([R_piv, 0, hub_h]) rotate(ear_ang) translate([0, -ear_w / 2, 0]) cube([ear_r, ear_w, foot_t]);
        // post and boom
        translate([x_bb0, -Y0, hub_h]) cube([post_d, 2 * Y0, z_top - hub_h]);
        translate([x_h0, -Y0, z_bu]) cube([x_post1 - x_h0, 2 * Y0, beam_h]);
        // dovetail rail hanging from the boom
        translate([0, 0, z_rb]) linear_extrude(z_bu - z_rb + e) rail2d();
        // ESP32-S3 CAM plate, flush with the y = -Y0 face
        translate([x_bb0, -Y0, plate_z0]) cube([plate_w, plate_t, plate_l]);
      }
      translate([-1, -Y0, -1]) cube([300, 2 * Y0, 400]);   // keep every feature inside |y| <= Y0
    }
    // hinge axle hole
    translate([R_piv, 0, hub_h - 1]) cylinder(d = hinge_hole, h = foot_t + 2, $fn = 24);
    // foot magnet pocket (underside)
    translate([R_piv + r_mag * cos(mag_ang0), r_mag * sin(mag_ang0), hub_h - e])
      cylinder(d = mag_d_p, h = magnet_t + 0.1 + e, $fn = 48);
    // travel stop bolt (M3, across the rail stem, vertical when printed)
    translate([x_lock, 0, z_stop]) rotate([90, 0, 0]) cylinder(d = m3_clear, h = 2 * Y0 + 2, center = true, $fn = 24);
    // zip-tie slots on the ESP32 plate
    for (sx = [-1, 1], sz = [-1, 1])
      translate([x_bb0 + plate_w / 2 + sx * (esp_w / 2 + tie_margin) - tie_slot[0] / 2, -Y0 - e,
                 plate_z0 + plate_l / 2 + sz * esp_l / 4 - tie_slot[1] / 2])
        cube([tie_slot[0], plate_t + 2 * e, tie_slot[1]]);
    // Z marks: cup block top edge for each default eyepiece
    for (r = eyepiece_table) mark_cut(z_lens_of(r) + z_ct, str(r[0]));
  }
}

// ------------------------------------------------------------------ cup (cup frame, z = 0 lens front)
module groove2d() {   // (x, y) cut of the dovetail groove (floor toward the axis, opening toward the post)
  hwf = Y0 + dv_clr;
  hwn = stem_hw + dv_clr;
  polygon([[x_gf, -hwf], [x_h0 + head_flat, -hwf], [x_h1, -hwn], [x_cf + e, -hwn],
           [x_cf + e, hwn], [x_h1, hwn], [x_h0 + head_flat, hwf], [x_gf, hwf]]);
}

module cup() {
  difference() {
    union() {
      translate([0, 0, z_sk0]) cylinder(r = R_cup, h = z_ct - z_sk0);
      for (a = cup_screws) rotate(a) nut_boss(R_cup, z_sk0, z_ct);
      translate([R_cup - 1, -blk_hw, z_sk0]) cube([x_cf - R_cup + 1, 2 * blk_hw, z_ct - z_sk0]);
    }
    // bore: skirt / floor hole for the nose / carrier bore
    rotate_extrude()
      polygon([[0, z_sk0 - e], [R_sk, z_sk0 - e], [R_sk, z_fl], [R_hole, z_fl], [R_hole, z_pb],
               [R_b, z_pb], [R_b, z_ct + e], [0, z_ct + e]]);
    // three nylon M3 centring screws, nut slots open at the top
    for (a = cup_screws) rotate(a) nut_cut(R_b, R_cup, z_ts, z_ct);
    // dovetail groove, full height
    translate([0, 0, z_sk0 - e]) linear_extrude(z_ct - z_sk0 + 2 * e) groove2d();
    // M3 lock thumbscrew across the neck from +y (nut pocket open at the top)
    translate([x_lock, 0, z_lock]) rotate([90, 0, 0]) cylinder(d = m3_clear, h = 2 * blk_hw + 2, center = true, $fn = 24);
    translate([x_lock - (nut_af + nut_clr) / 2, blk_hw - nut_back - nut_depth, z_lock - nut_ac / 2 - 0.1])
      cube([nut_af + nut_clr, nut_depth, z_ct - (z_lock - nut_ac / 2 - 0.1) + 1]);
    // FPC notch in the shell rim
    rotate(fpc_angle) translate([R_b - 1, -(fpc_w + 3) / 2, z_ct - fpc_notch_depth])
      cube([shell_wall + 2, fpc_w + 3, fpc_notch_depth + 1]);
  }
}

// ------------------------------------------------------------------ carrier (carrier frame, z = 0 at nose end)
module carrier() {
  fw = fpc_w + 0.6;
  z_pt = carrier_h;
  rotate(fpc_angle - 90) difference() {
    union() {
      cylinder(d = nose_d, h = nose_len + e);
      translate([0, 0, nose_len]) cylinder(d = carrier_d, h = puck_h);
    }
    translate([-pocket_w / 2, -pocket_w / 2, -e]) cube([pocket_w, pocket_w, z_pcb_back + e]);
    translate([-fw / 2, -conn_y / 2, z_pcb_back - e]) cube([fw, conn_y, conn_h + e]);
    translate([-fw / 2, -fpc_slot_t / 2, z_pcb_back + conn_h - e])
      cube([fw, fpc_slot_t, z_pt - (z_pcb_back + conn_h) + 2 * e]);
    translate([-fw / 2, 0, z_pt - fpc_groove_d]) cube([fw, carrier_d / 2 + e, fpc_groove_d + e]);
    if (retain)
      translate([0, 0, lens_in + module_h + pcb_t / 2]) rotate([90, 0, 0])
        cylinder(d = retain_hole_d, h = carrier_d + 2, center = true, $fn = 24);
  }
}

// ------------------------------------------------------------------ assembly extras
module ov5640_ghost() {   // cup frame
  color("limegreen") translate([-pcb_w / 2, -pcb_w / 2, module_h]) cube([pcb_w, pcb_w, pcb_t]);
  color("dimgray") translate([-holder_w / 2, -holder_w / 2, 3]) cube([holder_w, holder_w, module_h - 3]);
  color("black") cylinder(d = barrel_d, h = 3);
}

module ghosts() {
  z_rim = z_rim_of(_row);
  color("silver", 0.55) translate([0, 0, -30]) cylinder(d = barlow_od, h = 30 + barlow_above_collar);  // Barlow
  color("silver", 0.55) difference() {                                                                  // eyepiece
    translate([0, 0, barlow_above_collar - 5]) cylinder(d = _row[1], h = 5 + z_rim - barlow_above_collar);
    translate([0, 0, z_rim - _row[3]]) cylinder(d = rim_id, h = _row[3] + e);
  }
  color("skyblue", 0.8) translate([0, 0, z_rim - _row[3] - 1]) cylinder(d = rim_id - 1, h = 1);
  color("gray", 0.5) translate([0, 0, -80]) cylinder(d = 60, h = 50);                                   // focuser
  rotate(motor_angle) color("gold", 0.6) translate([45, -14, -75]) cube([28, 28, 30]);                  // focus motor
}

module sec() {   // section=true keeps only y >= 0 (cut applied per part so colours survive)
  if (section)
    intersection() { children(); translate([-300, 0, -300]) cube([600, 600, 600]); }
  else children();
}

module hf() rotate(motor_angle + 180) children();                       // hinge frame -> assembly frame
module sw() translate([R_piv, 0, 0]) rotate(swing_dir * swing) translate([-R_piv, 0, 0]) children();   // swing about P

module assembly() {
  sec() hf() color("orange") collar();
  sec() hf() sw() color("steelblue") arm();
  sec() hf() sw() translate([0, 0, cup_z]) color("seagreen") cup();
  sec() hf() sw() translate([0, 0, cup_z - lens_in]) color("sandybrown") carrier();
  sec() hf() sw() translate([0, 0, cup_z]) ov5640_ghost();
  if (show_ghosts) sec() ghosts();
}

// ------------------------------------------------------------------ print poses
module collar_view() { if (print_pose) translate([0, 0, collar_h]) sec() collar(); else sec() collar(); }
module arm_view() {
  // lie on the y = -Y0 face: rotate so +y becomes up
  if (print_pose) translate([0, 0, Y0]) rotate([90, 0, 0]) sec() arm(); else sec() arm();
}
module cup_view() { if (print_pose) translate([0, 0, -z_sk0]) sec() cup(); else sec() cup(); }
module carrier_view() {
  if (print_pose) translate([0, 0, carrier_h]) rotate([180, 0, 0]) sec() carrier(); else sec() carrier();
}

if (part == "collar") collar_view();
else if (part == "arm") arm_view();
else if (part == "cup") cup_view();
else if (part == "carrier") carrier_view();
else if (part == "assembly") assembly();
else if (part == "none") { }   // render nothing: lets another file include this one and use the modules
else assert(false, "part must be \"collar\", \"arm\", \"cup\", \"carrier\" or \"assembly\"");
