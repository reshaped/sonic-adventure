#include "player_terrain.h"

#include "collision/collision.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"

static struct task_work work;
static struct task_motion motion;
static struct player record;
static struct polygon_ball ball;
static struct collision_work collision;

struct previous_player {
    struct task_work *work;
    struct task_motion *motion;
    struct player *record;
    NJS_VECTOR gravity;
};

static struct previous_player begin(int character, int mode, NJS_POINT3 start,
                                    const struct character_constants *constants,
                                    NJS_OBJECT *ground, Float speed)
{
    struct previous_player previous = {player_work[0], player_motion[0],
                                       player_records[0], world_gravity};
    work = (struct task_work){.mode = mode,
                              .flags = PLAYER_ON_LAND,
                              .pos = start,
                              .collision = &collision};
    motion = (struct task_motion){.speed = {speed, 0, 0}};
    record = (struct player){.contact = &ball,
                             .speed = motion.speed,
                             .consts = *constants,
                             .level = 1,
                             .ground_normal = {0, 1, 0},
                             .last_ground_flags = WORLD_SOLID,
                             .last_flags = PLAYER_ON_LAND,
                             .ground_object = ground,
                             .ground_flags = WORLD_SOLID};
    collision = (struct collision_work){0};
    ball = (struct polygon_ball){0};
    PLAYER_VARS(&work)->character = character;
    player_work[0] = &work;
    player_motion[0] = &motion;
    player_records[0] = &record;
    world_gravity = (NJS_VECTOR){0, -1, 0};
    return previous;
}

static void restore(const struct previous_player *previous)
{
    player_work[0] = previous->work;
    player_motion[0] = previous->motion;
    player_records[0] = previous->record;
    world_gravity = previous->gravity;
}

static Bool near(NJS_VECTOR a, NJS_VECTOR b, Float tolerance)
{
    return njAbs(a.x - b.x) <= tolerance && njAbs(a.y - b.y) <= tolerance &&
           njAbs(a.z - b.z) <= tolerance;
}

/* The supported interval starts several tiles before the first turn.
 * Both gentle changes of grade contribute to the final height and tangent. */
static Bool tiled_descent(terrain_install install,
                          const struct character_constants *sonic)
{
    static const Float column[] = {-15.0F, -12.0F, -10.0F, -8.0F, -6.0F,
                                   0.0F,   6.0F,   12.0F,  18.0F};
    static NJS_POINT3 points[18];
    static Sint16 strip[19] = {18};
    static NJS_MESHSET set = {NJD_MESHSET_TRIMESH, 1, strip};
    static NJS_MODEL model = {
        points, NULL, 18, &set, NULL, 1, 0, {0.0F, 0.0F, 0.0F}, 32.0F};
    static NJS_OBJECT land = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    for (int i = 0; i < 18; i++) {
        Float x = column[i / 2];
        Float y = x <= 0.0F ? 0.0F : -x * 0x1p-16F;
        if (x > 6.0F) {
            y -= (x - 6.0F) * 0x1p-16F;
        }
        points[i] = (NJS_POINT3){x, y, i & 1 ? 24.0F : -24.0F};
        strip[i + 1] = i;
    }
    NJS_POINT3 start = {-10.5F, 0.0F, -2.0F};
    struct previous_player previous = begin(
        CHARACTER_SONIC, sonic_terrain_modes.walk, start, sonic, &land, 20.0F);
    install(&land, WORLD_SOLID);
    Uint32 hit = player_collide(&work, &motion, &record);
    Bool good =
        !hit && (work.flags & PLAYER_ON_LAND) &&
        record.ground_object == &land &&
        near(work.pos, (NJS_POINT3){9.5F, -13.0F * 0x1p-16F, -2.0F}, .0001F) &&
        near(motion.speed, (NJS_VECTOR){20.0F, -40.0F * 0x1p-16F, 0.0F},
             .0001F);
    restore(&previous);
    return good;
}

