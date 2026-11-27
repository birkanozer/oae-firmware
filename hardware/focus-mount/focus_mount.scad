// OAE focus motor mount: 28BYJ-48 on a clamp bracket, GT2 belt to the focus knob of a cheap
// rack-and-pinion focuser (see README.md for what to measure, buy, print and how to assemble).
//
// Printed parts:
//   bracket      : rectangular split collar clamping the focuser body (M3 pinch screw at the back),
//                  with two rails that carry the motor plate on the +y (knob) side.
//   motor_plate  : flat slotted plate on the rails; the 28BYJ-48 hangs behind it, its shaft comes
//                  through to the belt side. Slots on the ear holes and the shaft = belt tension.
//   knob_pulley  : GT2 split ring clamping the focus knob (M3 pinch screw), flanged both sides.
//   motor_pulley : GT2 pulley with a D bore for the double-flat shaft + M3 grub screw.
//
// Usage:
//   openscad -o bracket.stl      -D 'part="bracket"'      -D print_pose=true focus_mount.scad
//   openscad -o motor_plate.stl  -D 'part="motor_plate"'  -D print_pose=true focus_mount.scad
//   openscad -o knob_pulley.stl  -D 'part="knob_pulley"'  focus_mount.scad
//   openscad -o motor_pulley.stl -D 'part="motor_pulley"' focus_mount.scad
//   The console prints the belt length to buy (look for "BELT").
//
// Frames. World: the drawtube / optical axis is the z axis (x = y = 0), z = 0 is the base of the
// focuser body (where it sits on the tube), x runs along the tube, y along the focus-knob axis; the
// motor is on the +y side (knob side), mirror the printed parts in the slicer for the -y side.
// "Local" frame: origin on the focus-knob axis, rotated about y by motor_angle: local x = u points
// from the knob to the motor, local z = v is across the belt, local y is the world y (outboard).
// All lengths in mm.

// ------------------------------------------------------------------ what to show
part = "assembly";      // "bracket" | "motor_plate" | "knob_pulley" | "motor_pulley" | "assembly"
print_pose = false;     // true: bracket and motor_plate in print orientation (see README)
show_ghosts = true;     // assembly: focuser block, drawtube, tube, knob, motor, belt stand-ins
$fn = 64;               // round parts; small holes use fewer

motor_angle = 10;       // belt direction from the knob to the motor: 0 = +x horizontal, + = tilted up, away from the tube (+-40)

// ------------------------------------------------------------------ focuser (MEASURE THESE)
body_x = 40;            // focuser body size along the tube
body_y = 40;            // focuser body size across, between the two knob faces' base (knob axis direction)
body_h = 40;            // body height from the tube seat (z = 0) to its top face
knob_axis_z = 20;       // knob shaft axis height above z = 0
knob_axis_x = 0;        // knob shaft axis offset from the drawtube axis along x (+x = away from the tube end)
knob_d = 30;            // focus knob diameter
knob_w = 12;            // focus knob width
knob_gap = 4;           // body side face to the inner face of the knob (must exceed collar_wall_y + 0.5)
shaft_hole_d = 16;      // clearance hole in the collar side walls around the knob shaft and its hub
drawtube_od = 42;       // ghost only: drawtube outer diameter (check the rail post clears it)
tube_od = 76;           // ghost only: telescope tube outer diameter

// ------------------------------------------------------------------ motor 28BYJ-48 (5 V, unipolar)
motor_d = 28;           // body diameter
motor_depth = 19;       // body depth behind the ear face
ear_pitch = 35;         // distance between the two mounting-ear holes
ear_hole = 4.2;         // ear hole diameter
ear_w = 7;              // ear width
shaft_d = 5;            // shaft diameter
shaft_flat = 3;         // distance across the two shaft flats
shaft_flat_len = 6;     // length of the flatted end of the shaft
shaft_len = 10;         // shaft tip beyond the ear face (boss included)
shaft_off = 8;          // shaft offset from the body centre (perpendicular to the ear line)
boss_d = 9;             // round boss around the shaft
boss_h = 1.5;           // boss height
motor_flip = 1;         // +1 / -1: which side of the shaft the body centre is (along the belt direction)

