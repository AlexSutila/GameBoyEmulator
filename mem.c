#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include "mem.h"
#include "ppu.h"
#include "hwio.h"

// Full contents of the DMG bootrom, checks the cartridge header, scrolls
//		nintendo logo and plays the funny blingy sound
static uint8_t bootcode[256] = 
{ 
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB,
	0x21, 0x26, 0xFF, 0x0E, 0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3,
	0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0, 0x47, 0x11, 0x04, 0x01,
	0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22,
	0x23, 0x05, 0x20, 0xF9, 0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99,
	0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20, 0xF9, 0x2E, 0x0F, 0x18,
	0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20,
	0xF7, 0x1D, 0x20, 0xF2, 0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62,
	0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06, 0x7B, 0xE2, 0x0C, 0x3E,
	0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17,
	0xC1, 0xCB, 0x11, 0x17, 0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9,
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83,
	0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63,
	0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
	0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C, 0x21, 0x04, 0x01, 0x11,
	0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE,
	0x3E, 0x01, 0xE0, 0x50

};

// Opens file explorer and initializes cart if a rom is selected
void open_romFile(struct cartridge* cart_ptr) {

	OPENFILENAME ofn;
	TCHAR szFile[260] = { 0 };

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"rom_file.gb\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		char c_szText[260];
		wcstombs(c_szText, szFile, wcslen(szFile) + 1);
		init_cart(cart_ptr, c_szText);
	}
	else exit(0);
}

// ////////////////////////////////////////////////////////////////////////////////////////////////