/* Final Egg's wide panel reaches a flat neighbor along x=0. Reflecting its
 * height gives the same small turn in both directions. At the higher speed
 * the feet must finish on the flat plane with its tangent. */
static int final_egg_tread(terrain_install install,
                           const struct character_constants *sonic)
{
    static NJS_POINT3 points[] = {{-0x1.03fdep+6F, 0.0F, -0x1.e05ep+2F},
                                  {0.0F, 0.0F, 0x1.b2ffep+6F},
                                  {0.0F, 0.0F, 0.0F},
                                  {65.0F, 0.0F, 0x1.b2ffep+6F}};
    static Sint16 indices[] = {0, 1, 2, 1, 3, 2};
    static NJS_MATERIAL material = {{0}, {0}, 0, 0, 0x94282400U};
    static NJS_MESHSET set = {NJD_MESHSET_3, 2, indices};
    static NJS_MODEL model = {
        points, NULL, 4, &set, &material, 1, 1, {0.0F, 0.0F, 54.0F}, 100.0F};
    static NJS_OBJECT land = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    const Uint32 flags = LAND_DRAWN | WORLD_LARGE | WORLD_SOLID;
    for (int i = 0; i < 4; i++) {
        points[0].y = i & 1 ? -0x1.9p-11F : 0x1.9p-11F;
        Float grade = points[0].y / points[0].x;
        Float speed = i < 2 ? 8.0F : 15.0F;
        Float dx = -speed * 0.5F;
        NJS_POINT3 start = {dx, dx * grade, points[1].z * 0.5F};
        struct previous_player previous =
            begin(CHARACTER_SONIC, sonic_terrain_modes.walk, start, sonic,
                  &land, speed);
        motion.speed.y = speed * grade;
        record.ground_normal = (NJS_VECTOR){-grade, 1.0F, 0.0F};
        record.ground_flags = record.last_ground_flags = flags;
        install(&land, flags);
        Uint32 hit = player_collide(&work, &motion, &record);
        Float tolerance = i < 2 ? .0001F : .00003F;
        Bool good = !hit && (work.flags & PLAYER_ON_LAND) &&
                    record.ground_object == &land &&
                    record.ground_flags == flags &&
                    njAbs(work.pos.x + dx) < .001F &&
                    njAbs(work.pos.z - start.z) < .001F &&
                    njAbs(work.pos.y) < tolerance &&
                    njAbs(motion.speed.y) < tolerance &&
                    njAbs(motion.speed.x - speed) < .001F &&
                    njAbs(motion.speed.z) < tolerance;
        restore(&previous);
        if (!good) {
            return i + 1;
        }
    }
    return 0;
}

/* Sonic crosses this high-grip Speed Highway tread after the dash panels.
 * Its fourth corner lies one float increment below the other face's plane.
 * Hex literals preserve the captured bits under SHC's decimal conversion. */