// ------------------------------------------------------------------ belt and pulleys (GT2, 2 mm pitch)
pitch = 2;              // GT2 pitch
belt_w = 6;             // belt width
belt_play = 0.8;        // extra groove width in the knob pulley
tooth_h = 0.75;         // GT2 tooth height
tooth_r = 0.555;        // GT2 tooth radius
pld = 0.381;            // pitch line offset: OD = pitch diameter - 2 * pld
gt2_clr = 0.15;         // clearance added to the groove radius and depth of the printed teeth
motor_teeth = 20;       // motor pulley teeth
knob_teeth = 60;        // knob pulley teeth (3:1)
motor_distance = 46;    // wanted centre distance (must clear everything); the belt is rounded up to a loop
flange_t = 1.2;         // knob pulley flange thickness
flange_extra = 1.5;     // flange radius beyond the tooth tips
knob_fit = 0.3;         // knob pulley bore = knob_d + this (a pinch screw closes the rest)
shaft_clr = 0.1;        // D bore clearance per side
grub_d = 2.7;           // grub screw hole (M3 cuts its own thread)
plate_gap = 0.5;        // gap between the motor plate face and the motor pulley
loop_lengths = [for (l = [100 : 2 : 200]) l];   // common GT2 closed-loop lengths (mm)

// ------------------------------------------------------------------ hardware
m3_clear = 3.4;         // M3 clearance hole
m4_clear = 4.4;         // M4 clearance (motor ear screws)
nut_af = 5.5;           // M3 nut across flats
nut_t = 2.4;            // M3 nut thickness
nut_clr = 0.3;          // clearance around a nut in its pocket
head_d = 6.4;           // M3 cap head pocket diameter
head_h = 3.2;           // M3 cap head pocket depth

// ------------------------------------------------------------------ bracket (collar + rails)
fit = 0.3;              // collar cavity clearance per side around the body
collar_wall_x = 3;      // collar wall at the +x / -x faces
collar_wall_y = 3;      // collar wall at the +y / -y faces (sits in the knob gap)
clamp_z0 = 3;           // collar bottom (above the tube seat)
clamp_top_gap = 2;      // collar top below the body top
slit_w = 1.2;           // pinch slit at the -x wall
ear_len = 12;           // pinch ear length beyond the -x wall
ear_t = 5.5;            // pinch ear thickness either side of the slit
ear_h = 12;             // pinch ear height
rail_t = 6;             // rail thickness (y)
rail_w = 7;             // rail width (v)
rail_gap = 1.5;         // rail clearance beside the motor body / ears
rail_back = 18;         // rails start this far before the motor centre (u)
root_embed = 2;         // rail root block embedded in the collar +x wall
root_out = 2.5;         // rail root block beyond the collar +x wall
root_h = 8;             // rail root block height

// ------------------------------------------------------------------ motor plate
plate_t = 3;            // plate thickness
plate_near = 14;        // plate extent towards the knob from the motor centre (u)
plate_far = 24;         // plate extent away from the knob
plate_margin = 3.5;     // plate margin beyond the rail centre line
tension_travel = 8;     // total slot travel for belt tension (+-travel/2)
rail_screw_u = [-7, 17];// rail screw positions relative to the motor centre (u)
plate_r = 4;            // plate corner radius

// ------------------------------------------------------------------ knob pulley pinch ear
pinch_ear_t = 5;        // ear thickness either side of the slit
pinch_ear_out = 4.5;    // ear extends this far beyond the screw axis
pinch_screw_off = 1.8;  // screw axis beyond the flange radius

