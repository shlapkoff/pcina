// Solodky, Shlapkov / 050502
// Mar 2013
#include "hexioctrl.h"
#include "pci_codes.h"
#include <stdio.h>
#include <conio.h>

#define PCI_ADDRESS_PORT		0x0CF8
#define PCI_DATA_PORT			0x0CFC
#define PCI_MAX_BUSES			128
#define PCI_MAX_DEVICES			32
#define PCI_MAX_FUNCTIONS		8
#define FILE_OUTPUT				"pci_info.html"

#pragma pack(1)
typedef struct _PCI_CONFIG_ADDRESS {
    union {
        struct {
            UINT32 Zero:2;
            UINT32 RegisterNumber:6;
            UINT32 FunctionNumber:3;
            UINT32 DeviceNumber:5;
            UINT32 BusNumber:8;
            UINT32 Reserved:7;
            UINT32 Enable:1;
        } s1;
        UINT32 Value;
    } u1;
} PCI_CONFIG_ADDRESS;
#pragma pack()

void putDeviceInfo(FILE * file, ULONG devId, ULONG venId, ULONG subVenId, ULONG baseClass, ULONG subClass, ULONG prog){
	// echo class
	for(int i = 0; i < PCI_CLASSCODETABLE_LEN; i++){
		if(PciClassCodeTable[i].BaseClass == baseClass && 
		   PciClassCodeTable[i].SubClass == subClass &&
		   PciClassCodeTable[i].ProgIf == prog)
		{
			fprintf(file, "%s %s %s", 
				PciClassCodeTable[i].BaseDesc, 
				PciClassCodeTable[i].SubDesc, 
				PciClassCodeTable[i].ProgDesc);
			break;
		}
	}
	fprintf(file, "</td><td>");
	// echo vendor
	for(int i = 0; i < PCI_VENTABLE_LEN; i++){
		if(PciVenTable[i].VenId == venId){
			fprintf(file, "%s", PciVenTable[i].VenFull); break;
		}
	}
	fprintf(file, "</td><td>");

	// echo subsystem vendor
	if (subVenId == 0) subVenId = venId;
	for(int i = 0; i < PCI_VENTABLE_LEN; i++){
		if(PciVenTable[i].VenId == subVenId){
			fprintf(file, "%s", PciVenTable[i].VenFull); break;
		}
	}
	fprintf(file, "</td><td>");
	// echo dev
	for(int i = 0; i < PCI_DEVTABLE_LEN; i++){
		if(PciDevTable[i].VenId == venId && PciDevTable[i].DevId == devId){
			fprintf(file, "%s %s", PciDevTable[i].Chip, PciDevTable[i].ChipDesc); break;
		}
	}
	fprintf(file, "</td><td>");
}


int main(){
	// variables
	PCI_CONFIG_ADDRESS cfg;
	USHORT bus = 0, dev = 0, func = 0;
	ULONG val = 0;
	ULONG devId, venId;
	ULONG class0, class1, class2, class3, revision;
	ULONG subId, subVenId;
	int count = 0;

	// init	
	// load driver
	ALLOW_IO_OPERATIONS;
	// open output file
	FILE * f = fopen(FILE_OUTPUT, "w");
	if (!f) { printf("Can't create output file. Abort"); return 1; }
	// printf output header
	fprintf(f, "<table><tr><th>Bus</th><th>Device</th><th>Function</th><th>DeviceId</th><th>VendorId</th><th>ClassName</th><th>VendorName</th><th>SubVenName</th><th>DeviceName</th><th>Class</th><th>Revision</th><th>SubId</th><th>SubVendorId</th></tr>\n");


	printf("PCI look-up started...\n");
	// loop through all bus-dev-func variations
	for(bus = 0; bus < PCI_MAX_BUSES; bus++){
		for(dev = 0; dev < PCI_MAX_DEVICES; dev++){
			for(func = 0; func < PCI_MAX_FUNCTIONS; func++){
				// prepate bits
				cfg.u1.s1.Enable = 1;
				cfg.u1.s1.BusNumber = bus;
				cfg.u1.s1.DeviceNumber = dev;
				cfg.u1.s1.FunctionNumber = func;
				cfg.u1.s1.RegisterNumber = 0;
				// read general info
				_outpd(PCI_ADDRESS_PORT, cfg.u1.Value);
				val = _inpd(PCI_DATA_PORT);
				// if no device - continue
				if (val == 0 || val == -1) continue;
				count++;

				// get deviceId and vendorId
				devId = val >> 16;
				venId = val - (devId << 16);

				// read classId, revisionId
				cfg.u1.s1.RegisterNumber = 0x08 >> 2;
				_outpd(PCI_ADDRESS_PORT, cfg.u1.Value);
				val = _inpd(PCI_DATA_PORT);
				class0 = val >> 8;
				revision = val - (class0 << 8);
				class1 = class0 >> 16;
				class3 = class0 - (class1 << 16);
				class2 = class3 >> 8;
				class3 = class3 - (class2 << 8);

				// read subsystemId
				cfg.u1.s1.RegisterNumber = 0x2C >> 2;
				_outpd(PCI_ADDRESS_PORT, cfg.u1.Value);
				val = _inpd(PCI_DATA_PORT);
				subId = val >> 16;
				subVenId = val - (subId << 16);

				// all data collected
				// put data to file
				fprintf(f, "<tr><td>%02X</td><td>%02X</td><td>%02X</td><td>%02X</td><td>%02X</td><td>",
					bus, dev, func, devId, venId);
				putDeviceInfo(f, devId, venId, subVenId, class1, class2, class3);
				fprintf(f, "%02X-%02X-%02X</td><td>%02X</td><td>%02X</td><td>%02X</td></tr>\n",
					class1, class2, class3, revision, subId, subVenId);
			}
		}
	}
	// close table
	fprintf(f, "</table><p>Total PCI devices discovered: %d</p>", count);

	printf("\nPCI look-up done. Devices discovered: %d\nSee %s to details\n", count, FILE_OUTPUT);
	return 0;
}