int terrain_tread_cases(terrain_install install,
                        const struct character_constants *sonic)
{
    static const NJS_POINT3 road[] = {
        {1157.978394F, -1141.681641F, 252.776047F},
        {1157.978394F, -1141.681641F, 312.776093F},
        {0x1.290794p+10F, -0x1.1dc1e4p+10F, 258.029572F},
        {0x1.290794p+10F, -0x1.1dc1e6p+10F, 318.029633F}};
    static const int faces[2][6] = {{0, 1, 2, 1, 3, 2}, {0, 1, 3, 0, 3, 2}};
    static Sint16 winding[2][6] = {{0, 1, 2, 3, 4, 5}, {0, 2, 1, 3, 5, 4}};
    static NJS_POINT3 points[6];
    static NJS_MATERIAL material = {{0}, {0}, 0, 0, NJD_FLAG_DOUBLE_SIDE};
    static NJS_MESHSET set = {NJD_MESHSET_3};
    static NJS_MODEL model = {
        points, NULL, 6, &set, &material, 1, 1, {0.0F, 0.0F, 0.0F}, 100.0F};
    static NJS_OBJECT land = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    const Uint32 flags =
        WORLD_SOLID | WORLD_GRIP_HIGH | WORLD_MEDIUM | LAND_DRAWN;

    for (int layout = 0; layout < 8; layout++) {
        NJS_POINT3 origin = layout & 4 ? road[0] : (NJS_POINT3){0};
        NJS_POINT3 start = {1164.308838F - origin.x, -1141.964722F - origin.y,
                            285.101288F - origin.z};
        NJS_POINT3 finish = {1177.178467F - origin.x,
                             -0x1.1da294p+10F - origin.y,
                             287.916382F - origin.z};
        set.meshes = winding[(layout >> 1) & 1];
        enum { JOINED, OPEN, LOWERED };
        for (int edge = JOINED; edge <= LOWERED; edge++) {
            for (int i = 0; i < 6; i++) {
                NJS_POINT3 v = road[faces[layout & 1][i]];
                points[i] = (NJS_POINT3){v.x - origin.x, v.y - origin.y,
                                         v.z - origin.z};
                if (edge == LOWERED && i >= 3) {
                    /* The projected edge still lines up across this gap. */
                    points[i].y -= 0x1p-12F;
                }
            }
            set.nbMesh = edge == OPEN ? 1 : 2;
            model.center = points[0];
            struct previous_player previous =
                begin(CHARACTER_SONIC, sonic_terrain_modes.walk, start, sonic,
                      &land, 13.186513F);
            work.ang.y = motion.aim.y = 2244;
            work.ang.z = -466;
            motion.speed =
                (NJS_VECTOR){12.866091F, -0x1.4f76d4p-1F, 0x1.6855fp+1F};
            record.speed.y = -0x1.475a5ep-4F;
            record.ground_flags = record.last_ground_flags = flags;
            record.ground_normal =
                (NJS_VECTOR){0x1.6dfdb2p-5F, 0x1.ff7d2p-1F, -0.0F};
            install(&land, flags);
            Uint32 hit = player_collide(&work, &motion, &record);
            Bool good =
                !hit && !!(work.flags & PLAYER_ON_LAND) == (edge == JOINED) &&
                (edge != JOINED || (record.ground_object == &land &&
                                    record.ground_flags == flags)) &&
                near(work.pos, finish, .001F) &&
                near(motion.speed,
                     (NJS_VECTOR){12.869653F, -.57554746F, 0x1.6855fp+1F},
                     .001F);
            restore(&previous);
            if (!good) {
                return layout * 3 + edge + 1;
            }
        }
    }
    int wide = final_egg_tread(install, sonic);
    if (wide) {
        return 24 + wide;
    }
    return tiled_descent(install, sonic) ? 0 : 29;
}

/* Knuckles crosses the diagonal in Red Mountain's sloped strip on his
 * first tick. He stays on the strip for two ticks and leaves at its crest. */