// ------------------------------------------------------------------ belt maths
function pd(n) = n * pitch / PI;                     // pitch diameter
function od(n) = pd(n) - 2 * pld;                    // outer diameter over the tooth tips
function belt_len(d1, d2, c) =                       // closed loop pitch length, exact open-belt formula
  let(phi = asin((d2 - d1) / (2 * c)))
  2 * c * cos(phi) + (d1 / 2) * (180 - 2 * phi) * PI / 180 + (d2 / 2) * (180 + 2 * phi) * PI / 180;
function solve_c(len, lo, hi, i = 0) =               // centre distance that gives a loop length (bisection)
  i >= 60 ? (lo + hi) / 2 :
  let(m = (lo + hi) / 2)
  belt_len(d1, d2, m) > len ? solve_c(len, lo, m, i + 1) : solve_c(len, m, hi, i + 1);

d1 = pd(motor_teeth);
d2 = pd(knob_teeth);
len_raw = belt_len(d1, d2, motor_distance);
loop = min([for (l = loop_lengths) if (l >= len_raw - 1e-6) l]);
assert(!is_undef(loop), "no common loop length is long enough: raise loop_lengths or lower motor_distance");
C = solve_c(loop, (d2 - d1) / 2 + 0.01, 400);        // centre distance for the chosen loop (design value)

echo(str("BELT: pulleys ", motor_teeth, "T / ", knob_teeth, "T (pitch dia ", round(d1 * 100) / 100, " / ",
         round(d2 * 100) / 100, " mm), wanted centre distance ", motor_distance, " mm"));
echo(str("BELT: needed pitch length ", round(len_raw * 10) / 10, " mm -> BUY a GT2 6 mm closed loop of ",
         loop, " mm (", loop / pitch, " teeth)"));
echo(str("BELT: centre distance for that loop = ", round(C * 100) / 100, " mm (slot travel +-", tension_travel / 2,
         " mm around it)"));

// ------------------------------------------------------------------ derived geometry
X_in = body_x / 2 + fit;
Y_in = body_y / 2 + fit;
x_out = X_in + collar_wall_x;
y_out = Y_in + collar_wall_y;
c_z0 = clamp_z0;
c_z1 = body_h - clamp_top_gap;

knob_in_y = body_y / 2 + knob_gap;
knob_out_y = knob_in_y + knob_w;
groove_w = belt_w + belt_play;
pulley_w = groove_w + 2 * flange_t;
pulley_y0 = knob_out_y - pulley_w;                  // inboard face of the knob pulley
y_belt = knob_out_y - pulley_w / 2;                 // belt mid plane, set by the knob pulley
m_len = shaft_len - plate_t - plate_gap;            // motor pulley length on the shaft
plate_out = y_belt - plate_gap - m_len / 2;         // plate face towards the belt
plate_in = plate_out - plate_t;                     // plate face towards the motor body
rail_y0 = plate_in - rail_t;

fl_r = od(knob_teeth) / 2 + flange_extra;           // knob pulley flange radius
m_tip_r = od(motor_teeth) / 2;
rail_v0 = max(motor_d / 2, ear_pitch / 2 + ear_w / 2) + rail_gap;
rail_v1 = rail_v0 + rail_w;
plate_hw = rail_v0 + rail_w / 2 + plate_margin + rail_w / 2;
rail_screw_v = rail_v0 + rail_w / 2;

assert(abs(motor_angle) <= 40, "motor_angle must be within +-40 deg");
assert(y_out + 0.5 <= knob_in_y, "knob_gap too small for the collar side wall: lower collar_wall_y or use a thinner wall");
assert(m_len >= belt_w, "motor pulley shorter than the belt: reduce plate_t / plate_gap or use a longer shaft");
assert(C - plate_near > fl_r + 2, "motor too close to the knob pulley: raise motor_distance");
assert(C > knob_d / 2 + motor_d / 2 + shaft_off + 1, "motor body would hit the knob: raise motor_distance");
assert(pulley_y0 > y_out + 0.5, "knob pulley would hit the collar: knob_w too small or knob_gap too small");

