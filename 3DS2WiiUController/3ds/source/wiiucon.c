/*
 * 3ds -> wiiu controller conversion
 */

#include <3ds.h>

/* Wii U Keys definitions */
#define WPAD_PRO_BUTTON_UP             0x00000001
#define WPAD_PRO_BUTTON_LEFT           0x00000002
#define WPAD_PRO_TRIGGER_ZR            0x00000004
#define WPAD_PRO_BUTTON_X              0x00000008
#define WPAD_PRO_BUTTON_A              0x00000010
#define WPAD_PRO_BUTTON_Y              0x00000020
#define WPAD_PRO_BUTTON_B              0x00000040
#define WPAD_PRO_TRIGGER_ZL            0x00000080
#define WPAD_PRO_RESERVED              0x00000100
#define WPAD_PRO_TRIGGER_R             0x00000200
#define WPAD_PRO_BUTTON_PLUS           0x00000400
#define WPAD_PRO_BUTTON_HOME           0x00000800
#define WPAD_PRO_BUTTON_MINUS          0x00001000
#define WPAD_PRO_TRIGGER_L             0x00002000
#define WPAD_PRO_BUTTON_DOWN           0x00004000
#define WPAD_PRO_BUTTON_RIGHT          0x00008000
#define WPAD_PRO_BUTTON_STICK_L        0x00020000

#define WPAD_PRO_STICK_L_EMULATION_UP        0x00200000
#define WPAD_PRO_STICK_L_EMULATION_DOWN      0x00100000
#define WPAD_PRO_STICK_L_EMULATION_LEFT      0x00040000
#define WPAD_PRO_STICK_L_EMULATION_RIGHT     0x00080000

/* Assign a Wii U Key to the 32 bits of the 3ds input data */
u32 KeyConversionTable[32] = {
	WPAD_PRO_BUTTON_A,					// BIT(0): KEY_A
	WPAD_PRO_BUTTON_B,					// BIT(1): KEY_B
	WPAD_PRO_BUTTON_MINUS,				// BIT(2): KEY_SELECT
	WPAD_PRO_BUTTON_PLUS,				// BIT(3): KEY_START
	WPAD_PRO_BUTTON_RIGHT,				// BIT(4): KEY_DRIGHT
	WPAD_PRO_BUTTON_LEFT,				// BIT(5): KEY_DLEFT
	WPAD_PRO_BUTTON_UP,					// BIT(6): KEY_DUP
	WPAD_PRO_BUTTON_DOWN,				// BIT(7): KEY_DDOWN
	WPAD_PRO_TRIGGER_R,					// BIT(8): KEY_R
	WPAD_PRO_TRIGGER_L,					// BIT(9): KEY_L
	WPAD_PRO_BUTTON_X,					// BIT(10): KEY_X
	WPAD_PRO_BUTTON_Y,					// BIT(11): KEY_Y
	0, 0, 								// BIT(12,13): NONE
	WPAD_PRO_TRIGGER_ZL,				// BIT(14): KEY_ZL
	WPAD_PRO_TRIGGER_ZR,				// BIT(15): KEY_ZR
	0, 0, 0, 0,							// BIT(16,17,18,19): NONE
	0,									// BIT(20): KEY_TOUCH
	0, 0, 0,							// BIT(21,22,23): NONE
	0,									// BIT(24): KEY_CSTICK_RIGHT
	0,									// BIT(25): KEY_CSTICK_LEFT
	0,									// BIT(26): KEY_CSTICK_UP
	0,									// BIT(27): KEY_CSTICK_DOWN
	WPAD_PRO_STICK_L_EMULATION_RIGHT,	// BIT(28): KEY_CPAD_RIGHT
	WPAD_PRO_STICK_L_EMULATION_LEFT,	// BIT(29): KEY_CPAD_LEFT
	WPAD_PRO_STICK_L_EMULATION_UP,		// BIT(30): KEY_CPAD_UP
	WPAD_PRO_STICK_L_EMULATION_DOWN,	// BIT(31): KEY_CPAD_DOWN
};


u32 getKeysWiiU(u32 Keys3ds)
{
	u32 KeysWiiU = 0;
	for (int i = 0; i < 32; i++)
	{
		if (Keys3ds & BIT(i))
		{
			KeysWiiU |= KeyConversionTable[i];
		}
	}
	return KeysWiiU;
}