int terrain_crest_case(terrain_install install,
                       const struct character_constants *knuckles)
{
    static NJS_POINT3 points[] = {
        {-3566.42285F, 726.965393F, -1789.65979F},
        {-3480.32251F, 726.965393F, -1921.06494F},
        {-3574.19287F, 732.118469F, -1802.03064F},
        {-3492.79004F, 732.118469F, -1928.5896F},
        {-3515.13184F, 732.157837F, -1938.28674F},
        {-3591.63159F, 732.517151F, -1821.24609F},
    };
    static Sint16 slope[] = {3, 0, 1, 2, 3, 2, 1, 3};
    static Sint16 crest[] = {3, 3, 4, 2, 3, 2, 4, 5};
    static NJS_MATERIAL materials[] = {{{0}, {0}, 0, 0, 0x9430a400U},
                                       {{0}, {0}, 0, 0, 0x94202400U}};
    static NJS_MESHSET sets[] = {{NJD_MESHSET_TRIMESH, 2, slope},
                                 {NJD_MESHSET_TRIMESH | 1, 2, crest}};
    static NJS_MODEL model = {
        points, NULL, 6, sets, materials, 2, 2, {-3536, 730, -1864}, 100};
    static NJS_OBJECT ground = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    const NJS_POINT3 start = {-3545.49658F, 730.496338F, -1838.1991F};
    const NJS_VECTOR uphill = {-1.67151F, 0.77412F, -1.44353F};
    struct previous_player previous =
        begin(CHARACTER_KNUCKLES, knuckles_terrain_modes.walk, start, knuckles,
              &ground, 2.34F);
    record.ground_normal =
        (NJS_VECTOR){0.302602261F, 0.932265818F, 0.198273495F};
    motion.speed = (NJS_VECTOR){-1.6736815F, 0.770172298F, -1.44507205F};
    install(&ground, 0xa0000001U);
    int result = 0;
    for (int tick = 0; tick < 3; tick++) {
        if (player_collide(&work, &motion, &record) ||
            !!(work.flags & PLAYER_ON_LAND) != (tick < 2) ||
            record.ground_object != (tick < 2 ? &ground : NULL) ||
            !near(motion.speed, uphill, 0.001F)) {
            result = 1 + tick;
            break;
        }
        if (tick == 0 &&
            !near(work.pos,
                  (NJS_POINT3){-3547.04106F, 731.21762F, -1839.54017F},
                  0.003F)) {
            result = 4;
            break;
        }
        player_take_speed(&work, &motion, &record);
    }
    restore(&previous);
    return result;
}

static NJS_OBJECT *curb_geometry(Float roof)
{
    static NJS_POINT3 points[] = {
        {-64, 0, -32}, {-64, 0, 32},  {0, 0, 32},   {0, 0, -32},
        {0, 2, -32},   {0, 2, 32},    {64, 2, 32},  {64, 2, -32},
        {-64, 0, 32},  {-64, 0, -32}, {64, 0, -32}, {64, 0, 32},
    };
    static Sint16 triangles[] = {
        3, 0, 1, 2,  3, 0, 2,  3, /* lower tread */
        3, 5, 6, 7,  3, 5, 7,  4, /* upper tread */
        3, 2, 5, 4,  3, 2, 4,  3, /* riser */
        3, 8, 9, 10, 3, 8, 10, 11, /* ceiling */
    };
    static NJS_MESHSET set = {NJD_MESHSET_TRIMESH, 8, triangles};
    static NJS_MATERIAL material;
    static NJS_MODEL model = {points, NULL, 12,        &set, &material,
                              1,      1,    {0, 0, 0}, 80};
    static NJS_OBJECT ground = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    for (int i = 8; i < 12; i++) {
        points[i].y = roof;
    }
    return &ground;
}

/* The curb is two units high. A five-unit sphere fits above its upper
 * tread under the high roof. Under the low roof it can reach only the lip. */
static int curb(terrain_install install,
                const struct character_constants *sonic, Float roof,
                Bool blocked)
{
    NJS_OBJECT *ground = curb_geometry(roof);
    struct previous_player previous =
        begin(CHARACTER_SONIC, sonic_terrain_modes.walk, (NJS_POINT3){-8, 0, 1},
              sonic, ground, 2);
    install(ground, WORLD_SOLID);
    int result = 0;
    for (int tick = 0; tick < 10; tick++) {
        if (blocked) {
            motion.speed = (NJS_VECTOR){2, 0, 0};
            player_take_speed(&work, &motion, &record);
        }
        player_collide(&work, &motion, &record);
        if (blocked) {
            if (!(work.flags & PLAYER_ON_LAND) ||
                record.ground_object != ground || njAbs(work.pos.y) > 0.002F ||
                work.pos.x > -3.998F || njAbs(work.pos.z - 1) > 0.005F) {
                result = 1;
                break;
            }
        } else if (njAbs(work.pos.x - (-8 + (tick + 1) * 2)) > 0.005F ||
                   njAbs(work.pos.z - 1) > 0.005F) {
            result = 2;
            break;
        }
        player_take_speed(&work, &motion, &record);
        player_apply_accel(&work, &motion, &record);
    }
    if (!result && blocked) {
        Float x = work.pos.x;
        work.ang.y = motion.aim.y = 0x8000;
        motion.speed = (NJS_VECTOR){-2, 0, 0};
        player_take_speed(&work, &motion, &record);
        player_collide(&work, &motion, &record);
        if (!(x >= -6.001F) ||
            !near(work.pos, (NJS_POINT3){x - 2, 0, 1}, 0.0001F)) {
            result = 3;
        }
    } else if (!result && (!(work.flags & PLAYER_ON_LAND) ||
                           record.ground_object != ground ||
                           njAbs(work.pos.y - 2) > 0.002F)) {
        result = 4;
    }
    restore(&previous);
    return result;
}