// ------------------------------------------------------------------ helpers
module in_local() { translate([knob_axis_x, 0, knob_axis_z]) rotate([0, -motor_angle, 0]) children(); }

// 2D children in (u, v) extruded along y from y0 to y1, in the local frame
module slab_y(y0, y1) { translate([0, y1, 0]) rotate([90, 0, 0]) linear_extrude(y1 - y0) children(); }

module local_box(u0, u1, y0, y1, v0, v1) { translate([u0, y0, v0]) cube([u1 - u0, y1 - y0, v1 - v0]); }

// hex nut pocket along +z from z = 0
module nut_pocket(h) { cylinder(d = (nut_af + nut_clr) / cos(30), h = h, $fn = 6); }

// GT2 tooth profile (simple semicircular grooves in a circle of the tip diameter)
module gt2_2d(n) {
  o = od(n);
  depth = tooth_h + gt2_clr;
  r = tooth_r + gt2_clr;
  difference() {
    circle(d = o, $fn = max(90, n * 4));
    for (i = [0 : n - 1]) rotate(i * 360 / n) translate([o / 2 - depth + r, 0])
      hull() { circle(r = r, $fn = 16); translate([depth + 1, 0]) circle(r = r, $fn = 16); }
  }
}

// ------------------------------------------------------------------ knob pulley (axis z, inboard face z = 0)
module knob_pulley() {
  bore_r = (knob_d + knob_fit) / 2;
  rs = fl_r + pinch_screw_off;                      // pinch screw axis radius (on the -x side)
  eo = rs + pinch_ear_out;                          // ear outer radius
  zs = pulley_w / 2;
  difference() {
    union() {
      cylinder(r = fl_r, h = flange_t, $fn = 120);
      translate([0, 0, pulley_w - flange_t]) cylinder(r = fl_r, h = flange_t, $fn = 120);
      translate([0, 0, flange_t]) linear_extrude(groove_w) gt2_2d(knob_teeth);
      // pinch ears on the -x side (opposite the motor, where the belt does not wrap)
      for (sy = [-1, 1]) translate([-eo, sy > 0 ? slit_w / 2 : -slit_w / 2 - pinch_ear_t, 0])
        cube([eo - (fl_r - 4), pinch_ear_t, pulley_w]);
    }
    translate([0, 0, -1]) cylinder(r = bore_r, h = pulley_w + 2, $fn = 120);
    // slit through the ring and ears
    translate([-eo - 1, -slit_w / 2, -1]) cube([eo + 1 - (bore_r - 1), slit_w, pulley_w + 2]);
    // pinch screw along y: clearance, head pocket on -y ear, nut pocket on +y ear
    translate([-rs, -slit_w / 2 - pinch_ear_t - 1, zs]) rotate([-90, 0, 0])
      cylinder(d = m3_clear, h = 2 * pinch_ear_t + slit_w + 2, $fn = 24);
    translate([-rs, -slit_w / 2 - pinch_ear_t - 0.01, zs]) rotate([-90, 0, 0]) cylinder(d = head_d, h = head_h, $fn = 32);
    translate([-rs, slit_w / 2 + pinch_ear_t + 0.01, zs]) rotate([90, 0, 0]) nut_pocket(nut_t + 0.4);
  }
}

// ------------------------------------------------------------------ motor pulley (axis z, plate side z = 0)
module motor_pulley() {
  round_len = max(0, m_len - shaft_flat_len);       // base part sits on the round, unflatted shaft
  fw = shaft_flat + 2 * shaft_clr;
  difference() {
    linear_extrude(m_len) gt2_2d(motor_teeth);
    translate([0, 0, -1]) cylinder(d = shaft_d + 2 * shaft_clr, h = round_len + 1, $fn = 48);
    translate([0, 0, round_len - 0.01]) linear_extrude(m_len - round_len + 1)
      intersection() { circle(d = shaft_d + 2 * shaft_clr, $fn = 48); square([fw, shaft_d + 2], center = true); }
    // grub screw towards +x, on the flat, in the flatted part
    translate([0, 0, round_len + (m_len - round_len) / 2]) rotate([0, 90, 0]) cylinder(d = grub_d, h = 12, $fn = 20);
  }
}