uint8_t RB(struct GB* gb, uint16_t addr, uint8_t cycles)
{
	static const uint8_t(*IO_reads[128])(struct GB*, uint8_t) = {
		/* 0xFF00 */ &JOYP_RB,
		/* 0xFF01 */ NULL,
		/* 0xFF02 */ NULL,
		/* 0xFF03 */ NULL,
		/* 0xFF04 */ &DIV_RB,
		/* 0xFF05 */ &TIMA_RB,
		/* 0xFF06 */ &TMA_RB,
		/* 0xFF07 */ &TAC_RB,
		/* 0xFF08 */ NULL,
		/* 0xFF09 */ NULL,
		/* 0xFF0A */ NULL,
		/* 0xFF0B */ NULL,
		/* 0xFF0C */ NULL,
		/* 0xFF0D */ NULL,
		/* 0xFF0E */ NULL,
		/* 0xFF0F */ NULL,
		/* 0xFF10 */ NULL,
		/* 0xFF11 */ NULL,
		/* 0xFF12 */ NULL,
		/* 0xFF13 */ NULL,
		/* 0xFF14 */ NULL,
		/* 0xFF15 */ NULL,
		/* 0xFF16 */ NULL,
		/* 0xFF17 */ NULL,
		/* 0xFF18 */ NULL,
		/* 0xFF19 */ NULL,
		/* 0xFF1A */ NULL,
		/* 0xFF1B */ NULL,
		/* 0xFF1C */ NULL,
		/* 0xFF1D */ NULL,
		/* 0xFF1E */ NULL,
		/* 0xFF1F */ NULL,
		/* 0xFF20 */ NULL,
		/* 0xFF21 */ NULL,
		/* 0xFF22 */ NULL,
		/* 0xFF23 */ NULL,
		/* 0xFF24 */ NULL,
		/* 0xFF25 */ NULL,
		/* 0xFF26 */ NULL,
		/* 0xFF27 */ NULL,
		/* 0xFF28 */ NULL,
		/* 0xFF29 */ NULL,
		/* 0xFF2A */ NULL,
		/* 0xFF2B */ NULL,
		/* 0xFF2C */ NULL,
		/* 0xFF2D */ NULL,
		/* 0xFF2E */ NULL,
		/* 0xFF2F */ NULL,
		/* 0xFF30 */ NULL,
		/* 0xFF31 */ NULL,
		/* 0xFF32 */ NULL,
		/* 0xFF33 */ NULL,
		/* 0xFF34 */ NULL,
		/* 0xFF35 */ NULL,
		/* 0xFF36 */ NULL,
		/* 0xFF37 */ NULL,
		/* 0xFF38 */ NULL,
		/* 0xFF39 */ NULL,
		/* 0xFF3A */ NULL,
		/* 0xFF3B */ NULL,
		/* 0xFF3C */ NULL,
		/* 0xFF3D */ NULL,
		/* 0xFF3E */ NULL,
		/* 0xFF3F */ NULL,
		/* 0xFF40 */ &LCDC_RB,
		/* 0xFF41 */ &STAT_RB,
		/* 0xFF42 */ &SCY_RB,
		/* 0xFF43 */ &SCX_RB,
		/* 0xFF44 */ &LY_RB,
		/* 0xFF45 */ &LYC_RB,
		/* 0xFF46 */ &DMA_RB,
		/* 0xFF47 */ &BGP_RB,
		/* 0xFF48 */ &OBP0_RB,
		/* 0xFF49 */ &OBP1_RB,
		/* 0xFF4A */ &WY_RB,
		/* 0xFF4B */ &WX_RB,
		/* 0xFF4C */ NULL,
		/* 0xFF4D */ NULL,
		/* 0xFF4E */ NULL,
		/* 0xFF4F */ NULL,
		/* 0xFF50 */ NULL,
		/* 0xFF51 */ NULL,
		/* 0xFF52 */ NULL,
		/* 0xFF53 */ NULL,
		/* 0xFF54 */ NULL,
		/* 0xFF55 */ NULL,
		/* 0xFF56 */ NULL,
		/* 0xFF57 */ NULL,
		/* 0xFF58 */ NULL,
		/* 0xFF59 */ NULL,
		/* 0xFF5A */ NULL,
		/* 0xFF5B */ NULL,
		/* 0xFF5C */ NULL,
		/* 0xFF5D */ NULL,
		/* 0xFF5E */ NULL,
		/* 0xFF5F */ NULL,
		/* 0xFF60 */ NULL,
		/* 0xFF61 */ NULL,
		/* 0xFF62 */ NULL,
		/* 0xFF63 */ NULL,
		/* 0xFF64 */ NULL,
		/* 0xFF65 */ NULL,
		/* 0xFF66 */ NULL,
		/* 0xFF67 */ NULL,
		/* 0xFF68 */ NULL,
		/* 0xFF69 */ NULL,
		/* 0xFF6A */ NULL,
		/* 0xFF6B */ NULL,
		/* 0xFF6C */ NULL,
		/* 0xFF6D */ NULL,
		/* 0xFF6E */ NULL,
		/* 0xFF6F */ NULL,
		/* 0xFF70 */ NULL,
		/* 0xFF71 */ NULL,
		/* 0xFF72 */ NULL,
		/* 0xFF73 */ NULL,
		/* 0xFF74 */ NULL,
		/* 0xFF75 */ NULL,
		/* 0xFF76 */ NULL,
		/* 0xFF77 */ NULL,
		/* 0xFF78 */ NULL,
		/* 0xFF79 */ NULL,
		/* 0xFF7A */ NULL,
		/* 0xFF7B */ NULL,
		/* 0xFF7C */ NULL,
		/* 0xFF7D */ NULL,
		/* 0xFF7E */ NULL,
		/* 0xFF7F */ NULL
	};

	// Cart read or bootrom
	if (addr >= 0x0000 && addr <= 0x00FF) {
		if (gb->memory[0xFF50] != 0x01) return gb->bootstrap[addr];
		else return (*gb->cart.memAccess)(&gb->cart, addr, 0xFF, CART_READ);
	}
	// Cart read - ROM
	else if (addr >= 0x0100 && addr <= 0x7FFF) {
		return (*gb->cart.memAccess)(&gb->cart, addr, 0xFF, CART_READ);
	}
	// VRAM
	else if (addr >= 0x8000 && addr <= 0x9FFF) {
		return gb->vram[addr - 0x8000];
	}
	// Cart read - RAM
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		return (*gb->cart.memAccess)(&gb->cart, addr, 0xFF, CART_READ);
	}
	// ECHO memory 
	else if (addr >= 0xE000 && addr <= 0xDDFF) {
		return gb->memory[addr - 0x2000];
	}
	// Unusable - reads always FF?
	else if (addr >= 0xFEA0 && addr <= 0xFEFF) {
		return 0xFF;
	}
	// Mapped IO
	else if (addr >= 0xFF00 && addr <= 0xFF7F) {
		uint8_t const (*readFunc)(struct GB*, uint8_t) = IO_reads[addr - 0xFF00];
		if (readFunc != NULL) return (*readFunc)(gb, cycles);
		else return gb->memory[addr];
	}
	// Rest of memory
	else return gb->memory[addr];
}