int terrain_curb_case(terrain_install install,
                      const struct character_constants *sonic)
{
    int result = curb(install, sonic, 12.5F, FALSE);
    if (result) {
        return result;
    }
    result = curb(install, sonic, 11, TRUE);
    return result ? 4 + result : 0;
}

static NJS_VECTOR roll_step(void)
{
    player_turn_to_stick(&work, &motion, &record);
    player_accel_roll(&work, &motion, &record);
    player_apply_accel(&work, &motion, &record);
    NJS_VECTOR entry = motion.speed;
    player_collide(&work, &motion, &record);
    player_take_speed(&work, &motion, &record);
    return entry;
}

/* Sonic's Speed Highway demo crosses these ramp panels on foot.
 * Start his released spin dash four units before their common edge. */
static Bool highway_roll(terrain_install install,
                         const struct character_constants *sonic)
{
    static NJS_POINT3 points[] = {{1085.36474F, -1129.48071F, 59.9000015F},
                                  {1068, -1130.5F, 54.9000015F},
                                  {1068, -1130.5F, 114.900001F},
                                  {1085.36474F, -1129.48071F, 119.900001F},
                                  {1102.2019F, -1124.96911F, 64.9000015F},
                                  {1102.2019F, -1124.96911F, 124.900001F}};
    static Sint16 faces[] = {0, 1, 2, 3, 0, 2, 4, 0, 3, 5, 4, 3};
    static NJS_MATERIAL material = {{0}, {0}, 0, 0, 0x94ac2400U};
    static NJS_MESHSET set = {NJD_MESHSET_3, 4, faces};
    static NJS_MODEL model = {
        points, NULL, 6, &set, &material, 1, 1, {1085, -1128, 90}, 48};
    static NJS_OBJECT ground = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    const Uint32 flags = 0xc0000001U;
    struct previous_player previous = begin(
        CHARACTER_SONIC, sonic_terrain_modes.fast,
        (NJS_POINT3){1081.37161F, -1129.71510F, 89.9F}, sonic, &ground, 10);
    work.flags |= PLAYER_ATTACKING;
    work.ang.z = motion.aim.z = 612;
    record.ground_flags = record.last_ground_flags = flags;
    record.ground_normal = (NJS_VECTOR){-0.05859797F, 0.99828166F, 0};
    install(&ground, flags);
    NJS_VECTOR entry = roll_step();
    /* Redirect the incoming tangent onto the 15-degree outgoing panel. */
    Float speed = entry.x * 0.99828166F + entry.y * 0.05859797F;
    NJS_VECTOR tangent = {speed * 0.96592444F, speed * 0.25882421F, 0};
    Float rise = (work.pos.x - points[0].x) * (0.25882421F / 0.96592444F);
    Bool ok = (work.flags & PLAYER_ON_LAND) &&
              record.ground_object == &ground && work.pos.x > points[0].x &&
              njAbs(work.pos.y - points[0].y - rise) < 0.002F &&
              near(motion.speed, tangent, 0.002F) &&
              njAbs(record.speed.x - speed) < 0.002F &&
              near(ball.hit.normal_speed, (NJS_VECTOR){0}, 0.002F);
    restore(&previous);
    return ok;
}