// ------------------------------------------------------------------ motor plate (local frame)
module motor_plate() {
  uc = C + motor_flip * shaft_off;                  // body centre / ear line
  half = tension_travel / 2;
  slab_y(plate_in, plate_out) difference() {
    translate([C - plate_near + plate_r, -plate_hw + plate_r])
      offset(r = plate_r) square([plate_near + plate_far - 2 * plate_r, 2 * plate_hw - 2 * plate_r]);
    // shaft + boss slot
    hull() for (du = [-half, half]) translate([C + du, 0]) circle(d = boss_d + 0.5, $fn = 40);
    // ear screw slots
    for (sv = [-1, 1]) hull() for (du = [-half, half]) translate([uc + du, sv * ear_pitch / 2]) circle(d = m4_clear, $fn = 24);
    // rail screws
    for (u = rail_screw_u, sv = [-1, 1]) translate([C + u, sv * rail_screw_v]) circle(d = m3_clear, $fn = 24);
  }
}

// ------------------------------------------------------------------ bracket (world frame)
module rail(sv) {
  v0 = sv > 0 ? rail_v0 : -rail_v1;
  v1 = v0 + rail_w;
  in_local() local_box(C - rail_back, C + plate_far, rail_y0, plate_in, v0, v1);
  hull() {
    in_local() local_box(C - rail_back, C - rail_back + 2, rail_y0, plate_in, v0, v1);
    translate([x_out - root_embed, rail_y0, sv > 0 ? c_z1 - root_h : c_z0])
      cube([root_embed + root_out, plate_in - rail_y0, root_h]);
  }
}

module bracket() {
  ex = -(x_out + ear_len / 2);                      // pinch screw x (at the -x wall ears)
  ez = c_z0 + ear_h / 2;
  difference() {
    union() {
      translate([-x_out, -y_out, c_z0]) cube([2 * x_out, 2 * y_out, c_z1 - c_z0]);
      for (sy = [-1, 1]) translate([-x_out - ear_len, sy > 0 ? slit_w / 2 : -slit_w / 2 - ear_t, c_z0])
        cube([ear_len + 0.5, ear_t, ear_h]);
      rail(1);
      rail(-1);
    }
    translate([-X_in, -Y_in, -1]) cube([2 * X_in, 2 * Y_in, c_z1 + 2]);                  // body cavity
    translate([-x_out - ear_len - 1, -slit_w / 2, c_z0 - 1]) cube([ear_len + 1 + collar_wall_x + 0.5, slit_w, c_z1 - c_z0 + 2]);   // slit
    // pinch screw along y with head pocket (-y ear) and nut pocket (+y ear)
    translate([ex, -slit_w / 2 - ear_t - 1, ez]) rotate([-90, 0, 0]) cylinder(d = m3_clear, h = 2 * ear_t + slit_w + 2, $fn = 24);
    translate([ex, -slit_w / 2 - ear_t - 0.01, ez]) rotate([-90, 0, 0]) cylinder(d = head_d, h = head_h, $fn = 32);
    translate([ex, slit_w / 2 + ear_t + 0.01, ez]) rotate([90, 0, 0]) nut_pocket(nut_t + 0.4);
    // clearance for the knob shafts through both side walls
    translate([knob_axis_x, -y_out - 1, knob_axis_z]) rotate([-90, 0, 0]) cylinder(d = shaft_hole_d, h = 2 * y_out + 2, $fn = 48);
    // rail screws: clearance through the rail, nut pocket from the inboard face
    in_local() for (u = rail_screw_u, sv = [-1, 1]) {
      translate([C + u, rail_y0 - 1, sv * rail_screw_v]) rotate([-90, 0, 0]) cylinder(d = m3_clear, h = rail_t + 2, $fn = 24);
      translate([C + u, rail_y0 - 0.01, sv * rail_screw_v]) rotate([-90, 0, 0]) nut_pocket(nut_t + 0.4);
    }
  }
}