void WB(struct GB* gb, uint16_t addr, uint8_t val, uint8_t cycles)
{
	static const void (*IO_writes[128])(struct GB*, uint8_t, uint8_t) = {
		/* 0xFF00 */ &JOYP_WB,
		/* 0xFF01 */ NULL,
		/* 0xFF02 */ &SC_WB,
		/* 0xFF03 */ NULL,
		/* 0xFF04 */ &DIV_WB,
		/* 0xFF05 */ &TIMA_WB,
		/* 0xFF06 */ &TMA_WB,
		/* 0xFF07 */ &TAC_WB,
		/* 0xFF08 */ NULL,
		/* 0xFF09 */ NULL,
		/* 0xFF0A */ NULL,
		/* 0xFF0B */ NULL,
		/* 0xFF0C */ NULL,
		/* 0xFF0D */ NULL,
		/* 0xFF0E */ NULL,
		/* 0xFF0F */ NULL,
		/* 0xFF10 */ NULL,
		/* 0xFF11 */ NULL,
		/* 0xFF12 */ NULL,
		/* 0xFF13 */ NULL,
		/* 0xFF14 */ NULL,
		/* 0xFF15 */ NULL,
		/* 0xFF16 */ NULL,
		/* 0xFF17 */ NULL,
		/* 0xFF18 */ NULL,
		/* 0xFF19 */ NULL,
		/* 0xFF1A */ NULL,
		/* 0xFF1B */ NULL,
		/* 0xFF1C */ NULL,
		/* 0xFF1D */ NULL,
		/* 0xFF1E */ NULL,
		/* 0xFF1F */ NULL,
		/* 0xFF20 */ NULL,
		/* 0xFF21 */ NULL,
		/* 0xFF22 */ NULL,
		/* 0xFF23 */ NULL,
		/* 0xFF24 */ NULL,
		/* 0xFF25 */ NULL,
		/* 0xFF26 */ NULL,
		/* 0xFF27 */ NULL,
		/* 0xFF28 */ NULL,
		/* 0xFF29 */ NULL,
		/* 0xFF2A */ NULL,
		/* 0xFF2B */ NULL,
		/* 0xFF2C */ NULL,
		/* 0xFF2D */ NULL,
		/* 0xFF2E */ NULL,
		/* 0xFF2F */ NULL,
		/* 0xFF30 */ NULL,
		/* 0xFF31 */ NULL,
		/* 0xFF32 */ NULL,
		/* 0xFF33 */ NULL,
		/* 0xFF34 */ NULL,
		/* 0xFF35 */ NULL,
		/* 0xFF36 */ NULL,
		/* 0xFF37 */ NULL,
		/* 0xFF38 */ NULL,
		/* 0xFF39 */ NULL,
		/* 0xFF3A */ NULL,
		/* 0xFF3B */ NULL,
		/* 0xFF3C */ NULL,
		/* 0xFF3D */ NULL,
		/* 0xFF3E */ NULL,
		/* 0xFF3F */ NULL,
		/* 0xFF40 */ &LCDC_WB,
		/* 0xFF41 */ &STAT_WB,
		/* 0xFF42 */ &SCY_WB,
		/* 0xFF43 */ &SCX_WB,
		/* 0xFF44 */ &LY_WB,
		/* 0xFF45 */ &LYC_WB,
		/* 0xFF46 */ &DMA_WB,
		/* 0xFF47 */ &BGP_WB,
		/* 0xFF48 */ &OBP0_WB,
		/* 0xFF49 */ &OBP1_WB,
		/* 0xFF4A */ &WY_WB,
		/* 0xFF4B */ &WX_WB,
		/* 0xFF4C */ NULL,
		/* 0xFF4D */ NULL,
		/* 0xFF4E */ NULL,
		/* 0xFF4F */ NULL,
		/* 0xFF50 */ NULL,
		/* 0xFF51 */ NULL,
		/* 0xFF52 */ NULL,
		/* 0xFF53 */ NULL,
		/* 0xFF54 */ NULL,
		/* 0xFF55 */ NULL,
		/* 0xFF56 */ NULL,
		/* 0xFF57 */ NULL,
		/* 0xFF58 */ NULL,
		/* 0xFF59 */ NULL,
		/* 0xFF5A */ NULL,
		/* 0xFF5B */ NULL,
		/* 0xFF5C */ NULL,
		/* 0xFF5D */ NULL,
		/* 0xFF5E */ NULL,
		/* 0xFF5F */ NULL,
		/* 0xFF60 */ NULL,
		/* 0xFF61 */ NULL,
		/* 0xFF62 */ NULL,
		/* 0xFF63 */ NULL,
		/* 0xFF64 */ NULL,
		/* 0xFF65 */ NULL,
		/* 0xFF66 */ NULL,
		/* 0xFF67 */ NULL,
		/* 0xFF68 */ NULL,
		/* 0xFF69 */ NULL,
		/* 0xFF6A */ NULL,
		/* 0xFF6B */ NULL,
		/* 0xFF6C */ NULL,
		/* 0xFF6D */ NULL,
		/* 0xFF6E */ NULL,
		/* 0xFF6F */ NULL,
		/* 0xFF70 */ NULL,
		/* 0xFF71 */ NULL,
		/* 0xFF72 */ NULL,
		/* 0xFF73 */ NULL,
		/* 0xFF74 */ NULL,
		/* 0xFF75 */ NULL,
		/* 0xFF76 */ NULL,
		/* 0xFF77 */ NULL,
		/* 0xFF78 */ NULL,
		/* 0xFF79 */ NULL,
		/* 0xFF7A */ NULL,
		/* 0xFF7B */ NULL,
		/* 0xFF7C */ NULL,
		/* 0xFF7D */ NULL,
		/* 0xFF7E */ NULL,
		/* 0xFF7F */ NULL
	};

	// Cart re-map
	if (addr >= 0x0000 && addr <= 0x7FFF) {
		(*gb->cart.memAccess)(&gb->cart, addr, val, CART_WRITE);
	}
	// VRAM
	else if (addr >= 0x8000 && addr <= 0x9FFF) {
		gb->vram[addr - 0x8000] = val;
	}
	// Cart re-map
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		(*gb->cart.memAccess)(&gb->cart, addr, val, CART_WRITE);
	}
	// ECHO memory 
	else if (addr >= 0xE000 && addr <= 0xDDFF) {
		gb->memory[addr - 0x2000] = val;
	}
	// Unusable 
	else if (addr >= 0xFEA0 && addr <= 0xFEFF) {
		return; // Writes to this address range are ignored
	}
	// Mapped IO
	else if (addr >= 0xFF00 && addr <= 0xFF7F) {
		void const (*writeFunc)(struct GB*, uint8_t, uint8_t) = IO_writes[addr - 0xFF00];
		if (writeFunc != NULL) (*writeFunc)(gb, val, cycles);
		else gb->memory[addr] = val;
	}
	// Rest of memory
	else gb->memory[addr] = val;
}