static Bool rolling_curb(terrain_install install, int character, int mode,
                         const struct character_constants *constants,
                         Bool blocked)
{
    Float height = constants->height;
    NJS_OBJECT *ground = curb_geometry(height + (blocked ? 1 : 3));
    struct previous_player previous =
        begin(character, mode, (NJS_POINT3){-8, 0, 1}, constants, ground, 6);
    work.flags |= PLAYER_ATTACKING;
    install(ground, WORLD_SOLID);
    roll_step();
    Bool ok;
    if (blocked) {
        /* The rim is two units up. At contact x² + (r-2)² = r². */
        Float x = -njSqrt(2 * height - 4);
        ok = near(work.pos, (NJS_POINT3){x, 0, 1}, 0.002F) &&
             njAbs(record.speed.x) < 0.001F;
    } else {
        roll_step();
        ok = work.pos.x > 0 && njAbs(work.pos.y - 2) < 0.002F &&
             record.speed.x > 5;
    }
    ok = ok && (work.flags & PLAYER_ON_LAND) &&
         record.ground_object == ground && ball.radius == height * 0.5F;
    restore(&previous);
    return ok;
}

int terrain_roll_case(terrain_install install,
                      const struct character_constants *characters)
{
    if (!highway_roll(install, &characters[CHARACTER_SONIC])) {
        return 1;
    }
    const int ids[] = {CHARACTER_SONIC, CHARACTER_TAILS, CHARACTER_KNUCKLES};
    const int modes[] = {sonic_terrain_modes.fast, tails_terrain_modes.fast,
                         knuckles_terrain_modes.fast};
    for (int i = 0; i < 3; i++) {
        if (!rolling_curb(install, ids[i], modes[i], &characters[ids[i]],
                          FALSE) ||
            !rolling_curb(install, ids[i], modes[i], &characters[ids[i]],
                          TRUE)) {
            return 2 + i;
        }
    }
    return 0;
}

/* The first Speed Highway dash panels send Sonic across this convex crest.
 * A runner under his own control takes off at the same position and speed. */
static int dash_crest(terrain_install install,
                      const struct character_constants *sonic)
{
    static NJS_POINT3 points[] = {{1368.081055F, -1126.802002F, 334.580811F},
                                  {1368.081177F, -1126.802002F, 274.580811F},
                                  {1338.086548F, -1129.975098F, 333.461945F},
                                  {1338.086548F, -1129.975098F, 273.461945F},
                                  {1308.300537F, -1134.783325F, 331.849701F},
                                  {1308.300537F, -1134.783325F, 271.849640F}};
    static Sint16 faces[] = {0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4};
    static NJS_MATERIAL material;
    static NJS_MESHSET set = {NJD_MESHSET_3, 4, faces};
    static NJS_MODEL model = {
        points, NULL, 6, &set, &material, 1, 1, {1338, -1130, 305}, 64};
    static NJS_OBJECT road = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    const Uint32 flags =
        WORLD_SOLID | WORLD_GRIP_HIGH | WORLD_MEDIUM | LAND_DRAWN;
    const NJS_POINT3 feet = {1330.402344F, -1131.215454F, 321.348389F};
    const NJS_VECTOR incoming = {10.529958F, 1.6194634F, 2.3280256F};
    enum { RUNNER, PANEL, CAMERA, EXPIRED, REPLACED };
    for (int i = RUNNER; i <= REPLACED; i++) {
        struct previous_player previous =
            begin(CHARACTER_SONIC, sonic_terrain_modes.walk, feet, sonic, &road,
                  10.904868F);
        work.ang.y = motion.aim.y = 2244;
        work.ang.z = motion.aim.z = 1669;
        motion.speed = incoming;
        record.speed.y = -0.07897801F;
        record.ground_flags = record.last_ground_flags = flags;
        record.ground_normal = (NJS_VECTOR){-0.15936269F, 0.98722005F, 0};
        PLAYER_LOCK_INPUT(&record, i == CAMERA ? 30 : 42);
        if (i == PANEL || i == EXPIRED || i == REPLACED) {
            PLAYER_MARK_DASH_PANEL(&record);
        }
        if (i == EXPIRED) {
            PLAYER_LOCK_INPUT(&record, 0);
        }
        if (i == REPLACED) {
            NJS_VECTOR kick = {10.904868F, 0, 0};
            const Angle angles[3] = {0, 0x4000 - 2244, 1669};
            player_send_flying(0, &kick, angles, 42);
            /* Compare the two sources on the same approach to the crest. */
            motion.speed = incoming;
            record.speed.y = -0.07897801F;
        }
        /* The other player may be on a panel while Sonic crosses the crest. */
        struct previous_player second = {player_work[1], player_motion[1],
                                         player_records[1], world_gravity};
        struct player other = record;
        if (i == RUNNER) {
            PLAYER_LOCK_INPUT(&other, 42);
            PLAYER_MARK_DASH_PANEL(&other);
            player_records[0] = &other;
            player_work[1] = &work;
            player_motion[1] = &motion;
            player_records[1] = &record;
            PLAYER_VARS(&work)->player = 1;
        }
        install(&road, flags);
        int hit = player_collide(&work, &motion, &record);
        Bool supported = i == PANEL;
        Bool good = !hit && !!(work.flags & PLAYER_ON_LAND) == supported &&
                    record.ground_object == (supported ? &road : NULL) &&
                    work.pos.x > 1340 && work.pos.x < 1342 &&
                    motion.speed.x > 10;
        player_work[1] = second.work;
        player_motion[1] = second.motion;
        player_records[1] = second.record;
        restore(&previous);
        if (!good) {
            return 1 + i;
        }
    }
    return 0;
}