// ------------------------------------------------------------------ ghosts (not printed)
module motor_ghost() {      // axis z = shaft direction, ear face at z = 0, shaft at the origin
  color("goldenrod") {
    translate([motor_flip * shaft_off, 0, -motor_depth]) cylinder(d = motor_d, h = motor_depth, $fn = 64);
    translate([motor_flip * shaft_off, 0, -0.8]) linear_extrude(0.8) difference() {
      hull() for (s = [-1, 1]) translate([0, s * ear_pitch / 2]) circle(d = ear_w, $fn = 24);
      for (s = [-1, 1]) translate([0, s * ear_pitch / 2]) circle(d = ear_hole, $fn = 24);
    }
    cylinder(d = boss_d, h = boss_h, $fn = 40);
  }
  color("silver") difference() {
    cylinder(d = shaft_d, h = shaft_len, $fn = 32);
    for (s = [-1, 1]) translate([s * (shaft_flat / 2 + 5), 0, shaft_len - shaft_flat_len / 2 + 0.01])
      cube([10, 10, shaft_flat_len + 0.1], center = true);
  }
}

module belt_ghost() {
  color("dimgray") translate([0, y_belt - belt_w / 2, 0]) rotate([-90, 0, 0]) linear_extrude(belt_w) difference() {
    hull() { circle(d = d2 + 1.4); translate([C, 0]) circle(d = d1 + 1.4); }
    hull() { circle(d = d2 - 1.0); translate([C, 0]) circle(d = d1 - 1.0); }
  }
}

module ghosts() {
  color("lightgray", 0.45) {
    translate([-body_x / 2, -body_y / 2, 0]) cube([body_x, body_y, body_h]);                      // focuser body
    translate([0, 0, body_h]) cylinder(d = drawtube_od, h = 30);                                                  // drawtube
    translate([-150, 0, -tube_od / 2]) rotate([0, 90, 0]) cylinder(d = tube_od, h = 300, $fn = 96);   // tube
    translate([knob_axis_x, -knob_out_y, knob_axis_z]) rotate([-90, 0, 0]) cylinder(d = 8, h = 2 * knob_out_y, $fn = 24);
  }
  color("steelblue", 0.6) for (s = [-1, 1])                                                         // knobs
    translate([knob_axis_x, s > 0 ? knob_in_y : -knob_out_y, knob_axis_z]) rotate([-90, 0, 0]) cylinder(d = knob_d, h = knob_w, $fn = 64);
  in_local() {
    translate([C, plate_in, 0]) rotate([-90, 0, 0]) motor_ghost();
    belt_ghost();
  }
}

// ------------------------------------------------------------------ parts and assembly
module assembly() {
  color("orange") bracket();
  color("tomato") in_local() motor_plate();
  color("seagreen") in_local() translate([0, pulley_y0, 0]) rotate([-90, 0, 0]) knob_pulley();
  color("mediumpurple") in_local() translate([C, plate_out + plate_gap, 0]) rotate([-90, 0, 0]) motor_pulley();
  if (show_ghosts) ghosts();
}

if (part == "bracket") {
  if (print_pose) translate([0, 0, plate_in]) rotate([-90, 0, 0]) bracket(); else bracket();
} else if (part == "motor_plate") {
  if (print_pose) rotate([90, 0, 0]) translate([-C, -plate_in, 0]) motor_plate(); else in_local() motor_plate();
} else if (part == "knob_pulley") knob_pulley();
else if (part == "motor_pulley") motor_pulley();
else if (part == "assembly") assembly();