// ////////////////////////////////////////////////////////////////////////////////////////////////

#define VRAM_BASE_ADDR 0x8000
// Access a specific tile, given the tile map base address and index
const struct tileStruct* tile_access(struct GB* gb, uint16_t map_base, uint16_t map_index)
{	// Obtain the tile index from the tile map, selected by the map_base. Important, map_base
	//		should only ever be 0x9800 or 0x9C00 as there are only two tile maps
	uint16_t tiledata_index = gb->vram[map_base + map_index - VRAM_BASE_ADDR];
	// Tiles are indexed one of two ways, using either signed or unsigned addressing. This is
	//		determined by LCDC bit 4
	if (gb->ppu.bgwin_tiledata)
	{	// Unsigned tile addressing, uses 0x8000 as base pointer
		return (struct tileStruct*)&gb->vram[tiledata_index*sizeof(struct tileStruct)];
	}
	else
	{	// Signed tile addressing, uses 0x9000 as base pointer
		int16_t signed_index = (int16_t)tiledata_index;
		return (struct tileStruct*)&gb->vram[0x1000+(signed_index*sizeof(struct tileStruct))];
	}
}
// Access a specific sprite, given the index into object attribute memory
const struct spriteStruct* objattr_access(struct GB* gb, uint16_t index)
{
	struct spriteStruct* sprites = (struct spriteStruct*)(gb->memory + 0xFE00);
	return &sprites[index];
}
#undef VRAM_BASE_ADDR

