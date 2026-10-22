/*
 * RAVE_TRACK_V1 - the VR hands as RAVE tracks them, for emulator cores that want more than buttons and sticks
 * (the RAVE builds of dolphin_libretro: a Wii Remote that IS the hand; flycast_libretro: Mazan's sword).
 *
 * A named Windows shared-memory block, "Local\\RAVE_TRACK_V1", 512 bytes, little-endian, packed. RAVE (the game)
 * creates and writes it every frame while a VR player plays a system with a screen; a core opens it read-only.
 * SEQLOCK: `sequence` is odd while RAVE writes - read it, copy the block, read it again; use the copy only if both
 * reads were the same EVEN number. LEASE: the data is current while GetTickCount64() < written_ms + lease_ms; after
 * that (RAVE closed, the player left the game) the core goes back to its normal input.
 *
 * SCREEN FRAME (positions, directions, velocity): metres, origin = the centre of the game's picture, x = the
 * picture's right, y = its up, z = INTO the screen (Unity's left-handed axes as the player looking at the screen
 * sees them) - so the player stands at negative z.
 * CONTROLLER FRAME (gyro, accelerometer): x = the controller's right, y = its up, z = where it points.
 *   gyro  = angular velocity, radians / second, about those axes.
 *   accel = proper acceleration (what an accelerometer reads: at rest it reads +9.81 m/s^2 UP), m/s^2.
 * AIM: where the controller's pointing ray hits the picture, u = 0 left .. 1 right, v = 0 top .. 1 bottom
 * (flag RAVE_HAND_AIM_ON_SCREEN; may run past 0..1 just off the picture).
 * TIP: the far end of what the hand holds (a sword's point), screen frame - for a bare hand, a point
 * `tip_length` metres along `forward`.
 */
#ifndef RAVE_TRACK_H
#define RAVE_TRACK_H

#include <stdint.h>

#define RAVE_TRACK_NAME      "Local\\RAVE_TRACK_V1"
#define RAVE_TRACK_NAME_W    L"Local\\RAVE_TRACK_V1"
#define RAVE_TRACK_MAGIC     0x4B545652u /* "RVTK" */
#define RAVE_TRACK_MAJOR     1
#define RAVE_TRACK_SIZE      512
#define RAVE_TRACK_HANDS     2           /* 0 = the main (right) hand, 1 = the other (left) hand */

#define RAVE_TRACK_ACTIVE         0x1u   /* header flags: a player is playing this system in VR */
#define RAVE_HAND_TRACKED         0x1u   /* hand flags */
#define RAVE_HAND_AIM_ON_SCREEN   0x2u

#pragma pack(push, 1)
typedef struct rave_track_hand
{
    uint32_t flags;          /* +0   */
    float position[3];       /* +4   screen frame, m        */
    float forward[3];        /* +16  unit, screen frame     */
    float up[3];             /* +28  unit, screen frame     */
    float velocity[3];       /* +40  m/s, screen frame      */
    float gyro[3];           /* +52  rad/s, controller frame */
    float accel[3];          /* +64  m/s^2, controller frame */
    float aim_uv[2];         /* +76  0..1, y down           */
    float aim_distance;      /* +84  m                       */
    float tip[3];            /* +88  screen frame, m        */
    float tip_length;        /* +100 m                       */
    uint8_t reserved[24];    /* +104 .. 127                  */
} rave_track_hand;

typedef struct rave_track_block
{
    uint32_t magic;          /* @0   RAVE_TRACK_MAGIC        */
    uint16_t major;          /* @4   1                        */
    uint16_t minor;          /* @6   0                        */
    uint32_t size;           /* @8   512                      */
    uint32_t sequence;       /* @12  seqlock                  */
    uint64_t written_ms;     /* @16  GetTickCount64 at write  */
    uint32_t lease_ms;       /* @24                           */
    uint32_t flags;          /* @28  RAVE_TRACK_ACTIVE        */
    float screen_width;      /* @32  m (the picture)         */
    float screen_height;     /* @36  m                        */
    uint32_t hand_count;     /* @40  2                        */
    uint8_t pad[20];         /* @44 .. 63                     */
    rave_track_hand hands[RAVE_TRACK_HANDS]; /* @64, 128 each */
    uint8_t reserved[192];   /* @320 .. 511                   */
} rave_track_block;
#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(rave_track_hand) == 128, "rave_track_hand must be 128 bytes");
static_assert(sizeof(rave_track_block) == RAVE_TRACK_SIZE, "rave_track_block must be 512 bytes");
#endif

#endif