/* A dash can graze a wall or jump into it. In either case the sphere
 * travels along the wall without losing its tangential speed. */
static int dash_wall(terrain_install install,
                     const struct character_constants *sonic)
{
    static NJS_POINT3 points[] = {{-64, 0, -64}, {-64, 0, 64},  {64, 0, 64},
                                  {64, 0, -64},  {0, -32, -64}, {0, 64, -64},
                                  {0, 64, 64},   {0, -32, 64}};
    static Sint16 faces[] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
    static NJS_MATERIAL material;
    static NJS_MESHSET set = {NJD_MESHSET_3, 4, faces};
    static NJS_MODEL model = {points, NULL, 8,         &set, &material,
                              1,      1,    {0, 0, 0}, 128};
    static NJS_OBJECT ground = {.model = &model, .scl = {1.0F, 1.0F, 1.0F}};
    for (int jump = 0; jump <= 1; jump++) {
        Float y = jump ? 12 : 0;
        struct previous_player previous =
            begin(CHARACTER_SONIC,
                  jump ? sonic_terrain_modes.jump : sonic_terrain_modes.walk,
                  (NJS_POINT3){5.8F, y, -8}, sonic, &ground, 4);
        if (jump) {
            work.flags = 0;
            record.last_flags = record.last_ground_flags = record.ground_flags =
                0;
            record.ground_object = NULL;
        }
        work.ang.y = motion.aim.y = 0x49fb;
        motion.speed = (NJS_VECTOR){-1, 0, 4};
        PLAYER_LOCK_INPUT(&record, 42);
        PLAYER_MARK_DASH_PANEL(&record);
        install(&ground, WORLD_SOLID);
        int hit = player_collide(&work, &motion, &record);
        Bool good = !hit && !!(work.flags & PLAYER_ON_LAND) == !jump &&
                    near(work.pos, (NJS_POINT3){5, y, -4}, .002F) &&
                    near(motion.speed, (NJS_VECTOR){0, 0, 4}, .002F) &&
                    near(record.wall_normal, (NJS_VECTOR){1, 0, 0}, .002F);
        restore(&previous);
        if (!good) {
            return 1 + jump;
        }
    }
    return 0;
}

int terrain_dash_case(terrain_install install,
                      const struct character_constants *sonic)
{
    int result = dash_crest(install, sonic);
    if (result) {
        return result;
    }
    result = dash_wall(install, sonic);
    return result ? 5 + result : 0;
}