// ////////////////////////////////////////////////////////////////////////////////////////////////

void gb_init(struct GB* gb)
{
	// Memory initializations
	gb->sync_sel = 0;
	gb->memory = (uint8_t*)calloc(0x10000, sizeof(uint8_t)); // Do not plan on sticking with this 
	                                                         // use of direct addressing, wasteful
	gb->vram = (uint8_t*)calloc(0x2000, sizeof(uint8_t));
	gb->bootstrap = bootcode;
	open_romFile(&gb->cart); // Calls cart init

	// Initializing mapped IO to boot values - need to double check these
	gb->memory[0xFF00] = 0xCF;
	gb->memory[0xFF02] = 0x7E;
	gb->memory[0xFF03] = 0xFF;
	gb->memory[0xFF07] = 0xF8;
	gb->memory[0xFF08] = 0xFF;
	gb->memory[0xFF09] = 0xFF;
	gb->memory[0xFF0A] = 0xFF;
	gb->memory[0xFF0B] = 0xFF;
	gb->memory[0xFF0C] = 0xFF;
	gb->memory[0xFF0D] = 0xFF;
	gb->memory[0xFF0E] = 0xFF;
	gb->memory[0xFF0F] = 0xE1;
	gb->memory[0xFF10] = 0x80;
	gb->memory[0xFF11] = 0xBF;
	gb->memory[0xFF12] = 0xF3;
	gb->memory[0xFF13] = 0xFF;
	gb->memory[0xFF14] = 0xBF;
	gb->memory[0xFF15] = 0xFF;
	gb->memory[0xFF16] = 0x3F;
	gb->memory[0xFF17] = 0x00;
	gb->memory[0xFF18] = 0xFF;
	gb->memory[0xFF19] = 0xBF;
	gb->memory[0xFF1A] = 0x7F;
	gb->memory[0xFF1B] = 0xFF;
	gb->memory[0xFF1C] = 0x9F;
	gb->memory[0xFF1D] = 0xFF;
	gb->memory[0xFF1E] = 0xBF;
	gb->memory[0xFF1F] = 0xFF;
	gb->memory[0xFF20] = 0xFF;
	gb->memory[0xFF23] = 0xBF;
	gb->memory[0xFF24] = 0x77;
	gb->memory[0xFF25] = 0xF3;
	gb->memory[0xFF26] = 0xF1;
	gb->memory[0xFF27] = 0xFF;
	gb->memory[0xFF28] = 0xFF;
	gb->memory[0xFF29] = 0xFF;
	gb->memory[0xFF2A] = 0xFF;
	gb->memory[0xFF2B] = 0xFF;
	gb->memory[0xFF2C] = 0xFF;
	gb->memory[0xFF2D] = 0xFF;
	gb->memory[0xFF2E] = 0xFF;
	gb->memory[0xFF2F] = 0xFF;
	gb->memory[0xFF31] = 0xFF;
	gb->memory[0xFF33] = 0xFF;
	gb->memory[0xFF35] = 0xFF;
	gb->memory[0xFF37] = 0xFF;
	gb->memory[0xFF39] = 0xFF;
	gb->memory[0xFF3B] = 0xFF;
	gb->memory[0xFF3D] = 0xFF;
	gb->memory[0xFF3F] = 0xFF;
	gb->memory[0xFF41] = 0x81;
	gb->memory[0xFF46] = 0xFF;
	gb->memory[0xFF47] = 0xFC;
	gb->memory[0xFF48] = 0xFF;
	gb->memory[0xFF49] = 0xFF;
	gb->memory[0xFF4C] = 0xFF;
	gb->memory[0xFF4D] = 0xFF;
	gb->memory[0xFF4E] = 0xFF;
	gb->memory[0xFF4F] = 0xFF;
	gb->memory[0xFF50] = 0xFF;
	gb->memory[0xFF51] = 0xFF;
	gb->memory[0xFF52] = 0xFF;
	gb->memory[0xFF53] = 0xFF;
	gb->memory[0xFF54] = 0xFF;
	gb->memory[0xFF55] = 0xFF;
	gb->memory[0xFF56] = 0xFF;
	gb->memory[0xFF57] = 0xFF;
	gb->memory[0xFF58] = 0xFF;
	gb->memory[0xFF59] = 0xFF;
	gb->memory[0xFF5A] = 0xFF;
	gb->memory[0xFF5B] = 0xFF;
	gb->memory[0xFF5C] = 0xFF;
	gb->memory[0xFF5D] = 0xFF;
	gb->memory[0xFF5E] = 0xFF;
	gb->memory[0xFF5F] = 0xFF;
	gb->memory[0xFF60] = 0xFF;
	gb->memory[0xFF61] = 0xFF;
	gb->memory[0xFF62] = 0xFF;
	gb->memory[0xFF63] = 0xFF;
	gb->memory[0xFF64] = 0xFF;
	gb->memory[0xFF65] = 0xFF;
	gb->memory[0xFF66] = 0xFF;
	gb->memory[0xFF67] = 0xFF;
	gb->memory[0xFF68] = 0xFF;
	gb->memory[0xFF69] = 0xFF;
	gb->memory[0xFF6A] = 0xFF;
	gb->memory[0xFF6B] = 0xFF;
	gb->memory[0xFF6C] = 0xFF;
	gb->memory[0xFF6D] = 0xFF;
	gb->memory[0xFF6E] = 0xFF;
	gb->memory[0xFF6F] = 0xFF;
	gb->memory[0xFF70] = 0xFF;
	gb->memory[0xFF71] = 0xFF;
	gb->memory[0xFF72] = 0xFF;
	gb->memory[0xFF73] = 0xFF;
	gb->memory[0xFF74] = 0xFF;
	gb->memory[0xFF75] = 0xFF;
	gb->memory[0xFF76] = 0xFF;
	gb->memory[0xFF77] = 0xFF;
	gb->memory[0xFF78] = 0xFF;
	gb->memory[0xFF79] = 0xFF;
	gb->memory[0xFF7A] = 0xFF;
	gb->memory[0xFF7B] = 0xFF;
	gb->memory[0xFF7C] = 0xFF;
	gb->memory[0xFF7D] = 0xFF;
	gb->memory[0xFF7E] = 0xFF;
	gb->memory[0xFF7F] = 0xFF;

	// Initialize other components
	cpu_init(&gb->cpu);
	init_JOYP(&gb->joyp);
	init_timers(&gb->timer);
	init_ppu(&gb->ppu);
}

void gb_free(struct GB* gb)
{
	free_cart(&gb->cart);
	free_ppu(&gb->ppu);
	free(gb->memory);
	free(gb->vram);
}

// This only exists because the upper 3 bits of the 
//		IF register should always read as 1
uint8_t IF_RB(struct GB* gb, uint8_t cycles)
{
	return gb->memory[0xFF0F] | 0xE0;
}
